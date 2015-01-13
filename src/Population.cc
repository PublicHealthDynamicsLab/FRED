/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Population.cc
//

#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <fstream>

#include "Population.h"
#include "Params.h"
#include "Global.h"
#include "Place_List.h"
#include "Household.h"
#include "Disease.h"
#include "Person.h"
#include "Demographics.h"
#include "Manager.h"
#include "AV_Manager.h"
#include "Vaccine_Manager.h"
#include "Age_Map.h"
#include "Random.h"
#include "Date.h"
#include "Travel.h"
#include "Utils.h"
#include "Past_Infection.h"
#include "Evolution.h"
#include "Activities.h"


using namespace std; 

char ** pstring;

// used for reporting
int age_count_male[Demographics::MAX_AGE + 1];
int age_count_female[Demographics::MAX_AGE + 1];
int birth_count[Demographics::MAX_AGE + 1];
int death_count_male[Demographics::MAX_AGE + 1];
int death_count_female[Demographics::MAX_AGE + 1];

char Population::popfile[256];
char Population::pop_outfile[256];
char Population::output_population_date_match[256];
int  Population::output_population = 0;
bool Population::is_initialized = false;
int  Population::next_id = 0;

Population::Population() {

  clear_static_arrays();
  //pop.clear();
  pop_map.clear();
  birthday_map.clear();
  pop_size = 0;
  disease = NULL;
  av_manager = NULL;
  vacc_manager = NULL;
  mutation_prob = NULL;

  for(int i = 0; i < 367; ++i) {
    birthday_vecs[i].clear();
  }
}

void Population::initialize_masks() {
  // can't do this in the constructor because the Global:: variables aren't yet
  // available when the Global::Pop is defined
  blq.add_mask( fred::Infectious );
  blq.add_mask( fred::Susceptible );
  blq.add_mask( fred::Update_Demographics );
  blq.add_mask( fred::Update_Health );
}


// index and id are not the same thing!

Person * Population::get_person_by_index( int _index ) {
  return &( blq.get_item_by_index( _index ) );
}

Person * Population::get_person_by_id( int _id ) {
  return &( blq.get_item_by_index( id_to_index[ _id ] ) ); 
}

Population::~Population() {
  // free all memory
  //for (unsigned i = 0; i < pop.size(); ++i)
  //  delete pop[i];
  //pop.clear();
  pop_map.clear();
  pop_size = 0;
  if(disease != NULL) delete [] disease;
  if(vacc_manager != NULL) delete vacc_manager;
  if(av_manager != NULL) delete av_manager;
  if(mutation_prob != NULL) delete [] mutation_prob;
}

void Population::get_parameters() {
  Params::get_param_from_string("popfile", Population::popfile);

  int num_mutation_params =
    Params::get_param_matrix((char *) "mutation_prob", &mutation_prob);
  if (num_mutation_params != Global::Diseases) {
    fprintf(Global::Statusfp,
        "Improper mutation matrix: expected square matrix of %i rows, found %i",
        Global::Diseases, num_mutation_params);
    exit(1);
  }

  if (Global::Verbose > 1) {
    printf("\nmutation_prob:\n");
    for (int i  = 0; i < Global::Diseases; i++)  {
      for (int j  = 0; j < Global::Diseases; j++) {
        printf("%f ", mutation_prob[i][j]);
      }
      printf("\n");
    }
  }

  //Only do this one time
  if(!Population::is_initialized) {
    Params::get_param_from_string("output_population", &Population::output_population);
    if(Population::output_population > 0) {
      Params::get_param_from_string("pop_outfile", Population::pop_outfile);
      Params::get_param_from_string("output_population_date_match", Population::output_population_date_match);
    }
    Population::is_initialized = true;
  }
}

/*
 * All Persons in the population must have been created using add_person
 */
Person * Population::add_person( int id, int age, char sex, int race, int rel, Place *house,
    Place *school, Place *work, int day, bool today_is_birthday ) {

  int idx = blq.get_free_index();
    
  Person * person = blq.get_free_pointer( idx );

  // mark valid before adding person so that mask operations will be
  // available in the constructor (of Person and all ancillary objects)
  blq.mark_valid_by_index( idx ); 

  new( person ) Person( idx, id, age, sex, race, rel,
        house, school, work, day, today_is_birthday );

  //person->set_pop_index( idx );
  
  assert( id_to_index.find( id ) == id_to_index.end() );
  id_to_index[ id ] = idx;

  assert( (unsigned) pop_size == blq.size() - 1 );

  pop_map[ person ] = pop_size;
  pop_size = blq.size();

  if ( Global::Enable_Aging ) {
    Demographics * demographics = person->get_demographics();
    int pos = demographics->get_birth_day_of_year();
    //Check to see if the day of the year is after FEB 28
    if ( pos > 59 && !Date::is_leap_year( demographics->get_birth_year() ) ) {
      pos++;
    }
    birthday_vecs[ pos ].push_back( person );
    birthday_map[ person ] = ( (int) birthday_vecs[ pos ].size() - 1 );
  }

  return person;
}

void Population::set_mask_by_index( fred::Population_Masks mask, int person_index ) {
  // assert that the mask has in fact been added
  blq.set_mask_by_index( mask, person_index );
}

void Population::clear_mask_by_index( fred::Population_Masks mask, int person_index ) {
   // assert that the mask has in fact been added
  blq.clear_mask_by_index( mask, person_index );
}

void Population::delete_person(Person * person) {
  map<Person *,int>::iterator it;
  Utils::fred_verbose(1,"DELETE PERSON: %d\n", person->get_id());
  it = pop_map.find(person);
  if (it == pop_map.end()) {
    Utils::fred_verbose(0,"Help! person %d deleted, but not in the pop_map\n", person->get_id());
  }
  assert(it != pop_map.end());
  

  person->terminate();
  Utils::fred_verbose(1,"DELETED PERSON: %d\n", person->get_id());
      
  //Travel::terminate_person(person);
  for (int d = 0; d < Global::Diseases; d++) {
    disease[d].get_evolution()->terminate_person(person);
  }
 
  if ( Global::Enable_Travel ) {
    Travel::terminate_person(person);
  }

  assert( id_to_index.find( person->get_id() ) != id_to_index.end() );
  id_to_index.erase( person->get_id() );

  blq.mark_invalid_by_index( person->get_pop_index() ); // <------------ TODO perform check to make sure person and blq[index] refer to the same thing! 

  pop_size--;
  pop_map.erase(it);

  if ((unsigned) pop_size != blq.size()) {
    Utils::fred_verbose(0,"pop_size = %d  blq.size() = %d\n",
        pop_size, (int) blq.size());
  }
  assert( (unsigned) pop_size == blq.size() );

  // graveyard.push_back(person);
}

void Population::prepare_to_die(int day, Person *per) {
  fred::Scoped_Lock lock( mutex );
  // add person to daily death_list
  death_list.push_back(per);
  report_death(day, per);
  // you'll be stone dead in a moment...
  per->die();
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "prepare to die: ");
    per->print(Global::Statusfp,0);
  }
}

void Population::prepare_to_give_birth(int day, Person *per) {
  fred::Scoped_Lock lock( mutex );
  // add person to daily maternity_list
  maternity_list.push_back(per);
  report_birth(day, per);
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp,"prepare to give birth: ");
    per->print(Global::Statusfp,0);
  }
}

void Population::setup() {
  FRED_STATUS(0, "setup population entered\n","");
  
  disease = new Disease [Global::Diseases];
  for (int d = 0; d < Global::Diseases; d++) {
    disease[d].setup(d, this, mutation_prob[d]);
  }

  if ( Global::Enable_Vaccination ) {
    vacc_manager = new Vaccine_Manager(this);
  } else {
    vacc_manager = new Vaccine_Manager();
  }

  if ( Global::Enable_Antivirals ) {
    av_manager = new AV_Manager(this);
  } else {
    av_manager = new AV_Manager();
  }

  if (Global::Verbose > 1) av_manager->print();
  
  //x//pop.clear(); TODO provide clear() method for bloque
  id_to_index.clear();
  pop_map.clear();
  pop_size = 0;
  maternity_list.clear();
  graveyard.clear();
  death_list.clear();
  read_population();

  // empty out the incremental list of Person's who have changed
  incremental_changes.clear();
  never_changed.clear();
  /*
     for (int p = 0; p < pop_size; p++){
     never_changed[pop[p]]=true; // add all agents to this list at start
     }
     */

  if(Global::Verbose > 0){
    int count = 0;
    for(int p = 0; p < pop_size; p++){
      Disease* d = &disease[0];
      if ( blq.get_item_by_index( p ).get_health()->is_immune(d) ) { count++; }
    }
    FRED_STATUS(0, "number of residually immune people = %d\n", count);
  }
  av_manager->reset();
  vacc_manager->reset();

  FRED_STATUS(0, "population setup finished\n","");
}

void Population::read_population() {
  FRED_STATUS(0, "read population entered\n");

  // read in population

  char population_file[256];
  char line[1024];
  char newline[1024];
  bool use_gzip = false;
  FILE *fp = NULL;

  if (strcmp(Global::Synthetic_population_id, "none") == 0) {
    sprintf(population_file, "%s", Population::popfile);
    FILE *fp = Utils::fred_open_file(population_file);
  }
  else {
    sprintf(population_file, "%s/%s/%s_synth_people.txt",
	    Global::Synthetic_population_directory,
	    Global::Synthetic_population_id,
	    Global::Synthetic_population_id);
    fp = Utils::fred_open_file(population_file);
  }

  if (fp == NULL) {
    // try to find the gzipped version
    char population_file_gz[256];
    sprintf(population_file_gz, "%s.gz", population_file);
    if (Utils::fred_open_file(population_file_gz)) {
      char cmd[256];
      use_gzip = true;
      sprintf(cmd, "gunzip -c %s > %s", population_file_gz, population_file);
      system(cmd);
      fp = Utils::fred_open_file(population_file);
    }
    if (fp == NULL) {
      Utils::fred_abort("population_file %s not found\n", population_file);
    }
  }

  // create strings for original individuals
  // pstring = new char* [psize];
  // for (int i = 0; i < psize; i++) pstring[i] = new char[256];

  Population::next_id = 0;
  fgets(line, 255, fp);
  while (fgets(line, 255, fp) != NULL) {
    // printf("line: |%s|\n", line); fflush(stdout); // exit(0);
    int age, race, married, occ, relationship;
    char label[32], house_label[32], school_label[32], work_label[32];
    char sex;

    if (strcmp(Global::Synthetic_population_id, "none") != 0) {
      // parse the line
      char p_id[32];
      char hh_id[32];
      char serialno[32];
      char stcotrbg[32];
      char age_str[4];
      char sex_str[4];
      char race_str[4];
      char sporder[32];
      char relate[4];
      char school_id[32];
      char workplace_id[32];

      Utils::replace_csv_missing_data(newline, line, "-1");
      // printf("new:  %s\n", newline); fflush(stdout); // exit(0);
      strcpy(p_id,strtok(newline,","));
      strcpy(hh_id,strtok(NULL,","));
      strcpy(serialno,strtok(NULL,","));
      strcpy(stcotrbg,strtok(NULL,","));
      strcpy(age_str,strtok(NULL,","));
      strcpy(sex_str,strtok(NULL,","));
      strcpy(race_str,strtok(NULL,","));
      strcpy(sporder,strtok(NULL,","));
      strcpy(relate,strtok(NULL,","));
      strcpy(school_id,strtok(NULL,","));
      strcpy(workplace_id,strtok(NULL,",\n"));

      strcpy(label, p_id);
      if (strcmp(hh_id,"-1")) {	sprintf(house_label, "H%s", hh_id);}
      else { strcpy(house_label,"-1"); }
      if (strcmp(workplace_id,"-1")) { sprintf(work_label, "W%s", workplace_id); }
      else { strcpy(work_label,"-1"); }
      if (strcmp(school_id,"-1")) { sprintf(school_label, "S%s", school_id); }
      else { strcpy(school_label,"-1"); }
      sscanf(relate, "%d", &relationship);
      sscanf(age_str, "%d", &age);
      sex = strcmp(sex_str,"1")==0 ? 'M' : 'F';
      sscanf(race_str, "%d", &race);
      // printf("|%s %d %c %d %s %s %s %d|\n", label, age, sex, race
      //    house_label, work_label, school_label, relationship);
      // fflush(stdout);
      /*
      if (strcmp(work_label,"-1") && strcmp(school_label,"-1")) {
	printf("STUDENT-WORKER: %s %d %c %s %s\n", label, age, sex, work_label, school_label);
	fflush(stdout);
      }
      */
    }
    else {
      // old population file format
      // skip white-space-only lines
      int i = 0;
      while (i < 255 && line[i] != '\0' && isspace(line[i])) i++;
      if (line[i] == '\0') continue;
      
      // skip comment lines
      if (line[0] == '#') continue;
      
      if (sscanf(line, "%s %d %c %d %d %s %s %s %d",
		 label, &age, &sex, &married, &occ,
		 house_label, school_label, work_label, &relationship) != 9) {

	Utils::fred_abort("Help! Bad format in input line when next_id = %d: %s\n", Population::next_id, line);
      }

      // NOTE: the following adjustment reflects the new relationship
      // codes in the version 2 synthetic population.  The HOUSEHOLDER,
      // SPOUSE and CHILD relationships are preserved, but the others
      // are now incorrect.  Use with caution!!
      relationship--;

      // nominal race value -- not present in old input files
      race = 1;
    }
    Place * house = Global::Places.get_place_from_label(house_label);

    FRED_CONDITIONAL_VERBOSE(0, house==NULL,
        "WARNING: skipping person %s in %s --  no household found for label = %s\n",
        label, population_file, house_label);

    if (house == NULL) { continue; }

    Place * work = Global::Places.get_place_from_label( work_label );

    if ( strcmp( work_label, "-1" ) != 0 && work == NULL ) {

      FRED_VERBOSE(0,"WARNING: person %s in %s -- no workplace found for label = %s\n",
            label, population_file, work_label);
      
      if (Global::Enable_Local_Workplace_Assignment) {
        work = Global::Places.get_random_workplace();
            
        FRED_CONDITIONAL_VERBOSE(0, work!=NULL, "WARNING: person %s assigned to workplace %s\n",
                label, work->get_label());
        FRED_CONDITIONAL_VERBOSE(0, work==NULL, "WARNING: no workplace available for person %s\n",
                label);
      }
    }
    Place * school = Global::Places.get_place_from_label(school_label);
    
    FRED_CONDITIONAL_VERBOSE(0,  (strcmp(school_label,"-1")!=0 && school == NULL),
        "WARNING: person %s in %s -- no school found for label = %s\n",
        label, population_file, school_label);

    bool today_is_birthday = false;
    int day = 0;

    add_person(Population::next_id, age, sex, race, relationship,
        house, school, work, day, today_is_birthday);
    
    Population::next_id++;
  }
  fclose(fp);

  if (use_gzip) {
    // remove the uncompressed file
    unlink(population_file);
  }
  // Read past infections
  char past_infections[256];
  int delay;
  Params::get_param((char *)"past_infections", past_infections);
  Params::get_param((char *)"past_infections_delay", &delay);
  if ( strcmp(past_infections, "none") != 0) {
    FILE *pifp = fopen(past_infections, "r");
    int person_id, dis_id, rec_date, age_at_exp, num_strains, strain;
    vector<int> strains;
    // Read header line
    char line[256];
    fscanf(pifp, "%[^\n]s\n", line);
    int linenum = 0;
    while ( !feof(pifp) ) {
      if ( fscanf(pifp, "%d %d %d %d %d ", 
            &person_id, &dis_id, &rec_date, 
            &age_at_exp, &num_strains) != 5) {
        Utils::fred_abort("Help! Read failure for past infections line %d\n", linenum);
      }
        
      rec_date = (rec_date>delay)?0:(rec_date-delay);
      assert(rec_date <= 0);
      strains.clear();
      for(int i=0; i<num_strains; i++){
        fscanf(pifp, "%d", &strain);
        strains.push_back(strain);
      }
      fscanf(pifp, "\n");
      Disease *dis = get_disease( dis_id );
      Person *person = get_person_by_id( person_id ); // <-------------------------------- TODO person id and index not the same thing!

      //Past_Infection *pi = new Past_Infection(strains, rec_date, age_at_exp, dis, person);
      //person->add_past_infection(dis_id, pi);
      person->add_past_infection( strains, rec_date, age_at_exp, dis, person );
      
      linenum++;
    }
    fclose(pifp);
  }

  // select adult to make health decisions
  for (int p = 0; p < pop_size; p++) {
    blq.get_item_by_index( p ).select_adult_decision_maker(NULL); // TODO use blq.apply for this
  }

  FRED_VERBOSE(0, "finished reading population, pop_size = %d\n", pop_size);

}

void Population::update(int day) {

  // clear lists of births and deaths
  if (Global::Enable_Deaths) death_list.clear();
  if (Global::Enable_Births) maternity_list.clear();

  if (Global::Enable_Aging) {

    //Find out if we are currently in a leap year
    int year = Global::Sim_Start_Date->get_year(day);
    int day_of_year = Date::get_day_of_year(year,
        Global::Sim_Start_Date->get_month(day),
        Global::Sim_Start_Date->get_day_of_month(day));

  int bd_count = 0;
  size_t vec_size = 0;

  printf("day_of_year = [%d]\n", day_of_year);

  bool is_leap = Date::is_leap_year(year);

  if((is_leap && day_of_year == 60) || (!is_leap && day_of_year != 60)) {
    vec_size = this->birthday_vecs[day_of_year].size();
      for (size_t p = 0; p < vec_size; p++) {
        this->birthday_vecs[day_of_year][p]->get_demographics()->birthday(day);
        bd_count++;
      }
  }

  //If we are NOT in a leap year, then we need to do all of the day 60 (feb 29) birthdays on day 61
  if(!is_leap && day_of_year == 61) {
    vec_size = this->birthday_vecs[60].size();
    for (size_t p = 0; p < vec_size; p++) {
    this->birthday_vecs[60][p]->get_demographics()->birthday(day);
    bd_count++;
    }
  }

  printf("birthday count = [%d]\n", bd_count);
  }

  // process queued births and deaths ( these are calculated on the Person's birthday )
  if ( Global::Enable_Births || Global::Enable_Deaths ) {
    update_population_demographics update_functor( day );

    update_functor.update_births = Global::Enable_Births;
    update_functor.update_deaths = Global::Enable_Deaths;
    // only check those people who've been flagged for updates in Demographics
    blq.parallel_masked_apply( fred::Update_Demographics, update_functor ); 
  }
  // Utils::fred_print_wall_time("day %d update_demographics", day);

  if (Global::Enable_Births) {
    // add the births to the population
    size_t births = maternity_list.size();
    for (size_t i = 0; i < births; i++) {
      Person * mother = maternity_list[i];
      Person * baby = mother->give_birth(day);

      if (Global::Enable_Behaviors) {
        // turn mother into an adult decision maker, if not already
        if (mother != mother->get_adult_decision_maker()) {
          Utils::fred_verbose(0, "young mother %d age %d become adult decision maker on day %d\n",
              mother->get_id(), mother->get_age(), day);
          mother->become_an_adult_decision_maker();
        }
        // let mother decide health behaviors for child
        baby->get_behavior()->set_adult_decision_maker(mother);
      }

      if(vacc_manager->do_vaccination()){
        if(Global::Debug > 1)
          fprintf(Global::Statusfp,"Adding %d to Vaccine Queue\n",baby->get_id());
        vacc_manager->add_to_queue(baby);
      }
      int age_lookup = mother->get_age();
      if (age_lookup > Demographics::MAX_AGE)
        age_lookup = Demographics::MAX_AGE;
      birth_count[age_lookup]++;
    }
    if (Global::Verbose > 0) {
      fprintf(Global::Statusfp, "births = %d\n", (int)births);
      fflush(Global::Statusfp);
    }
  }

  if (Global::Enable_Deaths) {
    // remove the dead from the population
    size_t deaths = death_list.size();
    for (size_t i = 0; i < deaths; i++) {
      //For reporting
      int age_lookup = death_list[i]->get_age();
      if (age_lookup > Demographics::MAX_AGE)
        age_lookup = Demographics::MAX_AGE;
      if (death_list[i]->get_sex() == 'F')
        death_count_female[age_lookup]++;
      else
       death_count_male[age_lookup]++;

      if(vacc_manager->do_vaccination()){
       if(Global::Debug > 1)
          fprintf(Global::Statusfp,"Removing %d from Vaccine Queue\n", death_list[i]->get_id());
        vacc_manager->remove_from_queue(death_list[i]);
      }

      //Remove the person from the birthday lists
      if(Global::Enable_Aging) {
        map<Person *, int>::iterator itr;
        itr = birthday_map.find(death_list[i]);
        if (itr == birthday_map.end()) {
          FRED_VERBOSE(0, "Help! person %d deleted, but not in the birthday_map\n",
              death_list[i]->get_id() );
        }
        assert(itr != birthday_map.end());
        int pos = (*itr).second;
        int day_of_year = death_list[i]->get_demographics()->get_birth_day_of_year();

      	//Check to see if the day of the year is after FEB 28
        if(day_of_year > 59 && !Date::is_leap_year(death_list[i]->get_demographics()->get_birth_year()))
          day_of_year++;

        Person * last = this->birthday_vecs[day_of_year].back();
        birthday_map.erase(itr);
        birthday_map[last] = pos;

        this->birthday_vecs[day_of_year][pos] = this->birthday_vecs[day_of_year].back();
        this->birthday_vecs[day_of_year].pop_back();
      }

      delete_person(death_list[i]);
    }
    FRED_STATUS( 0, "deaths = %d\n", (int) deaths );
  }

  // first update everyone's health intervention status
  if ( Global::Enable_Vaccination || Global::Enable_Antivirals ) {
    // update everyone's health vaccination and antiviral status
    for (int p = 0; p < pop_size; p++) {
      blq.get_item_by_index( p ).update_health_interventions(day);
    }
  }

  // update everyone's health status
  update_population_health update_functor( day );
  blq.parallel_masked_apply( fred::Update_Health, update_functor );
  // Utils::fred_print_wall_time("day %d update_health", day);

  if (Global::Enable_Mobility) {
    // update household mobility activity on July 1
    if (Date::match_pattern(Global::Sim_Current_Date, "07-01-*")) {
      for (int p = 0; p < pop_size; p++) {
        blq.get_item_by_index( p ).update_household_mobility();
      }
    }
  }

  // prepare Activities at start up
  if (day == 0) {
    for (int p = 0; p < pop_size; p++) {
      blq.get_item_by_index( p ).prepare_activities();
    }
    Activities::before_run();
  }

  // update activity profiles on July 1
  if (Global::Enable_Aging && Date::match_pattern(Global::Sim_Current_Date, "07-01-*")) {
    for (int p = 0; p < pop_size; p++) {
      blq.get_item_by_index( p ).update_activity_profile();
    }
  }

  // update travel decisions
  Travel::update_travel(day);
  // Utils::fred_print_wall_time("day %d update_travel", day);

  // update decisions about behaviors
  for (int p = 0; p < pop_size; p++) {
    blq.get_item_by_index( p ).update_behavior(day);
  }
  // Utils::fred_print_wall_time("day %d update_behavior", day);

  // distribute vaccines
  vacc_manager->update(day);
  // Utils::fred_print_wall_time("day %d vacc_manager", day);

  // distribute AVs
  av_manager->update(day);
  // Utils::fred_print_wall_time("day %d av_manager", day);

  FRED_STATUS( 1, "population begin_day finished\n");
}

void Population::update_population_demographics::operator() ( Person & p ) {
  // default Demographics::update currently empty
  //p.demographics.update( day );
  if ( update_births ) {
    p.demographics.update_births( day );
  }
  if ( update_deaths ) {
    p.demographics.update_deaths( day );
  }
}

void Population::update_population_health::operator() ( Person & p ) {
  p.update_health( day );
}

void Population::report(int day) {

  // give out anti-virals (after today's infections)
  av_manager->disseminate(day);

  if (Global::Verbose > 0 && Date::match_pattern(Global::Sim_Current_Date, "12-31-*")) {
    // print the statistics on December 31 of each year
    for (int i = 0; i < pop_size; ++i) {
      Person & pop_i = blq.get_item_by_index( i );
      int age_lookup = pop_i.get_age();

      if (age_lookup > Demographics::MAX_AGE)
        age_lookup = Demographics::MAX_AGE;
      if (pop_i.get_sex() == 'F')
        age_count_female[age_lookup]++;
      else
        age_count_male[age_lookup]++;
    }
    for (int i = 0; i <= Demographics::MAX_AGE; ++i) {
      int count, num_deaths, num_births;
      double birthrate, deathrate;
      fprintf(Global::Statusfp,
         "DEMOGRAPHICS Year %d TotalPop %d Age %d ", 
          Global::Sim_Current_Date->get_year(), pop_size, i);
      count = age_count_female[i];
      num_births = birth_count[i];
      num_deaths = death_count_female[i];
      birthrate = count>0 ? ((100.0*num_births)/count) : 0.0;
      deathrate = count>0 ? ((100.0*num_deaths)/count) : 0.0;
      fprintf(Global::Statusfp,
          "count_f %d births_f %d birthrate_f %.03f deaths_f %d deathrate_f %.03f ",
          count, num_births, birthrate, num_deaths, deathrate);
      count = age_count_male[i];
      num_deaths = death_count_male[i];
      deathrate = count?((100.0*num_deaths)/count):0.0;
      fprintf(Global::Statusfp,
          "count_m %d deaths_m %d deathrate_m %.03f\n",
          count, num_deaths, deathrate);
      fflush(Global::Statusfp);
    }
    clear_static_arrays();
  }

  for (int d = 0; d < Global::Diseases; d++) {
    disease[d].print_stats(day);
  }

  // Write out the population if the output_population parameter is set.
  // Will write only on the first day of the simulation, on days
  // matching the date pattern in the parameter file, and the on
  // the last day of the simulation
  if(Population::output_population > 0) {
    if((day == 0) || (Date::match_pattern(Global::Sim_Current_Date, Population::output_population_date_match))) {
      this->write_population_output_file(day);
    }
  }
}

void Population::print(int incremental, int day) {
  if (Global::Tracefp == NULL) return;

  if (!incremental){
    if (Global::Trace_Headers) fprintf(Global::Tracefp, "# All agents, by ID\n");
    for (int p = 0; p < pop_size; p++) {
      for (int i=0; i<Global::Diseases; i++) {
        Person & pop_i = blq.get_item_by_index( p );
        pop_i.print(Global::Tracefp, i);
      }
    }

  } else if (1==incremental) {
    ChangeMap::const_iterator iter;

    if (Global::Trace_Headers){
      if (day < Global::Days)
        fprintf(Global::Tracefp, "# Incremental Changes (every %d): Day %3d\n", Global::Incremental_Trace, day);
      else
        fprintf(Global::Tracefp, "# End-of-simulation: Remaining unreported changes\n");

      if (! incremental_changes.size()){
        fprintf(Global::Tracefp, "# <LIST-EMPTY>\n");
        return;
      }
    }

    for (iter = this->incremental_changes.begin();
        iter != this->incremental_changes.end();
        iter++){
      (iter->first)->print(Global::Tracefp, 0); // the map key is a Person*
    }
  } else {
    ChangeMap::const_iterator iter;
    if (Global::Trace_Headers){
      fprintf(Global::Tracefp, "# Agents that never changed\n");
      if (! never_changed.size()){
        fprintf(Global::Tracefp, "# <LIST-EMPTY>\n");
        return;
      }
    }

    for (iter = this->never_changed.begin();
        iter != this->never_changed.end();
        iter++){
      (iter->first)->print(Global::Tracefp, 0); // the map key is a Person*
    }
  }

  // empty out the incremental list of Person's who have changed
  if (-1 < incremental)
    incremental_changes.clear();
}


void Population::end_of_run() {
  // print out agents who have changes yet unreported
  this->print(1, Global::Days);

  // print out all those agents who never changed
  this->print(-1);

  //Write the population to the output file if the parameter is set
  //  * Will write only on the first day of the simulation, days matching the date pattern in the parameter file,
  //    and the last day of the simulation *
  if(Population::output_population > 0) {
    this->write_population_output_file(Global::Days);
  }
}

Disease *Population::get_disease(int disease_id) {
  return &disease[disease_id];
}

void Population::quality_control() {
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "population quality control check\n");
    fflush(Global::Statusfp);
  }

  // check population
  for (int p = 0; p < pop_size; p++) {
    Person & pop_i = blq.get_item_by_index( p );

    if (pop_i.get_activities()->get_household() == NULL) {
      fprintf(Global::Statusfp, "Help! Person %d has no home.\n",p);
      pop_i.print(Global::Statusfp, 0);

    }
  }

  if (Global::Verbose > 0) {
    int n0, n5, n18, n65;
    int count[20];
    int total = 0;
    n0 = n5 = n18 = n65 = 0;
    // age distribution
    for (int c = 0; c < 20; c++) { count[c] = 0; }
    for (int p = 0; p < pop_size; p++) {
      Person & pop_i = blq.get_item_by_index( p );
      //int a = pop[p]->get_age();
      int a = pop_i.get_age();

      if (a < 5) { n0++; }
      else if (a < 18) { n5++; }
      else if (a < 65) { n18++; }
      else { n65++; }
      int n = a / 5;
      if (n < 20) { count[n]++; }
      else { count[19]++; }
      total++;
    }
    fprintf(Global::Statusfp, "\nAge distribution: %d people\n", total);
    for (int c = 0; c < 20; c++) {
      fprintf(Global::Statusfp, "age %2d to %d: %6d (%.2f%%)\n",
          5*c, 5*(c+1)-1, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "AGE 0-4: %d %.2f%%\n", n0, (100.0*n0)/total);
    fprintf(Global::Statusfp, "AGE 5-17: %d %.2f%%\n", n5, (100.0*n5)/total);
    fprintf(Global::Statusfp, "AGE 18-64: %d %.2f%%\n", n18, (100.0*n18)/total);
    fprintf(Global::Statusfp, "AGE 65-100: %d %.2f%%\n", n65, (100.0*n65)/total);
    fprintf(Global::Statusfp, "\n");

    // Print out At Risk distribution
    for(int d = 0; d < Global::Diseases; d++){
      if(disease[d].get_at_risk()->get_num_ages() > 0){
        Disease* dis = &disease[d];
        int rcount[20];
        for (int c = 0; c < 20; c++) { rcount[c] = 0; }
        for (int p = 0; p < pop_size; p++) {
          Person & pop_i = blq.get_item_by_index( p );
          int a = pop_i.get_age();
          int n = a / 10;
          if(pop_i.get_health()->is_at_risk(dis)==true) {
            if( n < 20 ) { rcount[n]++; }
            else { rcount[19]++; }
          }
        }
        fprintf(Global::Statusfp, "\n Age Distribution of At Risk for Disease %d: %d people\n",d,total);
        for(int c = 0; c < 10; c++ ) {
          fprintf(Global::Statusfp, "age %2d to %2d: %6d (%.2f%%)\n",
              10*c, 10*(c+1)-1, rcount[c], (100.0*rcount[c])/total);
        }
        fprintf(Global::Statusfp, "\n");
      }
    }  
  }


  if (Global::Verbose) {
    fprintf(Global::Statusfp, "population quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Population::set_changed(Person *p){
  incremental_changes[p]=true; // note that this element has been changed
  never_changed.erase(p);      // remove it from this list 
  // (might already be gone, but this won't hurt)
}

//Static function to clear arrays
void Population::clear_static_arrays() {
  for (int i = 0; i <= Demographics::MAX_AGE; ++i) {
    age_count_male[i] = 0;
    age_count_female[i] = 0;
    death_count_male[i] = 0;
    death_count_female[i] = 0;
  }
  for (int i = 0; i <= Demographics::MAX_AGE; ++i) {
    birth_count[i] = 0;
  }
}

int Population::get_next_id() {
  return Population::next_id++;
}

void Population::assign_classrooms() {
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign classrooms entered\n");
    fflush(Global::Statusfp);
  }
  for (int p = 0; p < pop_size; p++){
    blq.get_item_by_index( p ).assign_classroom();
  }
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign classrooms finished\n");
    fflush(Global::Statusfp);
  }
}

void Population::assign_offices() {
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign offices entered\n");
    fflush(Global::Statusfp);
  }
  for (int p = 0; p < pop_size; p++){
    blq.get_item_by_index( p ).assign_office();
  }
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign offices finished\n");
    fflush(Global::Statusfp);
  }
}

void Population::get_network_stats(char *directory) {
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats entered\n");
    fflush(Global::Statusfp);
  }
  char filename[256];
  sprintf(filename, "%s/degree.csv", directory);
  FILE *fp = fopen(filename, "w");
  fprintf(fp, "id,age,deg,h,n,s,c,w,o\n");
  for (int p = 0; p < pop_size; p++){
    Person & pop_i = blq.get_item_by_index( p );
    fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        pop_i.get_id(),
        pop_i.get_age(),
        pop_i.get_degree(),
        pop_i.get_household_size(),
        pop_i.get_neighborhood_size(),
        pop_i.get_school_size(),
        pop_i.get_classroom_size(),
        pop_i.get_workplace_size(),
        pop_i.get_office_size());
  }
  fclose(fp);
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats finished\n");
    fflush(Global::Statusfp);
  }
}

void Population::report_birth(int day, Person *per) const {
  if (Global::Birthfp == NULL) return;
  fprintf(Global::Birthfp, "day %d mother %d age %d\n",
      day,
      per->get_id(),
      per->get_age());
  fflush(Global::Birthfp);
}

void Population::report_death(int day, Person *per) const {
  if (Global::Deathfp == NULL) return;
  fprintf(Global::Deathfp, "day %d person %d age %d\n",
      day,
      per->get_id(),
      per->get_age());
  fflush(Global::Deathfp);
}

char * Population::get_pstring(int id) {
  return pstring[id];
}

void Population::print_age_distribution(char * dir, char * date_string, int run) {
  FILE *fp;
  int count[21];
  double pct[21];
  char filename[256];
  sprintf(filename, "%s/age_dist_%s.%02d", dir, date_string, run);
  printf("print_age_dist entered, filename = %s\n", filename); fflush(stdout);
  for (int i = 0; i < 21; i++) {
    count[i] = 0;
  }
  for (int p = 0; p < pop_size; p++){
    Person & pop_i = blq.get_item_by_index( p );
    int age = pop_i.get_age();
    int bin = age/5;
    if (-1 < bin && bin < 21) count[bin]++;
    if (-1 < bin && bin > 20) count[20]++;
  }
  fp = fopen(filename, "w");
  for (int i = 0; i < 21; i++) {
    pct[i] = 100.0*count[i]/pop_size;
    fprintf(fp, "%d  %d %f\n", i*5, count[i], pct[i]);
  }
  fclose(fp);
}

Person * Population::select_random_person() {
  int i = IRAND(0,pop_size-1);
  return &( blq.get_item_by_index(i) );
}

Person * Population::select_random_person_by_age(int min_age, int max_age) {
  int i = IRAND(0,pop_size-1);
  while (blq.get_item_by_index( i ).get_age() < min_age || blq.get_item_by_index( i ).get_age() > max_age) {
    i = IRAND(0,pop_size-1);
  }
  return &( blq.get_item_by_index(i) );
}

void Population::write_population_output_file(int day) {

  //Loop over the whole population and write the output of each Person's to_string to the file
  char population_output_file[256];
  sprintf(population_output_file, "%s/%s_%s.txt", Global::Output_directory, Population::pop_outfile,
      (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str());
  FILE *fp = fopen(population_output_file, "w");
  if (fp == NULL) {
    Utils::fred_abort("Help! population_output_file %s not found\n", population_output_file);
  }

  //  fprintf(fp, "Population for day %d\n", day);
  //  fprintf(fp, "------------------------------------------------------------------\n");
  for (int p = 0; p < pop_size; ++p) {
    Person & pop_i = blq.get_item_by_index( p );
    fprintf(fp, "%s\n", pop_i.to_string().c_str());
  }
  fflush(fp);
  fclose(fp);
}
