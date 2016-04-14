/*
  This file is part of the FRED system.

  Copyright (c) 2010-2016, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Population.cc
//

#include <unistd.h>

#include "Activities.h"
#include "Age_Map.h"
#include "AV_Manager.h"
#include "Behavior.h"
#include "Date.h"
#include "Demographics.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Evolution.h"
#include "Geo.h"
#include "Global.h"
#include "Health.h"
#include "Household.h"
#include "Manager.h"
#include "Params.h"
#include "Past_Infection.h"
#include "Person.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "School.h"
#include "Travel.h"
#include "Utils.h"
#include "Vaccine_Manager.h"

#if SNAPPY
#include "Compression.h"
#endif


Population::Population() {

  this->is_initialized = false;
  this->load_completed = false;
  this->next_id = 0;

  // clear_static_arrays();
  this->pop_size = 0;
  this->av_manager = NULL;
  this->vacc_manager = NULL;
  this->enable_copy_files = 0;

  // reserve memory for lists
  this->death_list.reserve(1000);
  this->migrant_list.reserve(1000);
  this->people.clear();

}

Population::~Population() {
  // free all memory
  this->pop_size = 0;
  if(this->vacc_manager != NULL) {
    delete this->vacc_manager;
  }
  if(this->av_manager != NULL) {
    delete this->av_manager;
  }
}

void Population::get_parameters() {

  // Only do this one time
  if(!this->is_initialized) {
    Params::get_param_from_string("enable_copy_files", &this->enable_copy_files);
    Params::get_param_from_string("output_population", &this->output_population);
    if(this->output_population > 0) {
      Params::get_param_from_string("pop_outfile", this->pop_outfile);
      Params::get_param_from_string("output_population_date_match",
				    this->output_population_date_match);
    }
    this->is_initialized = true;
  }
}

/*
 * All Persons in the population must have been created using add_person_to_population
 */
Person* Population::add_person_to_population(int age, char sex, int race, int rel,
			       Place* house, Place* school, Place* work,
			       int day, bool today_is_birthday) {

  Person* person = new Person;
  int id = this->next_id++;
  int idx = this->people.size();
  person->setup(idx, id, age, sex, race, rel, house, school, work, day, today_is_birthday);
  this->people.push_back(person);
  this->pop_size = this->people.size();
  return person;
}


void Population::prepare_to_die(int day, Person* person) {
  if (person->is_deceased() == false) {
    // add person to daily death_list
    fred::Scoped_Lock lock(this->mutex);
    this->death_list.push_back(person);
    person->set_deceased();
  }
}

void Population::prepare_to_migrate(int day, Person* person) {
  if (person->is_eligible_to_migrate()) {
    // add person to daily migrant_list
    fred::Scoped_Lock lock(this->mutex);
    this->migrant_list.push_back(person);
    person->clear_eligible_to_migrate();
  }
}

void Population::setup() {
  FRED_STATUS(0, "setup population entered\n", "");

  if(Global::Enable_Vaccination) {
    this->vacc_manager = new Vaccine_Manager(this);
  } else {
    this->vacc_manager = new Vaccine_Manager();
  }

  if(Global::Enable_Antivirals) {
    Utils::fred_abort("Sorry, antivirals are not enabled in this version of FRED.");
    this->av_manager = new AV_Manager(this);
  } else {
    this->av_manager = new AV_Manager();
  }

  if(Global::Verbose > 1) {
    this->av_manager->print();
  }

  this->pop_size = 0;
  this->death_list.clear();
  this->migrant_list.clear();
  read_all_populations();

  if(Global::Enable_Behaviors) {
    // select adult to make health decisions
    initialize_population_behavior();
  }

  if(Global::Enable_Health_Insurance) {
    // select insurance coverage
    // try to make certain that everyone in a household has same coverage
    initialize_health_insurance();
  }

  this->load_completed = true;
  
  if(Global::Enable_Population_Dynamics) {
    initialize_demographic_dynamics();
  }

  initialize_activities();

  if(Global::Verbose > 0) {
    for(int d = 0; d < Global::Conditions.get_number_of_conditions(); ++d) {
      int count = 0;
      for(int p = 0; p < this->get_population_size(); ++p) {
	Person* person = get_person(p);
	if(person->is_immune(d)) {
	  count++;
	}
      }
      FRED_STATUS(0, "number of residually immune people for condition %d = %d\n", d, count);
    }
  }
  this->av_manager->reset();
  this->vacc_manager->reset();

  // record age-specific popsize
  for(int age = 0; age <= Demographics::MAX_AGE; ++age) {
    Global::Popsize_by_age[age] = 0;
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    Global::Popsize_by_age[age]++;
  }

  FRED_STATUS(0, "population setup finished\n", "");
}

void Population::get_person_data(char* line, bool gq) {

  int day = 0;

  // person data
  char label[FRED_STRING_SIZE];
  Place* house = NULL;
  Place* work = NULL;
  Place* school = NULL;
  int age = -1;
  int race = -1;
  int relationship = -1;
  int sex = -1;
  bool today_is_birthday = false;

  // place labels
  char house_label[FRED_STRING_SIZE];
  char school_label[FRED_STRING_SIZE];
  char work_label[FRED_STRING_SIZE];

  strcpy(label, "-1");
  strcpy(house_label, "-1");
  strcpy(school_label, "-1");
  strcpy(work_label, "-1");

  // clean up data format
  char newline[FRED_STRING_SIZE];
  Utils::replace_csv_missing_data(newline, line, "-1");
  Utils::Tokens tokens = Utils::split_by_delim(newline, ',', false);

  // input format
  int pid_col = 0;
  int house_col = 1;
  int age_col = 4;
  int sex_col = 5;
  int race_col = 6;
  int rel_col = 8;
  int school_col = 9;
  int work_col = 10;

  if (gq) {
    age_col = 3;
    sex_col = 4;
    work_col = house_col;
  }

  // person label
  sscanf(tokens[pid_col], "%s", label);

  if (strcmp(label,"sp_id")==0) {
    // header line
    return;
  } 

  // age
  sscanf(tokens[age_col], "%d", &age);

  // sex
  sex = strcmp(tokens[sex_col], "1") == 0 ? 'M' : 'F';

  // house
  if(strcmp(tokens[house_col], "-1")) {
    sprintf(house_label, "H%s", tokens[house_col]);
  }
  
  // workplace
  if(strcmp(tokens[work_col], "-1")) {
    sprintf(work_label, "W%s", tokens[work_col]);
  }
  
  if (gq == false) {

    // race
    sscanf(tokens[race_col], "%d", &race);

    // relationship to householder
    sscanf(tokens[rel_col], "%d", &relationship);

    // schools 
    if(strcmp(tokens[school_col], "-1")) {
      sprintf(school_label, "S%s", tokens[school_col]);
    }
    
  }

  // set pointer to primary places in init data object
  house = Global::Places.get_place_from_label(house_label);
  work =  Global::Places.get_place_from_label(work_label);
  school = Global::Places.get_place_from_label(school_label);

  if(house == NULL) {
    // we need at least a household (homeless people not yet supported), so
    // skip this person
    FRED_VERBOSE(0, "WARNING: skipping person %s -- %s %s\n", label,
		 "no household found for label =", house_label);
  }
  FRED_VERBOSE(1, "person %s -- house_label %s\n", label, house_label);

  // warn if we can't find workplace
  if(strcmp(work_label, "-1") != 0 && work == NULL) {
    FRED_VERBOSE(2, "WARNING: person %s -- no workplace found for label = %s\n", label,
		 work_label);
    if(Global::Enable_Local_Workplace_Assignment) {
      work = Global::Places.get_random_workplace();
      FRED_CONDITIONAL_VERBOSE(0, work != NULL, "WARNING: person %s assigned to workplace %s\n",
			       label, work->get_label());
      FRED_CONDITIONAL_VERBOSE(0, work == NULL,
			       "WARNING: no workplace available for person %s\n", label);
    }
  }

  // warn if we can't find school.
  FRED_CONDITIONAL_VERBOSE(0, (strcmp(school_label,"-1") != 0 && school == NULL),
			   "WARNING: person %s -- no school found for label = %s\n", label, school_label);

  add_person_to_population(age, sex, race, relationship, house,
			   school, work, day, today_is_birthday);
}


void Population::parse_lines_from_stream(std::istream &stream, bool is_group_quarters_pop) {

  // flag for 2010_ver1 format
  bool is_2010_ver1_format = false;

  int n = 0;
  while(stream.good()) {
    char line[FRED_STRING_SIZE];
    stream.getline(line, FRED_STRING_SIZE);

    // check for 2010_ver1 format
    if(strncmp(line, "sp_id", 5) == 0) {
      is_2010_ver1_format = true;
      continue;
    }

    // skip empty lines...
    if((line[0] == '\0') || strncmp(line, "p_id", 4) == 0 || strncmp(line, "sp_id", 5) == 0) {
      continue;
    }

    get_person_data(line, is_group_quarters_pop);
    n++;
  } // <----- end while loop over stream
  FRED_VERBOSE(0, "end of stream, persons = %d\n", n);
  return;
}


void Population::split_synthetic_populations_by_deme() {
  using namespace std;
  using namespace Utils;
  const char delim = ' ';

  FRED_STATUS(0, "Validating synthetic population identifiers before reading.\n", "");
  const char* pop_dir = Global::Synthetic_population_directory;
  FRED_STATUS(0, "Using population directory: %s\n", pop_dir);
  FRED_STATUS(0, "FRED defines a \"deme\" to be a local population %s\n%s%s\n",
	      "of people whose households are contained in the same bounded geographic region.",
	      "No Synthetic Population ID may have more than one Deme ID, but a Deme ID may ",
	      "contain many Synthetic Population IDs.");

  int param_num_demes = 1;
  Params::get_param_from_string("num_demes", &param_num_demes);
  assert(param_num_demes > 0);
  this->demes.clear();

  std::set<std::string> pop_id_set;

  for(int d = 0; d < param_num_demes; ++d) {
    // allow up to 195 (county) population ids per deme
    // TODO set this limit in param file / Global.h.
    char pop_id_string[4096];
    std::stringstream ss;
    ss << "synthetic_population_id[" << d << "]";

    if(Params::does_param_exist(ss.str())) {
      Params::get_indexed_param("synthetic_population_id", d, pop_id_string);
    } else {
      if(d == 0) {
        strcpy(pop_id_string, Global::Synthetic_population_id);
      } else {
        Utils::fred_abort("Help! %d %s %d %s %d %s\n", param_num_demes,
			  "demes were requested (param_num_demes = ", param_num_demes,
			  "), but indexed paramater synthetic_population_id[", d, "] was not found!");
      }
    }
    this->demes.push_back(split_by_delim(pop_id_string, delim));
    FRED_STATUS(0, "Split ID string \"%s\" into %d %s using delimiter: \'%c\'\n", pop_id_string,
		int(this->demes[d].size()), this->demes[d].size() > 1 ? "tokens" : "token", delim);
    assert(this->demes.size() > 0);
    // only allow up to 255 demes
    assert(this->demes.size() <= std::numeric_limits<unsigned char>::max());
    FRED_STATUS(0, "Deme %d comprises %d synthetic population id%s:\n", d, this->demes[d].size(),
		this->demes[d].size() > 1 ? "s" : "");
    assert(this->demes[d].size() > 0);
    for(int i = 0; i < this->demes[d].size(); ++i) {
      FRED_CONDITIONAL_WARNING(pop_id_set.find(this->demes[d][i]) != pop_id_set.end(),
			       "%s %s %s %s\n", "Population ID ", this->demes[d][i], "was specified more than once!",
			       "Population IDs must be unique across all Demes!");
      assert(pop_id_set.find(this->demes[d][i]) == pop_id_set.end());
      pop_id_set.insert(this->demes[d][i]);
      FRED_STATUS(0, "--> Deme %d, Synth. Pop. ID %d: %s\n", d, i, this->demes[d][i]);
    }
  }
}

void Population::read_all_populations() {
  using namespace std;
  using namespace Utils;
  const char* pop_dir = Global::Synthetic_population_directory;
  assert(this->demes.size() > 0);
  for(int d = 0; d < this->demes.size(); ++d) {
    FRED_STATUS(0, "Loading population for Deme %d:\n", d);
    assert(this->demes[d].size() > 0);
    for(int i = 0; i < this->demes[d].size(); ++i) {
      FRED_STATUS(0, "Loading population %d for Deme %d: %s\n", i, d, this->demes[d][i]);
      // o---------------------------------------- Call read_population to actually
      // |                                         read the population files
      // V
      read_population(pop_dir, this->demes[d][i], "people");
      if(Global::Enable_Group_Quarters) {
        FRED_STATUS(0, "Loading group quarters population %d for Deme %d: %s\n", i, d,
		    this->demes[d][i]);
        // o---------------------------------------- Call read_population to actually
        // |                                         read the population files
        // V
        read_population(pop_dir, this->demes[d][i], "gq_people");
      }
    }
  }
  // report on time take to read populations
  Utils::fred_print_lap_time("reading populations");

}

void Population::read_population(const char* pop_dir, const char* pop_id, const char* pop_type) {

  FRED_STATUS(0, "read population entered\n");

  char cmd[1024];
  char line[1024];
  char* pop_file;
  char population_file[1024];
  char temp_file[256];
  if(getenv("SCRATCH_RAMDISK") != NULL) {
    sprintf(temp_file, "%s/temp_file-%d-%d",
	    getenv("SCRATCH_RAMDISK"),
	    (int)getpid(),
	    Global::Simulation_run_number);
  } else {
    sprintf(temp_file, "./temp_file-%d-%d",
	    (int)getpid(),
	    Global::Simulation_run_number);
  }

  bool is_group_quarters_pop = strcmp(pop_type, "gq_people") == 0 ? true : false;
  FILE* fp = NULL;

#if SNAPPY

  // try to open compressed population file
  sprintf(population_file, "%s/%s/%s_synth_%s.txt.fsz", pop_dir, pop_id, pop_id, pop_type);
  
  fp = Utils::fred_open_file(population_file);
  // fclose(fp); fp = NULL; // TEST
  if(fp != NULL) {
    fclose(fp);
    if(this->enable_copy_files) {
      std::ifstream  src(population_file, std::ios::binary);
      std::ofstream  dst(temp_file,   std::ios::binary);
      dst << src.rdbuf();
      pop_file = temp_file;
    } else {
      pop_file = population_file;
    }

    FRED_VERBOSE(0, "calling SnappyFileCompression on pop_file = %s\n", pop_file);
    SnappyFileCompression compressor = SnappyFileCompression(pop_file);
    compressor.init_compressed_block_reader();
    // if we have the magic, then it must be fsz block compressed
    if(compressor.check_magic_bytes()) {
      // limit to two threads to prevent locking and I/O overhead; also
      // helps to preserve population order in bloque assignment
#pragma omp parallel
      {
        std::stringstream stream;
        while(compressor.load_next_block_stream(stream)) {
          parse_lines_from_stream(stream, is_group_quarters_pop);
        }
      }
    }
    if(this->enable_copy_files) {
      unlink(temp_file);
    }
    FRED_VERBOSE(0, "finished reading compressed population, pop_size = %d\n", this->pop_size);
    return;
  }
#endif

  // try to read the uncompressed file
  sprintf(population_file, "%s/%s/%s_synth_%s.txt", pop_dir, pop_id, pop_id, pop_type);
  fp = Utils::fred_open_file(population_file);
  if(fp == NULL) {
    Utils::fred_abort("population_file %s not found\n", population_file);
  }
  fclose(fp);
  if(this->enable_copy_files) {
    std::ifstream  src(population_file, std::ios::binary);
    std::ofstream  dst(temp_file,   std::ios::binary);
    dst << src.rdbuf();
    pop_file = temp_file;
  } else {
    pop_file = population_file;
  }
  std::ifstream stream(pop_file, ifstream::in);
  parse_lines_from_stream(stream, is_group_quarters_pop);
  if(this->enable_copy_files) {
    unlink(temp_file);
  }
  FRED_VERBOSE(0, "finished reading uncompressed population, pop_size = %d\n", this->pop_size);
}

void Population::remove_dead_from_population(int day) {
  size_t deaths = this->death_list.size();
  for(size_t i = 0; i < deaths; ++i) {
    Person* person = this->death_list[i];
    delete_person_from_population(day, person);
  }
  // clear the death list
  this->death_list.clear();
}

void Population::remove_migrants_from_population(int day) {
  size_t migrants = this->migrant_list.size();
  for(size_t i = 0; i < migrants; ++i) {
    Person* person = this->migrant_list[i];
    delete_person_from_population(day, person);
  }
  // clear the migrant list
  this->migrant_list.clear();
}

void Population::delete_person_from_population(int day, Person* person) {
  FRED_VERBOSE(1, "DELETING PERSON: %d ...\n", person->get_id());

  // remove from vaccine queues
  if(this->vacc_manager->do_vaccination()) {
    FRED_DEBUG(1, "Removing %d from Vaccine Queue\n", person->get_id());
    this->vacc_manager->remove_from_queue(person);
  }

  person->terminate(day);
  FRED_VERBOSE(1, "DELETING PERSON: %d\n", person->get_id());

  // delete from population data structure
  int idx = person->get_pop_index();

  if (this->pop_size > 1) {
    // move last person in vector to this person's position
    this->people[idx] = this->people[this->pop_size-1];

    // inform the move person of its new index
    this->people[idx]->set_pop_index(idx);
  }

  // remove last element in vector
  this->people.pop_back();

  // record new population_size
  this->pop_size = this->people.size();

  // call Person's destructor directly!!!
  person->~Person();
}

void Population::report(int day) {

  // give out anti-virals (after today's infections)
  this->av_manager->disseminate(day);

  // Write out the population if the output_population parameter is set.
  // Will write only on the first day of the simulation, on days
  // matching the date pattern in the parameter file, and the on
  // the last day of the simulation
  if(this->output_population > 0) {
    int month;
    int day_of_month;
    sscanf(this->output_population_date_match,"%d-%d", &month, &day_of_month);
    if((day == 0)
       || (month == Date::get_month() && day_of_month == Date::get_day_of_month())) {
      this->write_population_output_file(day);
    }
  }
}

void Population::end_of_run() {
  // Write the population to the output file if the parameter is set
  // Will write only on the first day of the simulation, days matching
  // the date pattern in the parameter file, and the last day of the
  // simulation
  if(this->output_population > 0) {
    this->write_population_output_file(Global::Days);
  }
}

void Population::quality_control() {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "population quality control check\n");
    fflush(Global::Statusfp);
  }

  // check population
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_household() == NULL) {
      fprintf(Global::Statusfp, "Help! Person %d has no home.\n", person->get_id());
      person->print(Global::Statusfp, 0);
    }
  }

  if(Global::Verbose > 0) {
    int n0, n5, n18, n50, n65;
    int count[20];
    int total = 0;
    n0 = n5 = n18 = n50 = n65 = 0;
    // age distribution
    for(int c = 0; c < 20; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < this->get_population_size(); ++p) {
      Person* person = get_person(p);
      int a = person->get_age();

      if(a < 5) {
        n0++;
      } else if(a < 18) {
        n5++;
      } else if(a < 50) {
        n18++;
      } else if(a < 65) {
        n50++;
      }else {
        n65++;
      }
      int n = a / 5;
      if(n < 20) {
        count[n]++;
      } else {
        count[19]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nAge distribution: %d people\n", total);
    for(int c = 0; c < 20; ++c) {
      fprintf(Global::Statusfp, "age %2d to %d: %6d (%.2f%%)\n", 5 * c, 5 * (c + 1) - 1, count[c],
	      (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "AGE 0-4: %d %.2f%%\n", n0, (100.0 * n0) / total);
    fprintf(Global::Statusfp, "AGE 5-17: %d %.2f%%\n", n5, (100.0 * n5) / total);
    fprintf(Global::Statusfp, "AGE 18-49: %d %.2f%%\n", n18, (100.0 * n18) / total);
    fprintf(Global::Statusfp, "AGE 50-64: %d %.2f%%\n", n50, (100.0 * n50) / total);
    fprintf(Global::Statusfp, "AGE 65-100: %d %.2f%%\n", n65, (100.0 * n65) / total);
    fprintf(Global::Statusfp, "\n");

    // Print out At Risk distribution
    if (Global::Enable_Vaccination) {
      for(int d = 0; d < Global::Conditions.get_number_of_conditions(); ++d) {
	if(Global::Conditions.get_condition(d)->get_at_risk()->is_empty() == false) {
	  Condition* dis = Global::Conditions.get_condition(d);
	  int rcount[20];
	  for(int c = 0; c < 20; ++c) {
	    rcount[c] = 0;
	  }
	  for(int p = 0; p < this->get_population_size(); ++p) {
	    Person* person = get_person(p);
	    int a = person->get_age();
	    int n = a / 10;
	    if(person->get_health()->is_at_risk(d) == true) {
	      if(n < 20) {
		rcount[n]++;
	      } else {
		rcount[19]++;
	      }
	    }
	  }
	  fprintf(Global::Statusfp, "\n Age Distribution of At Risk for Condition %d: %d people\n", d,
		  total);
	  for(int c = 0; c < 10; ++c) {
	    fprintf(Global::Statusfp, "age %2d to %2d: %6d (%.2f%%)\n", 10 * c, 10 * (c + 1) - 1,
		    rcount[c], (100.0 * rcount[c]) / total);
	  }
	  fprintf(Global::Statusfp, "\n");
	}
      }
    }
  }
  FRED_STATUS(0, "population quality control finished\n");
}

void Population::assign_classrooms() {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign classrooms entered\n");
    fflush(Global::Statusfp);
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_school() != NULL) {
      person->assign_classroom();
    }
  }
  FRED_VERBOSE(0,"assign classrooms finished\n");
}

void Population::assign_offices() {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign offices entered\n");
    fflush(Global::Statusfp);
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_workplace() != NULL) {
      person->assign_office();
    }
  }
  FRED_VERBOSE(0,"assign offices finished\n");
}

void Population::assign_primary_healthcare_facilities() {
  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign primary healthcare entered\n");
    fflush(Global::Statusfp);
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    person->assign_primary_healthcare_facility();

  }
  FRED_VERBOSE(0,"assign primary healthcare finished\n");
}

void Population::get_network_stats(char* directory) {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats entered\n");
    fflush(Global::Statusfp);
  }
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/degree.csv", directory);
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "id,age,deg,h,n,s,c,w,o\n");
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", person->get_id(), person->get_age(), person->get_degree(),
	    person->get_household_size(), person->get_neighborhood_size(), person->get_school_size(),
	    person->get_classroom_size(), person->get_workplace_size(), person->get_office_size());
  }
  fclose(fp);
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats finished\n");
    fflush(Global::Statusfp);
  }
}

void Population::set_school_income_levels() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());
  typedef std::map<School*, int> SchoolMapT;
  typedef std::multimap<int, School*> SchoolMultiMapT;

  SchoolMapT* school_enrollment_map = new SchoolMapT();
  SchoolMapT* school_hh_income_map = new SchoolMapT();
  SchoolMultiMapT* school_income_hh_mm = new SchoolMultiMapT();

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_school() == NULL) {
      continue;
    } else {
      School* schl = static_cast<School*>(person->get_school());
      SchoolMapT::iterator got = school_enrollment_map->find(schl);
      //Try to find the school label
      if(got == school_enrollment_map->end()) {
        //Add the school to the map
        school_enrollment_map->insert(std::pair<School*, int>(schl, 1));
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        std::pair<School*, int> my_insert(schl, student_hh->get_household_income());
        school_hh_income_map->insert(my_insert);
      } else {
        //Update the values
        school_enrollment_map->at(schl) += 1;
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        school_hh_income_map->at(schl) += student_hh->get_household_income();
      }
    }
  }

  int total = static_cast<int>(school_hh_income_map->size());
  int q1 = total / 4;
  int q2 = q1 * 2;
  int q3 = q1 * 3;

  FRED_STATUS(0, "\nMEAN HOUSEHOLD INCOME STATS PER SCHOOL SUMMARY:\n");
  for(SchoolMapT::iterator itr = school_enrollment_map->begin();
      itr != school_enrollment_map->end(); ++itr) {
    double enrollment_total = static_cast<double>((*itr).second);
    School* schl = (*itr).first;
    SchoolMapT::iterator got = school_hh_income_map->find(schl);
    double hh_income_tot = static_cast<double>((*got).second);
    double mean_hh_income = (hh_income_tot / enrollment_total);
    std::pair<double, School*> my_insert(mean_hh_income, schl);
    school_income_hh_mm->insert(my_insert);
  }

  int mean_income_size = static_cast<int>(school_income_hh_mm->size());
  assert(mean_income_size == total);

  int counter = 0;
  for(SchoolMultiMapT::iterator itr = school_income_hh_mm->begin();
      itr != school_income_hh_mm->end(); ++itr) {
    School* schl = (*itr).second;
    assert(schl != NULL);
    SchoolMapT::iterator got_school_income_itr = school_hh_income_map->find(schl);
    assert(got_school_income_itr != school_hh_income_map->end());
    SchoolMapT::iterator got_school_enrollment_itr = school_enrollment_map->find(schl);
    assert(got_school_enrollment_itr != school_enrollment_map->end());
    if(counter < q1) {
      schl->set_income_quartile(Global::Q1);
    } else if(counter < q2) {
      schl->set_income_quartile(Global::Q2);
    } else if(counter < q3) {
      schl->set_income_quartile(Global::Q3);
    } else {
      schl->set_income_quartile(Global::Q4);
    }
    double hh_income_tot = static_cast<double>((*got_school_income_itr).second);
    double enrollment_tot = static_cast<double>((*got_school_enrollment_itr).second);
    double mean_hh_income = (hh_income_tot / enrollment_tot);
    FRED_STATUS(0, "MEAN_HH_INCOME: %s %.2f\n", schl->get_label(),
		(hh_income_tot / enrollment_tot));
    counter++;
  }

  delete school_enrollment_map;
  delete school_hh_income_map;
  delete school_income_hh_mm;
}

void Population::report_mean_hh_income_per_school() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());
  typedef std::map<School*, int> SchoolMapT;

  SchoolMapT* school_enrollment_map = new SchoolMapT();
  SchoolMapT* school_hh_income_map = new SchoolMapT();

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_school() == NULL) {
      continue;
    } else {
      School* schl = static_cast<School*>(person->get_school());
      SchoolMapT::iterator got = school_enrollment_map->find(schl);
      //Try to find the school
      if(got == school_enrollment_map->end()) {
        //Add the school to the map
        school_enrollment_map->insert(std::pair<School*, int>(schl, 1));
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        std::pair<School*, int> my_insert(schl, student_hh->get_household_income());
        school_hh_income_map->insert(my_insert);
      } else {
        //Update the values
        school_enrollment_map->at(schl) += 1;
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        school_hh_income_map->at(schl) += student_hh->get_household_income();
      }
    }
  }

  FRED_STATUS(0, "\nMEAN HOUSEHOLD INCOME STATS PER SCHOOL SUMMARY:\n");
  for(SchoolMapT::iterator itr = school_enrollment_map->begin();
      itr != school_enrollment_map->end(); ++itr) {
    double enrollment_tot = static_cast<double>((*itr).second);
    SchoolMapT::iterator got = school_hh_income_map->find((*itr).first);
    double hh_income_tot = static_cast<double>((*got).second);
    FRED_STATUS(0, "MEAN_HH_INCOME: %s %.2f\n", (*itr).first->get_label(),
		(hh_income_tot / enrollment_tot));
  }

  delete school_enrollment_map;
  delete school_hh_income_map;
}

void Population::report_mean_hh_size_per_school() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());
  typedef std::map<School*, int> SchoolMapT;

  SchoolMapT* school_enrollment_map = new SchoolMapT();
  SchoolMapT* school_hh_size_map = new SchoolMapT();

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_school() == NULL) {
      continue;
    } else {
      School* schl = static_cast<School*>(person->get_school());
      SchoolMapT::iterator got = school_enrollment_map->find(schl);
      //Try to find the school
      if(got == school_enrollment_map->end()) {
        //Add the school to the map
        school_enrollment_map->insert(std::pair<School*, int>(schl, 1));
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        std::pair<School*, int> my_insert(schl, student_hh->get_size());
        school_hh_size_map->insert(my_insert);
      } else {
        //Update the values
        school_enrollment_map->at(schl) += 1;
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        school_hh_size_map->at(schl) += student_hh->get_size();
      }
    }
  }

  FRED_STATUS(0, "\nMEAN HOUSEHOLD SIZE STATS PER SCHOOL SUMMARY:\n");
  for(SchoolMapT::iterator itr = school_enrollment_map->begin();
      itr != school_enrollment_map->end(); ++itr) {
    double enrollmen_tot = static_cast<double>((*itr).second);
    SchoolMapT::iterator got = school_hh_size_map->find((*itr).first);
    double hh_size_tot = (double)got->second;
    FRED_STATUS(0, "MEAN_HH_SIZE: %s %.2f\n", (*itr).first->get_label(), (hh_size_tot / enrollmen_tot));
  }

  delete school_enrollment_map;
  delete school_hh_size_map;

}

void Population::report_mean_hh_distance_from_school() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());

  typedef std::map<School*, int> SchoolMapT;
  typedef std::map<School*, double> SchoolMapDist;

  SchoolMapT* school_enrollment_map = new SchoolMapT();
  SchoolMapDist* school_hh_distance_map = new SchoolMapDist();

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_school() == NULL) {
      continue;
    } else {
      School* schl = static_cast<School*>(person->get_school());
      SchoolMapT::iterator got = school_enrollment_map->find(schl);
      //Try to find the school
      if(got == school_enrollment_map->end()) {
        //Add the school to the map
        school_enrollment_map->insert(std::pair<School*, int>(schl, 1));
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        double distance = Geo::haversine_distance(person->get_school()->get_longitude(),
						  person->get_school()->get_latitude(), student_hh->get_longitude(),
						  student_hh->get_latitude());
        std::pair<School*, int> my_insert(schl, distance);
        school_hh_distance_map->insert(my_insert);
      } else {
        //Update the values
        school_enrollment_map->at(schl) += 1;
        Household* student_hh = static_cast<Household*>(person->get_household());
        if(student_hh == NULL) {
          if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
            student_hh = static_cast<Household*>(person->get_permanent_household());;
          }
        }
        assert(student_hh != NULL);
        double distance = Geo::haversine_distance(person->get_school()->get_longitude(),
						  person->get_school()->get_latitude(), student_hh->get_longitude(),
						  student_hh->get_latitude());
        school_hh_distance_map->at(schl) += distance;
      }
    }
  }

  FRED_STATUS(0, "\nMEAN HOUSEHOLD DISTANCE STATS PER SCHOOL SUMMARY:\n");
  for(SchoolMapT::iterator itr = school_enrollment_map->begin();
      itr != school_enrollment_map->end(); ++itr) {
    double enrollmen_tot = static_cast<double>((*itr).second);
    SchoolMapDist::iterator got = school_hh_distance_map->find((*itr).first);
    double hh_distance_tot = (*got).second;
    FRED_STATUS(0, "MEAN_HH_DISTANCE: %s %.2f\n", (*itr).first->get_label(),
		(hh_distance_tot / enrollmen_tot));
  }

  delete school_enrollment_map;
  delete school_hh_distance_map;
}

void Population::report_mean_hh_stats_per_income_category() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());

  //First sort households into sets based on their income level
  std::set<Household*> household_sets[Household_income_level_code::UNCLASSIFIED + 1];

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_household() == NULL) {
      if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
        int income_level = static_cast<Household*>(person->get_permanent_household())->get_household_income_code();
        household_sets[income_level].insert((Household*)person->get_permanent_household());
        Global::Income_Category_Tracker->add_index(income_level);
      } else {
        continue;
      }
    } else {
      int income_level = static_cast<Household*>(person->get_household())->get_household_income_code();
      household_sets[income_level].insert(static_cast<Household*>(person->get_household()));
      Global::Income_Category_Tracker->add_index(income_level);
    }

    if(person->get_household() == NULL) {
      continue;
    } else {
      int income_level = static_cast<Household*>(person->get_household())->get_household_income_code();
      household_sets[income_level].insert(static_cast<Household*>(person->get_household()));
    }
  }

  for(int i = Household_income_level_code::CAT_I; i < Household_income_level_code::UNCLASSIFIED; ++i) {
    int count_hh = static_cast<int>(household_sets[i].size());
    float hh_income = 0.0;
    int count_people = 0;
    int count_children = 0;

    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_households", count_hh);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_people", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_school_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_workers", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_households", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_people", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_school_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_workers", (int)0);

    for(std::set<Household*>::iterator itr = household_sets[i].begin();
        itr !=household_sets[i].end(); ++itr) {
      count_people += (*itr)->get_size();
      count_children += (*itr)->get_children();

      if((*itr)->is_group_quarters()) {
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_households", (int)1);
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_people", (*itr)->get_size());
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_children", (*itr)->get_children());
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_people", (*itr)->get_size());
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_children", (*itr)->get_children());
      } else {
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_people", (*itr)->get_size());
        Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_children", (*itr)->get_children());
      }

      hh_income += static_cast<float>((*itr)->get_household_income());
      std::vector<Person*> inhab_vec = (*itr)->get_inhabitants();
      for(std::vector<Person*>::iterator inner_itr = inhab_vec.begin();
          inner_itr != inhab_vec.end(); ++inner_itr) {
        if((*inner_itr)->is_child()) {
          if((*inner_itr)->get_school() != NULL) {
            if((*itr)->is_group_quarters()) {
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_school_children", (int)1);
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_school_children", (int)1);
            } else {
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_school_children", (int)1);
            }
          }
        }
        if((*inner_itr)->get_workplace() != NULL) {
          if((*itr)->is_group_quarters()) {
            Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_workers", (int)1);
            Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_workers", (int)1);
          } else {
            Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_workers", (int)1);
          }
        }
      }
    }

    if(count_hh > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "mean_household_income",
							  (hh_income / static_cast<double>(count_hh)));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "mean_household_income", static_cast<double>(0.0));
    }

    //Store size info for later usage
    Household::count_inhabitants_by_household_income_level_map[i] = count_people;
    Household::count_children_by_household_income_level_map[i] = count_children;
  }
}

void Population::report_mean_hh_stats_per_census_tract() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());

  //First sort households into sets based on their census tract and income category
  map<int, std::set<Household*> > household_sets;

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    long int census_tract;
    if(person->get_household() == NULL) {
      if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
        census_tract = static_cast<Household*>(person->get_permanent_household())->get_census_tract_fips();
        Global::Tract_Tracker->add_index(census_tract);
        household_sets[census_tract].insert(static_cast<Household*>(person->get_permanent_household()));
        Household::census_tract_set.insert(census_tract);
      } else {
        continue;
      }
    } else {
      census_tract = static_cast<Household*>(person->get_household())->get_census_tract_fips();
      Global::Tract_Tracker->add_index(census_tract);
      household_sets[census_tract].insert(static_cast<Household*>(person->get_household()));
      Household::census_tract_set.insert(census_tract);
    }
  }

  for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
      census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {

    int count_people_per_census_tract = 0;
    int count_children_per_census_tract = 0;
    int count_hh_per_census_tract = static_cast<int>(household_sets[*census_tract_itr].size());
    float hh_income_per_census_tract = 0.0;

    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_households", count_hh_per_census_tract);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_people", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_school_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_workers", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_households", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_people", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_school_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_workers", (int)0);
    
    for(std::set<Household*>::iterator itr = household_sets[*census_tract_itr].begin();
        itr != household_sets[*census_tract_itr].end(); ++itr) {

      count_people_per_census_tract += (*itr)->get_size();
      count_children_per_census_tract += (*itr)->get_children();
      
      if((*itr)->is_group_quarters()) {
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_households", (int)1);
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_people", (*itr)->get_size());
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_children", (*itr)->get_children());
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_people", (*itr)->get_size());
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_children", (*itr)->get_children());
      } else {
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_people", (*itr)->get_size());
        Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_children", (*itr)->get_children());
      }
      
      hh_income_per_census_tract += static_cast<float>((*itr)->get_household_income());
      std::vector<Person*> inhab_vec = (*itr)->get_inhabitants();
      for(std::vector<Person*>::iterator inner_itr = inhab_vec.begin();
          inner_itr != inhab_vec.end(); ++inner_itr) {
        if((*inner_itr)->is_child()) {
          if((*inner_itr)->get_school() != NULL) {
            if((*itr)->is_group_quarters()) {
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_school_children", (int)1);
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_school_children", (int)1);
            } else {
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_school_children", (int)1);
            }
          }
        }
        if((*inner_itr)->get_workplace() != NULL) {
          if((*itr)->is_group_quarters()) {
            Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_workers", (int)1);
            Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_workers", (int)1);
          } else {
            Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_workers", (int)1);
          }
        }
      }
    }

    if(count_hh_per_census_tract > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "mean_household_income",
						(hh_income_per_census_tract / static_cast<double>(count_hh_per_census_tract)));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "mean_household_income", static_cast<double>(0.0));
    }

    //Store size info for later usage
    Household::count_inhabitants_by_census_tract_map[*census_tract_itr] = count_people_per_census_tract;
    Household::count_children_by_census_tract_map[*census_tract_itr] = count_children_per_census_tract;
  }
}

void Population::report_mean_hh_stats_per_income_category_per_census_tract() {

  assert(Global::Places.is_load_completed());
  assert(Global::Pop.is_load_completed());

  //First sort households into sets based on their census tract and income category
  map<int, map<int, std::set<Household*> > > household_sets;

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    long int census_tract;
    if(person->get_household() == NULL) {
      if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
        census_tract = ((Household*)person->get_permanent_household())->get_census_tract_fips();
        Global::Tract_Tracker->add_index(census_tract);
        int income_level = ((Household*)person->get_permanent_household())->get_household_income_code();
        Global::Income_Category_Tracker->add_index(income_level);
        household_sets[census_tract][income_level].insert((Household*)person->get_permanent_household());
        Household::census_tract_set.insert(census_tract);
      } else {
        continue;
      }
    } else {
      census_tract = ((Household*)person->get_household())->get_census_tract_fips();
      Global::Tract_Tracker->add_index(census_tract);
      int income_level = ((Household*)person->get_household())->get_household_income_code();
      Global::Income_Category_Tracker->add_index(income_level);
      household_sets[census_tract][income_level].insert((Household*)person->get_household());
      Household::census_tract_set.insert(census_tract);
    }
  }

  int count_hh_per_income_cat[Household_income_level_code::UNCLASSIFIED];
  float hh_income_per_income_cat[Household_income_level_code::UNCLASSIFIED];

  //Initialize the Income_Category_Tracker keys
  for(int i = Household_income_level_code::CAT_I; i < Household_income_level_code::UNCLASSIFIED; ++i) {
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_households", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_people", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_school_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_workers", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_households", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_people", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_school_children", (int)0);
    Global::Income_Category_Tracker->set_index_key_pair(i, "number_of_gq_workers", (int)0);
    hh_income_per_income_cat[i] = 0.0;
    count_hh_per_income_cat[i] = 0;
  }

  for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
      census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
    int count_people_per_census_tract = 0;
    int count_children_per_census_tract = 0;
    int count_hh_per_census_tract = 0;
    int count_hh_per_income_cat_per_census_tract = 0;
    float hh_income_per_census_tract = 0.0;

    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_people", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_school_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_workers", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_households", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_people", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_school_children", (int)0);
    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_gq_workers", (int)0);

    for(int i = Household_income_level_code::CAT_I; i < Household_income_level_code::UNCLASSIFIED; ++i) {
      count_hh_per_income_cat_per_census_tract += (int)household_sets[*census_tract_itr][i].size();
      count_hh_per_income_cat[i] += (int)household_sets[*census_tract_itr][i].size();
      count_hh_per_census_tract += (int)household_sets[*census_tract_itr][i].size();
      int count_people_per_income_cat = 0;
      int count_children_per_income_cat = 0;
      char buffer [50];

      //First increment the Income Category Tracker Household key (not census tract stratified)
      Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_households", (int)household_sets[*census_tract_itr][i].size());

      //Per income category
      sprintf(buffer, "%s_number_of_households", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)household_sets[*census_tract_itr][i].size());
      sprintf(buffer, "%s_number_of_people", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_children", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_school_children", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_workers", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_gq_people", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_gq_children", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_gq_school_children", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);
      sprintf(buffer, "%s_number_of_gq_workers", Household::household_income_level_lookup(i));
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, buffer, (int)0);

      for(std::set<Household*>::iterator itr = household_sets[*census_tract_itr][i].begin();
	  itr != household_sets[*census_tract_itr][i].end(); ++itr) {

        count_people_per_income_cat += (*itr)->get_size();
        count_people_per_census_tract += (*itr)->get_size();
        //Store size info for later usage
        Household::count_inhabitants_by_household_income_level_map[i] += (*itr)->get_size();
        Household::count_children_by_household_income_level_map[i] += (*itr)->get_children();
        count_children_per_income_cat += (*itr)->get_children();
        count_children_per_census_tract += (*itr)->get_children();
        hh_income_per_income_cat[i] += (float)(*itr)->get_household_income();
        hh_income_per_census_tract += (float)(*itr)->get_household_income();

        //First, increment the Income Category Tracker Household key (not census tract stratified)
        if((*itr)->is_group_quarters()) {
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_households", (int)1);
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_people", (*itr)->get_size());
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_children", (*itr)->get_children());
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_people", (*itr)->get_size());
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_children", (*itr)->get_children());
        } else {
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_people", (*itr)->get_size());
          Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_children", (*itr)->get_children());
        }

        //Next, increment the Tract tracker keys
        if((*itr)->is_group_quarters()) {
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_households", (int)1);
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_people", (*itr)->get_size());
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_children", (*itr)->get_children());
          sprintf(buffer, "%s_number_of_gq_people", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_size());
          sprintf(buffer, "%s_number_of_gq_children", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_children());
          //Don't forget the total counts
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_people", (*itr)->get_size());
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_children", (*itr)->get_children());
          sprintf(buffer, "%s_number_of_people", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_size());
          sprintf(buffer, "%s_number_of_children", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_children());
        } else {
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_people", (*itr)->get_size());
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_children", (*itr)->get_children());
          sprintf(buffer, "%s_number_of_people", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_size());
          sprintf(buffer, "%s_number_of_children", Household::household_income_level_lookup(i));
          Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (*itr)->get_children());
        }

        std::vector<Person*> inhab_vec = (*itr)->get_inhabitants();
        for(std::vector<Person*>::iterator inner_itr = inhab_vec.begin();
            inner_itr != inhab_vec.end(); ++inner_itr) {
          if((*inner_itr)->is_child()) {
            if((*inner_itr)->get_school() != NULL) {
              if((*itr)->is_group_quarters()) {
                //First, increment the Income Category Tracker Household key (not census tract stratified)
                Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_school_children", (int)1);
                //Next, increment the Tract tracker keys
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_school_children", (int)1);
                sprintf(buffer, "%s_number_of_gq_school_children", Household::household_income_level_lookup(i));
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
                //Don't forget the total counts
                //First, increment the Income Category Tracker Household key (not census tract stratified)
                Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_school_children", (int)1);
                //Next, increment the Tract tracker keys
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_school_children", (int)1);
                sprintf(buffer, "%s_number_of_school_children", Household::household_income_level_lookup(i));
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
              } else {
                //First, increment the Income Category Tracker Household key (not census tract stratified)
                Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_school_children", (int)1);
                //Next, increment the Tract tracker keys
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_school_children", (int)1);
                sprintf(buffer, "%s_number_of_school_children", Household::household_income_level_lookup(i));
                Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
              }
            }
          }
          if((*inner_itr)->get_workplace() != NULL) {
            if((*itr)->is_group_quarters()) {
              //First, increment the Income Category Tracker Household key (not census tract stratified)
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_gq_workers", (int)1);
              //Next, increment the Tract tracker keys
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_gq_workers", (int)1);
              sprintf(buffer, "%s_number_of_gq_workers", Household::household_income_level_lookup(i));
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
              //Don't forget the total counts
              //First, increment the Income Category Tracker Household key (not census tract stratified)
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_workers", (int)1);
              //Next, increment the Tract tracker keys
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_workers", (int)1);
              sprintf(buffer, "%s_number_of_workers", Household::household_income_level_lookup(i));
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
            } else {
              //First, increment the Income Category Tracker Household key (not census tract stratified)
              Global::Income_Category_Tracker->increment_index_key_pair(i, "number_of_workers", (int)1);
              //Next, increment the Tract tracker keys
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, "number_of_workers", (int)1);
              sprintf(buffer, "%s_number_of_workers", Household::household_income_level_lookup(i));
              Global::Tract_Tracker->increment_index_key_pair(*census_tract_itr, buffer, (int)1);
            }
          }
        }
      }
    }

    Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "number_of_households", count_hh_per_census_tract);

    if(count_hh_per_census_tract > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "mean_household_income", (hh_income_per_census_tract / (double)count_hh_per_census_tract));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "mean_household_income", (double)0.0);
    }

    //Store size info for later usage
    Household::count_inhabitants_by_census_tract_map[*census_tract_itr] = count_people_per_census_tract;
    Household::count_children_by_census_tract_map[*census_tract_itr] = count_children_per_census_tract;
  }

  for(int i = Household_income_level_code::CAT_I; i < Household_income_level_code::UNCLASSIFIED; ++i) {
    if(count_hh_per_income_cat > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "mean_household_income", (hh_income_per_income_cat[i] / (double)count_hh_per_income_cat[i]));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "mean_household_income", (double)0.0);
    }
  }
}

void Population::print_age_distribution(char* dir, char* date_string, int run) {
  FILE* fp;
  int count[Demographics::MAX_AGE + 1];
  double pct[Demographics::MAX_AGE + 1];
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/age_dist_%s.%02d", dir, date_string, run);
  printf("print_age_dist entered, filename = %s\n", filename);
  fflush(stdout);
  for(int i = 0; i < 21; ++i) {
    count[i] = 0;
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    if(0 <= age && age <= Demographics::MAX_AGE) {
      count[age]++;
    }

    if(age > Demographics::MAX_AGE) {
      count[Demographics::MAX_AGE]++;
    }
    assert(age >= 0);
  }
  fp = fopen(filename, "w");
  for(int i = 0; i < 21; ++i) {
    pct[i] = 100.0 * count[i] / this->pop_size;
    fprintf(fp, "%d  %d %f\n", i * 5, count[i], pct[i]);
  }
  fclose(fp);
}

Person* Population::select_random_person() {
  int i = Random::draw_random_int(0, get_population_size() - 1);
  return get_person(i);
}

Person* Population::select_random_person_by_age(int min_age, int max_age) {
  Person* person;
  int age;
  if(max_age < min_age) {
    return NULL;
  }
  if(Demographics::MAX_AGE < min_age) {
    return NULL;
  }
  while(1) {
    person = select_random_person();
    age = person->get_age();
    if(min_age <= age && age <= max_age) {
      return person;
    }
  }
  return NULL;
}

void Population::write_population_output_file(int day) {

  //Loop over the whole population and write the output of each Person's to_string to the file
  char population_output_file[FRED_STRING_SIZE];
  sprintf(population_output_file, "%s/%s_%s.txt", Global::Output_directory, this->pop_outfile,
	  Date::get_date_string().c_str());
  FILE* fp = fopen(population_output_file, "w");
  if(fp == NULL) {
    Utils::fred_abort("Help! population_output_file %s not found\n", population_output_file);
  }

  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    fprintf(fp, "%s\n", person->to_string().c_str());
  }
  fflush(fp);
  fclose(fp);
}

void Population::get_age_distribution(int* count_males_by_age, int* count_females_by_age) {
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    count_males_by_age[i] = 0;
    count_females_by_age[i] = 0;
  }
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    if(person->get_sex() == 'F') {
      count_females_by_age[age]++;
    } else {
      count_males_by_age[age]++;
    }
  }
}

void Population::initialize_population_behavior() {
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    person->setup_behavior();
  }
}

void Population::initialize_activities() {
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    person->prepare_activities();
  }
}

void Population::initialize_demographic_dynamics() {
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    person->get_demographics()->initialize_demographic_dynamics(person);
  }
}

void Population::update_health_interventions(int day) {
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    person->update_health_interventions(day);
  }
}

void Population::initialize_health_insurance() {
  for(int p = 0; p < this->get_population_size(); ++p) {
    Person* person = get_person(p);
    set_health_insurance(person);
  }
}


void Population::set_health_insurance(Person* p) {

  //  assert(Global::Places.is_load_completed());
  //
  //  //65 or older will use Medicare
  //  if(p->get_real_age() >= 65.0) {
  //    p->get_health()->set_insurance_type(Insurance_assignment_index::MEDICARE);
  //  } else {
  //    //Get the household of the agent to see if anyone already has insurance
  //    Household* hh = static_cast<Household*>(p->get_household());
  //    if(hh == NULL) {
  //      if(Global::Enable_Hospitals && p->is_hospitalized() && p->get_permanent_household() != NULL) {
  //        hh = static_cast<Household*>(p->get_permanent_household());;
  //      }
  //    }
  //    assert(hh != NULL);
  //
  //    double low_income_threshold = 2.0 * Household::get_min_hh_income();
  //    double hh_income = hh->get_household_income();
  //    if(hh_income <= low_income_threshold && Random::draw_random() < (1.0 - (hh_income / low_income_threshold))) {
  //      p->get_health()->set_insurance_type(Insurance_assignment_index::MEDICAID);
  //    } else {
  //      std::vector<Person*> inhab_vec = hh->get_inhabitants();
  //      for(std::vector<Person*>::iterator itr = inhab_vec.begin();
  //          itr != inhab_vec.end(); ++itr) {
  //        Insurance_assignment_index::e insr = (*itr)->get_health()->get_insurance_type();
  //        if(insr != Insurance_assignment_index::UNSET && insr != Insurance_assignment_index::MEDICARE) {
  //          //Set this agent's insurance to the same one
  //          p->get_health()->set_insurance_type(insr);
  //          return;
  //        }
  //      }
  //
  //      //No one had insurance, so set to insurance from distribution
  //      Insurance_assignment_index::e insr = Health::get_health_insurance_from_distribution();
  //      p->get_health()->set_insurance_type(insr);
  //    }
  //  }

  //If agent already has insurance set (by another household agent), then return
  if(p->get_health()->get_insurance_type() != Insurance_assignment_index::UNSET) {
    return;
  }

  //Get the household of the agent to see if anyone already has insurance
  Household* hh = static_cast<Household*>(p->get_household());
  if(hh == NULL) {
    if(Global::Enable_Hospitals && p->is_hospitalized() && p->get_permanent_household() != NULL) {
      hh = static_cast<Household*>(p->get_permanent_household());;
    }
  }
  assert(hh != NULL);
  std::vector<Person*> inhab_vec = hh->get_inhabitants();
  for(std::vector<Person*>::iterator itr = inhab_vec.begin();
      itr != inhab_vec.end(); ++itr) {
    Insurance_assignment_index::e insr = (*itr)->get_health()->get_insurance_type();
    if(insr != Insurance_assignment_index::UNSET) {
      //Set this agent's insurance to the same one
      p->get_health()->set_insurance_type(insr);
      return;
    }
  }

  //No one had insurance, so set everyone in household to the same insurance
  Insurance_assignment_index::e insr = Health::get_health_insurance_from_distribution();
  for(std::vector<Person*>::iterator itr = inhab_vec.begin();
      itr != inhab_vec.end(); ++itr) {
    (*itr)->get_health()->set_insurance_type(insr);
  }

}
