/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: County.cc
//
#include <limits>
#include <unordered_map>

#include "Age_Map.h"
#include "County.h"
#include "Date.h"
#include "Global.h"
#include "Household.h"
#include "Person.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "School.h"
#include "Utils.h"
#include "Workplace.h"


int County::enable_migration_to_target_popsize = 0;
int County::enable_county_to_county_migration = 0;
int County::enable_within_state_school_assignment = 0;
int County::migration_parameters_read = 0;
int County::population_target_parameters_read = 0;
double**** County::migration_rate = NULL;
int*** County::male_migrants = NULL;
int*** County::female_migrants = NULL;
std::vector<int> County::migration_fips;

County::~County() {
}

County::County(int _fips) {
  this->is_initialized = false;
  this->fips = _fips;
  this->tot_current_popsize = 0;
  this->tot_female_popsize = 0;
  this->tot_male_popsize = 0;
  this->population_growth_rate = 0.0;
  this->college_departure_rate = 0.0;
  this->military_departure_rate = 0.0;
  this->prison_departure_rate = 0.0;
  this->youth_home_departure_rate = 0.0;
  this->adult_home_departure_rate = 0.0;
  this->number_of_households = 0;
  this->beds = NULL;
  this->occupants = NULL;
  this->max_beds = -1;
  this->ready_to_move.clear();
  this->households.clear();
  this->migration_households.clear();
  this->mortality_rate_adjustment_weight = 0.0;

  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->female_popsize[i] = 0;
    this->male_popsize[i] = 0;
  }
  County::migration_fips.clear();
}


void County::setup() {

  if(Global::Enable_Population_Dynamics == false) {
    return;
  }

  char mortality_rate_file[FRED_STRING_SIZE];
  char birth_rate_file[FRED_STRING_SIZE];
  char migration_file[FRED_STRING_SIZE];
  char male_target_file[FRED_STRING_SIZE];
  char female_target_file[FRED_STRING_SIZE];
  double birth_rate_multiplier;
  double mortality_rate_multiplier;

  Params::get_param_from_string("population_growth_rate", &(this->population_growth_rate));
  Params::get_param_from_string("mortality_rate_multiplier", &mortality_rate_multiplier);
  Params::get_param_from_string("birth_rate_multiplier", &birth_rate_multiplier);
  Params::get_param_from_string("mortality_rate_adjustment_weight", &(this->mortality_rate_adjustment_weight));
  Params::get_param_from_string("college_departure_rate", &(this->college_departure_rate));
  Params::get_param_from_string("military_departure_rate", &(this->military_departure_rate));
  Params::get_param_from_string("prison_departure_rate", &(this->prison_departure_rate));
  Params::get_param_from_string("youth_home_departure_rate", &(this->youth_home_departure_rate));
  Params::get_param_from_string("adult_home_departure_rate", &(this->adult_home_departure_rate));
  
  // birth, mortality and migration files.
  // look first for a file that is specific to this county, but fall back to
  // default file if the county file is not found.
  char paramstr[FRED_STRING_SIZE];
  Params::disable_abort_on_failure();

  Params::get_param_from_string("enable_migration_to_target_popsize", &(County::enable_migration_to_target_popsize));
  Params::get_param_from_string("enable_county_to_county_migration", &(County::enable_county_to_county_migration));

  sprintf(paramstr, "birth_rate_file_%d", fips);
  strcpy(birth_rate_file, "");
  Params::get_param_from_string(paramstr, birth_rate_file);
  if (strcmp(birth_rate_file,"") == 0) {
    Params::get_param_from_string("birth_rate_file", birth_rate_file);
  }

  sprintf(paramstr, "mortality_rate_file_%d", fips);
  strcpy(mortality_rate_file, "");
  Params::get_param_from_string(paramstr, mortality_rate_file);
  if (strcmp(mortality_rate_file,"") == 0) {
    Params::get_param_from_string("mortality_rate_file", mortality_rate_file);
  }

  Params::get_param_from_string("enable_within_state_school_assignment", &(County::enable_within_state_school_assignment));

  Params::set_abort_on_failure();

  // initialize the birth rate array
  for(int j = 0; j <= Demographics::MAX_AGE; ++j) {
    this->birth_rate[j] = 0.0;
  }

  // read the birth rates
  FILE* fp = NULL;
  fp = Utils::fred_open_file(birth_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "County birth_rate_file %s not found\n", birth_rate_file);
    exit(1);
  }
  int age;
  double rate;
  while(fscanf(fp, "%d %lf", &age, &rate) == 2) {
    if(age >= 0 && age <= Demographics::MAX_AGE) {
      this->birth_rate[age] = birth_rate_multiplier * rate;
    }
  }
  fclose(fp);

  // set up pregnancy rates
  for(int i = 0; i < Demographics::MAX_AGE; ++i) {
    // approx 3/4 of pregnancies deliver at the next age of the mother
    this->pregnancy_rate[i] = 0.25 * this->birth_rate[i] + 0.75 * this->birth_rate[i + 1];
  }
  this->pregnancy_rate[Demographics::MAX_AGE] = 0.0;

  if(Global::Verbose) {
    for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
      fprintf(Global::Statusfp, "BIRTH RATE     for age %d %e\n", i, this->birth_rate[i]);
      fprintf(Global::Statusfp, "PREGNANCY RATE for age %d %e\n", i, this->pregnancy_rate[i]);
    }
  }

  // read mortality rate file
  fp = Utils::fred_open_file(mortality_rate_file);
  if(fp == NULL) {
    fprintf(Global::Statusfp, "County mortality_rate %s not found\n", mortality_rate_file);
    exit(1);
  }
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    int age;
    double female_rate;
    double male_rate;
    if(fscanf(fp, "%d %lf %lf", &age, &female_rate, &male_rate) != 3) {
      Utils::fred_abort("Help! Read failure for age %d\n", i); 
    }
    if(Global::Verbose) {
      fprintf(Global::Statusfp, "MORTALITY RATE for age %d: female: %e male: %e\n", age, female_rate, male_rate);
    }
    this->female_mortality_rate[i] = mortality_rate_multiplier * female_rate;
    this->male_mortality_rate[i] = mortality_rate_multiplier * male_rate;
  }
  fclose(fp);

  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->adjusted_female_mortality_rate[i] = this->female_mortality_rate[i];
    this->adjusted_male_mortality_rate[i] = this->male_mortality_rate[i];
  }

  //read target files

  int state_fips = this->fips / 1000;
  sprintf(male_target_file,
	  "$FRED_HOME/input_files/countries/usa/projections/%d/%d-males.txt",
	  state_fips, this->fips);

  sprintf(female_target_file,
	  "$FRED_HOME/input_files/countries/usa/projections/%d/%d-females.txt",
	  state_fips, this->fips);

  fp = Utils::fred_open_file(male_target_file);
  if(fp == NULL) {
    fprintf(Global::Statusfp, "County male target file %s not found\n", male_target_file);
    exit(1);
  }

  if(fp != NULL) {
    for (int row = 0; row < AGE_GROUPS; row++) {
      int y;
      fscanf(fp, "%d ", &y);
      char other[100];
      fscanf(fp, "%s ", other);
      fscanf(fp, "%s ", other); //throw away first 3 fields
      for (int col = 0; col < TARGET_YEARS; col++ ) {
	int count;
	fscanf(fp, "%d ", &count);
	this->target_males[row][col] = count;
      }
    }
    printf("male targets:\n");fflush(stdout);
    for (int i = 0; i < AGE_GROUPS; i++) {
      for (int j = 0; j < TARGET_YEARS; j++) {
	printf("%d ", this->target_males[i][j]);
      }
      printf("\n");
    }
  }
  fclose(fp);

  fp = Utils::fred_open_file(female_target_file);
  if(fp == NULL) {
    fprintf(Global::Statusfp, "County female target file %s not found\n", female_target_file);
    exit(1);
  }

  if(fp != NULL) {
    for (int row = 0; row < AGE_GROUPS; row++) {
      int y;
      fscanf(fp, "%d ", &y);
      char other[100];
      fscanf(fp, "%s ", other);
      fscanf(fp, "%s ", other); //throw away first 3 fields
      for (int col = 0; col < TARGET_YEARS; col++ ) {
	int count;
	fscanf(fp, "%d ", &count);
	this->target_females[row][col] = count;
      }
    }
    printf("female targets:\n");fflush(stdout);
    for (int i = 0; i < AGE_GROUPS; i++) {
      for (int j = 0; j < TARGET_YEARS; j++) {
	printf("%d ", this->target_females[i][j]);
      }
      printf("\n");
    }
  }
  fclose(fp);

  set_workplace_probabilities();
  set_school_probabilities();

  // reassign student to in-state school, if necessary
  if (County::enable_within_state_school_assignment) {
    int houses = this->households.size();
    for (int i = 0; i < houses; i++) {
      Household* hh = this->households[i];
      int hh_size = hh->get_size();
      for (int j = 0; j < hh_size; j++) {
	Person* person = hh->get_enrollee(j);
	School* school = person->get_school();
	int grade = person->get_age();
	if (school != NULL && grade < GRADES) {
	  int state_fips = this->fips / 1000;
	  int school_state_fips = school->get_state_fips();
	  if (state_fips != school_state_fips) {
	    // transfer student to in-state school
	    School* new_school = select_new_school(grade);
	    person->change_school(new_school);
	    FRED_VERBOSE(0, "TRANSFERRED person %d from school %s in county %d to in-state school %s in county %d  new_size = %d\n",
			 person->get_id(), school->get_label(), school->get_county_fips(),
			 new_school==NULL?"NONE":new_school->get_label(),
			 new_school==NULL?(-1):new_school->get_county_fips(),
			 new_school==NULL?0:new_school->get_size());
	  }
	}
      }
    }
    // now recompute the school distribution based on the transfers
    set_school_probabilities();
  }


  if(Global::Enable_Population_Dynamics) {
    read_migration_parameters();
  }
}


bool County::increment_popsize(Person* person) {
  int age = person->get_age();
  char sex = person->get_sex();
  if(age > Demographics::MAX_AGE) {
    age = Demographics::MAX_AGE;
  }
  if(age >= 0) {
    if(sex == 'F') {
      this->female_popsize[age]++;
      this->tot_female_popsize++;
      this->tot_current_popsize++;
      return true;
    } else if(sex == 'M') {
      this->male_popsize[age]++;
      this->tot_male_popsize++;
      this->tot_current_popsize++;
      return true;
    }
  }
  return false;
}

bool County::decrement_popsize(Person* person) {
  int age = person->get_age();
  char sex = person->get_sex();
  if(age > Demographics::MAX_AGE) {
    age = Demographics::MAX_AGE;
  }
  if(age >= 0) {
    if(sex == 'F') {
      this->female_popsize[age]--;
      this->tot_female_popsize--;
      this->tot_current_popsize--;
      return true;
    } else if(sex == 'M') {
      this->male_popsize[age]--;
      this->tot_male_popsize--;
      this->tot_current_popsize--;
      return true;
    }
  }
  return false;
}

void County::update(int day) {

  FRED_VERBOSE(1, "County UPDATE: FIPS = %d day = %d\n", this->fips, day);

  if(day == 0) {
    this->number_of_households = this->households.size();
    // initialize house data structures
    this->beds = new int[this->number_of_households];
    this->occupants = new int[this->number_of_households];
    
    this->max_beds = -1;
    for(int i = 0; i < this->number_of_households; ++i) {
      Household* h = this->households[i];
      beds[i] = h->get_orig_size();
      if(this->beds[i] > this->max_beds) {
	this->max_beds = this->beds[i];
      }
    }
    // reserve ready_to_move vector:
    // this->ready_to_move.reserve(tot_current_popsize);


    // find nursing_homes
    for(int i = 0; i < this->number_of_households; ++i) {
      Household* hh = this->households[i];
      if (hh->is_nursing_home()) {
	this->nursing_homes.push_back(hh);
      }
    }
    this->number_of_nursing_homes = this->nursing_homes.size();
  }

  if (Date::get_year() < 2010) {
    return;
  }

  if(Date::get_month() == 6 && Date::get_day_of_month() == 28) {

    group_population_by_sex_and_age(1);

    // migrate among counties in this state
    if (County::enable_county_to_county_migration) {
      // prepare to select people to migrate out
      county_to_county_migration();
    }
  }

  if(Date::get_month() == 6 && Date::get_day_of_month() == 30) {
    
    // prepare to select people to migrate out
    group_population_by_sex_and_age(0);
    
    if (County::enable_migration_to_target_popsize) {
      // migration to/from outside state
      migrate_to_target_popsize();
    }

    // try to move households to houses of appropriate size
    update_housing(day);
    
    report();
  }
  FRED_VERBOSE(1, "County UPDATE finished: FIPS = %d day = %d\n", this->fips, day);
}

void County::get_housing_imbalance(int day) {
  get_housing_data();
  int imbalance = 0;
  for (int i = 0; i < this->number_of_households; ++i) {
    // skip group quarters
    if(this->households[i]->is_group_quarters()) {
      continue;
    }
    imbalance += abs(this->beds[i] - this->occupants[i]);
  }
  FRED_VERBOSE(1, "DAY %d HOUSING: houses = %d, imbalance = %d\n", day, this->number_of_households, imbalance);
}

int County::fill_vacancies(int day) {
  // move ready_to_moves into underfilled units
  int moved = 0;
  if(this->ready_to_move.size() > 0) {
    // first focus on the empty units
    for (int newhouse = 0; newhouse < this->number_of_households; ++newhouse) {
      if(this->occupants[newhouse] > 0) {
        continue;
      }
      int vacancies = this->beds[newhouse] - this->occupants[newhouse];
      if(vacancies > 0) {
	Household* houseptr = this->households[newhouse];
	// skip group quarters
	if(houseptr->is_group_quarters()) {
	  continue;
	}
	for(int j = 0; (j < vacancies) && (this->ready_to_move.size() > 0); ++j) {
	  Person* person = this->ready_to_move.back().first;
	  int oldhouse = this->ready_to_move.back().second;
	  Household* ohouseptr = this->households[oldhouse];
	  this->ready_to_move.pop_back();

	  // the following requires that current household is overfilled:
	  // if(ohouseptr->is_group_quarters() || (this->occupants[oldhouse] - this->beds[oldhouse] > 0)) {

	  // move person to new home
	  person->change_household(houseptr);
	  person->get_activities()->unset_in_parents_home();
	  this->occupants[oldhouse]--;
	  this->occupants[newhouse]++;
	  moved++;
	  // }
	}
      }
    }

    // now consider any vacancy
    for(int newhouse = 0; newhouse < this->number_of_households; ++newhouse) {
      int vacancies = beds[newhouse] - this->occupants[newhouse];
      if(vacancies > 0) {
	Household* houseptr = this->households[newhouse];
	// skip group quarters
	if(houseptr->is_group_quarters()) {
	  continue;
	}
	for(int j = 0; (j < vacancies) && (this->ready_to_move.size() > 0); ++j) {
	  Person* person = this->ready_to_move.back().first;
	  int oldhouse = this->ready_to_move.back().second;
	  Household* ohouseptr = this->households[oldhouse];
	  this->ready_to_move.pop_back();

	  // the following requires that current household is overfilled:
	  // if(ohouseptr->is_group_quarters() || (this->occupants[oldhouse] - this->beds[oldhouse] > 0)) {

	  // move person to new home
	  person->change_household(houseptr);
	  person->get_activities()->unset_in_parents_home();
	  this->occupants[oldhouse]--;
	  this->occupants[newhouse]++;
	  moved++;
	  // }
	}
      }
    }
  }
  return moved;
}

void County::update_housing(int day) {

  FRED_VERBOSE(1, "UPDATE_HOUSING: FIPS = %d day = %d houses = %d\n", fips, day, (int) this->households.size());

  get_housing_data();

  get_housing_imbalance(day);

  if (Global::Enable_Group_Quarters) {

    move_college_students_out_of_dorms(day);
    get_housing_imbalance(day);
    
    move_college_students_into_dorms(day);
    get_housing_imbalance(day);
    
    move_military_personnel_out_of_barracks(day);
    get_housing_imbalance(day);
    
    move_military_personnel_into_barracks(day);
    get_housing_imbalance(day);
    
    move_inmates_out_of_prisons(day);
    get_housing_imbalance(day);
    
    move_inmates_into_prisons(day);
    get_housing_imbalance(day);
    
    move_patients_into_nursing_homes(day);
    get_housing_imbalance(day);
    
  }

  move_young_adults(day);
  get_housing_imbalance(day);
    
  move_older_adults(day);
  get_housing_imbalance(day);

  swap_houses(day);
  get_housing_imbalance(day);

  report_household_distributions();
  // Global::Places.report_school_distributions(day);
  FRED_VERBOSE(1, "UPDATE_HOUSING finished: FIPS = %d day = %d houses = %d\n", fips, day, (int) this->households.size());
  return;
}

void County::move_college_students_out_of_dorms(int day) {
  printf("MOVE FORMER COLLEGE RESIDENTS IN fips %d =======================\n", this->fips);
  this->ready_to_move.clear();
  int college = 0;
  // find students ready to move off campus
  int dorms = 0;
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_college_dorm()) {
      dorms++;
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	if(!person->is_college_dorm_resident())  {
	  printf("HELP! PERSON %d DOES NOT LIVE IN COLLEGE DORM %s subtype %c\n",
		 person->get_id(), house->get_label(), house->get_subtype());
	}
	assert(person->is_college_dorm_resident());
	college++;
	// some college students leave each year
	if(Random::draw_random() < this->college_departure_rate) {
          this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  printf("DAY %d READY TO MOVE %d COLLEGE STUDENTS dorms = %d\n", day, (int) ready_to_move.size(), dorms);
  int moved = fill_vacancies(day);
  printf("DAY %d MOVED %d COLLEGE STUDENTS in fipds %d\n",day,moved, this->fips);
  printf("DAY %d COLLEGE COUNT AFTER DEPARTURES %d\n", day, college-moved); fflush(stdout);
  this->ready_to_move.clear();
}

void County::move_college_students_into_dorms(int day) {
  printf("GENERATE NEW COLLEGE RESIDENTS in fips %d =======================\n", this->fips);
  this->ready_to_move.clear();
  int moved = 0;
  int college = 0;

  // find vacant doom rooms
  std::vector<int>dorm_rooms;
  dorm_rooms.clear();
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_college()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for(int j = 0; j < vacancies; ++j) {
        dorm_rooms.push_back(i);
      }
      college += house->get_size();
    }
  }
  int dorm_vacancies = (int)dorm_rooms.size();
  printf("COLLEGE COUNT %d VACANCIES %d\n", college, dorm_vacancies);
  if(dorm_vacancies == 0) {
    FRED_DEBUG(0, "NO COLLEGE VACANCIES FOUND\n");
    return;
  }

  // find students to fill the dorms
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters() == false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) {
        continue;
      }
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	int age = person->get_age();
	if(Global::ADULT_AGE < age && age < 40 && person->get_number_of_children() == 0) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  printf("COLLEGE APPLICANTS %d in fips %d\n", (int)ready_to_move.size(), this->fips);

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(0, "NO COLLEGE APPLICANTS FOUND\n");
    return;
  }

  // shuffle the applicants
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < dorm_vacancies &&this->ready_to_move.size() > 0; ++i) {
    int newhouse = dorm_rooms[i];
    Place* houseptr = this->households[newhouse];
    // printf("VACANT DORM %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = this->households[oldhouse];
    this->ready_to_move.pop_back();

    // move person to new home

    // printf("APPLICANT %d SEX %c AGE %d OLD_HOUSE %s SIZE %d ORIG %d PROFILE %c NEW_HOUSE %s\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile(), houseptr->get_label());

    person->change_household(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d fips %d ACCEPTED %d COLLEGE STUDENTS, CURRENT = %d  MAX = %d\n", 
	 day, this->fips, moved, college+moved, college + dorm_vacancies);
  this->ready_to_move.clear();
}

void County::move_military_personnel_out_of_barracks(int day) {
  printf("MOVE FORMER MILITARY fips %d =======================\n", this->fips);
  this->ready_to_move.clear();
  int military = 0;
  // find military personnel to discharge
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_military_base()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	// printf("PERSON %d LIVES IN MILITARY BARRACKS %s\n", person->get_id(), house->get_label());
	assert(person->is_military_base_resident());
	military++;
	// some military leave each each year
	if(Random::draw_random() < this->military_departure_rate) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  printf("DAY %d READY TO MOVE %d FORMER MILITARY\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("DAY %d RELEASED %d MILITARY, TOTAL NOW %d fips %d\n",day,moved,military-moved, this->fips);
  this->ready_to_move.clear();
}

void County::move_military_personnel_into_barracks(int day) {
  printf("GENERATE NEW MILITARY BASE RESIDENTS ======================= fips %d\n", this->fips);
  this->ready_to_move.clear();
  int moved = 0;
  int military = 0;

  // find unfilled barracks units
  std::vector<int>barracks_units;
  barracks_units.clear();
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_military_base()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; ++j) {
        barracks_units.push_back(i);
      }
      military += house->get_size();
    }
  }
  int barracks_vacancies = (int)barracks_units.size();
  printf("MILITARY VACANCIES fips = %d vacancies = %d total_current troops = %d\n",
	 this->fips, barracks_vacancies, military);
  if(barracks_vacancies == 0) {
    FRED_DEBUG(1, "NO MILITARY VACANCIES FOUND\n");
    return;
  }

  // find recruits to fill the barracks
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      int selected = 0;
      for(int j = 0; j < hsize && selected < (hsize - house->get_orig_size()); ++j) {
	Person* person = house->get_enrollee(j);
	int age = person->get_age();
	if(Global::ADULT_AGE < age && age < 40 && person->get_number_of_children() == 0) {
	  this->ready_to_move.push_back(make_pair(person,i));
	  selected++;
	}
      }
    }
  }
  printf("MILITARY RECRUITS %d\n", (int)ready_to_move.size());

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(1, "NO MILITARY RECRUITS FOUND\n");
    return;
  }

  // shuffle the recruits
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < barracks_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = barracks_units[i];
    Place* houseptr = this->households[newhouse];
    // printf("UNFILLED BARRACKS %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = this->households[oldhouse];
    this->ready_to_move.pop_back();
    // move person to new home
    // printf("RECRUIT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->change_household(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d MILITARY, CURRENT = %d  MAX = %d fips %d\n",
	 day, moved, military + moved, military + barracks_vacancies, this->fips);
  this->ready_to_move.clear();
}

void County::move_inmates_out_of_prisons(int day) {
  printf("RELEASE PRISONERS fips = %d =======================\n", this->fips);
  this->ready_to_move.clear();
  int prisoners = 0;
  // find former prisoners still in jail
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_prison()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	// printf("PERSON %d LIVES IN PRISON %s\n", person->get_id(), house->get_label());
	assert(person->is_prisoner());
	prisoners++;
	// some prisoners get out each year
	if(Random::draw_random() < this->prison_departure_rate) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d FORMER PRISONERS\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  FRED_VERBOSE(0, "DAY %d RELEASED %d PRISONERS, TOTAL NOW %d fips %d\n",
	       day, moved, prisoners - moved, this->fips);
  this->ready_to_move.clear();
}

void County::move_inmates_into_prisons(int day) {
  printf("GENERATE NEW PRISON RESIDENTS ======================= fips %d\n", this->fips);
  this->ready_to_move.clear();
  int moved = 0;
  int prisoners = 0;

  // find unfilled jail_cell units
  std::vector<int>jail_cell_units;
  jail_cell_units.clear();
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_prison()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for(int j = 0; j < vacancies; ++j) {
        jail_cell_units.push_back(i);
      }
      prisoners += house->get_size();
    }
  }
  int jail_cell_vacancies = (int)jail_cell_units.size();
  printf("PRISON VACANCIES %d\n", jail_cell_vacancies);
  if(jail_cell_vacancies == 0) {
    FRED_DEBUG(1, "NO PRISON VACANCIES FOUND\n");
    return;
  }

  // find inmates to fill the jail_cells
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	int age = person->get_age();
	if((Global::ADULT_AGE < age && person->get_number_of_children() == 0) || (age < 50)) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  printf("PRISON POSSIBLE INMATES %d\n", (int)ready_to_move.size());

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(1, "NO INMATES FOUND\n");
    return;
  }

  // shuffle the potential inmates
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into prison
  for(int i = 0; i < jail_cell_vacancies && this->ready_to_move.size() > 0; ++i) {
    int newhouse = jail_cell_units[i];
    Place* houseptr = this->households[newhouse];
    // printf("UNFILLED JAIL_CELL %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = this->households[oldhouse];
    this->ready_to_move.pop_back();
    // move person to new home
    // printf("INMATE %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->change_household(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_VERBOSE(0, "DAY %d ADDED %d PRISONERS, CURRENT = %d  MAX = %d fips %d\n",
	       day, moved, prisoners + moved, prisoners + jail_cell_vacancies, this->fips);
  this->ready_to_move.clear();
}

void County::move_patients_into_nursing_homes(int day) {
  printf("NEW NURSING HOME RESIDENTS ======================= fips %d\n", this->fips);
  this->ready_to_move.clear();
  int moved = 0;
  int nursing_home_residents = 0;
  int beds = 0;

  // find unfilled nursing_home units
  std::vector<int>nursing_home_units;
  nursing_home_units.clear();
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if (house->is_nursing_home()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; ++j) { nursing_home_units.push_back(i); }
      nursing_home_residents += house->get_size();;
      beds += house->get_orig_size();
    }
  }
  int nursing_home_vacancies = (int)nursing_home_units.size();
  printf("NURSING HOME VACANCIES %d\n", nursing_home_vacancies);
  if(nursing_home_vacancies == 0) {
    FRED_DEBUG(1, "DAY %d ADDED %d NURSING HOME PATIENTS, TOTAL NOW %d BEDS = %d\n", day, 0, nursing_home_residents, beds);
    return;
  }

  // find patients to fill the nursing_homes
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) {
        continue;
      }
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_enrollee(j);
	int age = person->get_age();
	if(60 <= age) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("NURSING HOME POSSIBLE PATIENTS %d\n", (int)ready_to_move.size());

  // shuffle the patients
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into nursing_home
  for(int i = 0; i < nursing_home_vacancies && this->ready_to_move.size() > 0; ++i) {
    int newhouse = nursing_home_units[i];
    Place* houseptr = this->households[newhouse];
    // printf("UNFILLED NURSING_HOME UNIT %s ORIG %d SIZE %d\n", houseptr->get_label(),houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = this->households[oldhouse];
    this->ready_to_move.pop_back();
    // move person to new home
    /*
      printf("PATIENT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
      person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
      ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    */
    person->change_household(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_VERBOSE(0, "DAY %d ADDED %d NURSING HOME PATIENTS, CURRENT = %d  MAX = %d fips %d\n",
	       day,moved,nursing_home_residents+moved,beds, this->fips);
  this->ready_to_move.clear();
}

void County::move_young_adults(int day) {
  FRED_VERBOSE(1,"MOVE YOUNG ADULTS ======================= fips %d\n", this->fips);
  this->ready_to_move.clear();

  // According to National Longitudinal Survey of Youth 1997, the about
  // 20% of youths living with their parents will leave the parental
  // home each year.

  // for reporting stats
  int total[32];
  int moved_out[32];
  for (int i = 0; i < 32; i++) {
    total[i] = 0;
    moved_out[i] = 0;
  }

  // find young adults who are ready to move out
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* house = this->households[i];
    int hsize = house->get_size();
    for(int j = 0; j < hsize; ++j) {
      Person* person = house->get_enrollee(j);
      int age = person->get_age();
      if(16 < age && age <= 30) {
	total[age]++;
	if (person->get_activities()->lives_in_parents_home()) {
	  if (hsize == 1) {
	    // parents are already gone
	    person->get_activities()->unset_in_parents_home();
	    moved_out[age]++;
	  }
	  else {
	    // decide whether ready to move
	    if(Random::draw_random() < this->youth_home_departure_rate) {
	      this->ready_to_move.push_back(make_pair(person,i));
	    }
	  }
	}
	else {
	  // already moved out
	  moved_out[age]++;
	}
      }
    }
  }
  FRED_VERBOSE(0,"DAY %d READY TO MOVE young adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  FRED_VERBOSE(0,"MOVED %d YOUNG ADULTS ======================= fips %d\n",moved, this->fips);
  this->ready_to_move.clear();

  int year = Date::get_year();
  if (year % 5 == 0) {
    // report stats on number of youths already moved out
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/moved_out-%d-%d.txt",
	    Global::Simulation_directory,
	    year,
	    Global::Simulation_run_number);
    FILE *fp = fopen(filename,"w");
    assert(fp != NULL);
    for (int i = 17; i < 27; i++) {
      fprintf(fp, "age %d total %d moved %d pct %0.2f\n", i, total[i], moved_out[i],
	      total[i] > 0 ? (100.0*moved_out[i])/total[i] : 0.0);
    }
    fclose(fp);
  }
}




void County::move_older_adults(int day) {
  FRED_VERBOSE(1,"MOVE OLDER ADULTS ======================= fips %d\n", this->fips);
  this->ready_to_move.clear();

  // According to ACS, about 2% of marriages result in divorce each
  // year. We use this same percent to model adults leaving a household.

  if (1) {
    for(int i = 0; i < this->number_of_households; ++i) {
      Household* house = this->households[i];
      // find the oldest person in the house
      int hsize = house->get_size();
      int max_age = -1;
      int pos = -1;
      int adults = 0;
      for(int j = 0; j < hsize; ++j) {
	int age = house->get_enrollee(j)->get_age();
	if(age > max_age) {
	  max_age = age; pos = j;
	}
	if(age > 20) {
	  adults++;
	}
      }
      if(adults > 1) {
	Person* person = house->get_enrollee(pos);
	if(Random::draw_random() < this->adult_home_departure_rate) {
	  this->ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  else {
    // find older adults in overfilled units
    for(int i = 0; i < this->number_of_households; ++i) {
      int excess = this->occupants[i] - this->beds[i];
      if(excess > 0) {
	Household* house = this->households[i];
	// find the oldest person in the house
	int hsize = house->get_size();
	int max_age = -1;
	int pos = -1;
	int adults = 0;
	for(int j = 0; j < hsize; ++j) {
	  int age = house->get_enrollee(j)->get_age();
	  if(age > max_age) {
	    max_age = age; pos = j;
	  }
	  if(age > 20) {
	    adults++;
	  }
	}
	if(adults > 1) {
	  Person* person = house->get_enrollee(pos);
	  if(Random::draw_random() < this->adult_home_departure_rate) {
	    this->ready_to_move.push_back(make_pair(person,i));
	  }
	}
      }
    }
  }

  FRED_VERBOSE(1,"DAY %d READY TO MOVE older adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  FRED_VERBOSE(1,"MOVED %d OLDER ADULTS ======================= fips %d\n",moved, this->fips);
  this->ready_to_move.clear();
}

void County::report_ages(int day, int house_id) {
  Household* house = this->households[house_id];
  FRED_DEBUG(1, "HOUSE %d BEDS %d OCC %d AGES ", house->get_id(), this->beds[house_id], this->occupants[house_id]);
  int hsize = house->get_size();
  for(int j = 0; j < hsize; ++j) {
    int age = house->get_enrollee(j)->get_age();
    FRED_DEBUG(1, "%d ", age);
  }
}


void County::swap_houses(int day) {

  FRED_DEBUG(1, "SWAP HOUSES day = %d =======================\n", day);

  // two-dim array of vectors of imbalanced houses
  HouselistT** houselist;
  houselist = new HouselistT*[13];
  for(int i = 0; i < 13; ++i) {
    houselist[i] = new HouselistT[13];
    for(int j = 0; j < 13; ++j) {
      houselist[i][j].clear();
    }
  }

  for(int i = 0; i < this->number_of_households; ++i) {
    // skip group quarters
    if(this->households[i]->is_group_quarters()) {
      continue;
    }
    int b = this->beds[i];
    if(b > 12) {
      b = 12;
    }
    int occ = this->occupants[i];
    if(occ > 12) {
      occ = 12;
    }
    if(b != occ) {
      houselist[b][occ].push_back(i);
    }
  }

  // find complementary pairs (beds=i,occ=j) and (beds=j,occ=i)
  int n = 0;
  for(int i = 1; i < 10; ++i) {
    for(int j = i + 1; j < 10; ++j) {
      while(houselist[i][j].size() > 0 && houselist[j][i].size() > 0) {
	int hi = houselist[i][j].back();
	houselist[i][j].pop_back();
	int hj = houselist[j][i].back();
	houselist[j][i].pop_back();
	// swap houses hi and hj
	// printf("SWAPPING: "); report_ages(day,hi); report_ages(day,hj); printf("\n");
	Global::Places.swap_houses(hi, hj);
	this->occupants[hi] = i;
	this->occupants[hj] = j;
	n++;
	// printf("AFTER:\n"); report_ages(day,hi); report_ages(day,hj);
      }
    }
  }
  // printf("06-30 DAY swapped %d pairs of households\n",n);

  // free up memory
  for(int i = 0; i < 13; ++i) {
    delete[] houselist[i];
  }
  delete[] houselist;

  return; 

  // refill-vectors
  for(int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      houselist[i][j].clear();
    }
  }
  for(int i = 0; i < this->number_of_households; ++i) {
    int b = this->beds[i];
    if(b > 9) {
      b = 9;
    }
    int occ = this->occupants[i];
    if(occ > 9) {
      occ = 9;
    }
    if(b > 0 && b != occ) {
      houselist[b][occ].push_back(i);
    }
  }

  /*
  // find complementary pairs (beds=B,occ=o) and (beds=b,occ=O) where o<=b and O <= B
  for (int bd = 0; bd < 10; bd++) {
  for (int oc = bd+1; oc < 10; oc++) {
  // this household needs more beds
  while (houselist[bd][oc].size() > 0) {
  int h1 = houselist[bd][oc].back();
  houselist[bd][oc].pop_back();
  int swapper = 0;
  for (int Bd = oc; (swapper == 0) && Bd < 10; Bd++) {
  for (int Oc = bd; (swapper == 0) && 0 <= Oc; Oc--) {
  // this house has enough beds for the first house
  // and needs at most as many beds as are in the first house
  if (houselist[Bd][Oc].size() > 0) {
  int h2 = houselist[Bd][Oc].back();
  houselist[Bd][Oc].pop_back();
  // swap houses h1 and h2
  printf("SWAPPING II: "); report_ages(day,h1); report_ages(day,h2); printf("\n");
  Global::Places.swap_houses(h1, h2);
  occupants[h1] = Oc;
  occupants[h2] = oc;
  // printf("AFTER:\n"); report_ages(day,h1); report_ages(day,h1);
  swapper = 1;
  }
  }
  }
  }
  }
  }
  */
  // get_housing_imbalance(day);

  // NOTE: This can be simplified using swap houses matrix

  for(int i = 0; i < this->number_of_households; ++i) {
    if(this->beds[i] == 0) {
      continue;
    }
    int diff = this->occupants[i] - this->beds[i];
    if(diff < -1 || diff > 1) {
      // take a look at this house
      Household* house = this->households[i];
      FRED_DEBUG(1, "DAY %d PROBLEM HOUSE %d BEDS %d OCC %d AGES ",
		 day, house->get_id(), beds[i], occupants[i]);
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	int age = house->get_enrollee(j)->get_age();
	FRED_DEBUG(1, "%d ", age);
      }
      FRED_DEBUG(1, "\n");
    }
  }

  // make lists of overfilled houses
  vector<int>* overfilled;
  overfilled = new vector<int>[this->max_beds + 1];
  for(int i = 0; i <= this->max_beds; ++i) {
    overfilled[i].clear();
  }
  
  // make lists of underfilled houses
  vector<int>* underfilled;
  underfilled = new vector<int>[this->max_beds + 1];
  for(int i = 0; i <= this->max_beds; ++i) {
    underfilled[i].clear();
  }

  for(int i = 0; i < this->number_of_households; ++i) {
    if(this->beds[i] == 0) {
      continue;
    }
    int diff = this->occupants[i] - this->beds[i];
    if(diff > 0) {
      overfilled[this->beds[i]].push_back(i);
    }
    if (diff < 0) {
      underfilled[this->beds[i]].push_back(i);
    }
  }

  int count[100];
  for(int i = 0; i <= this->max_beds; ++i) {
    for(int j = 0; j <= this->max_beds+1; ++j) {
      count[j] = 0;
    }
    for(int k = 0; k < (int) overfilled[i].size(); ++k) {
      int kk = overfilled[i][k];
      int occ = this->occupants[kk];
      if(occ <= this->max_beds + 1) {
	count[occ]++;
      } else {
	count[this->max_beds+1]++;
      }
    }
    for(int k = 0; k < (int) underfilled[i].size(); ++k) {
      int kk = underfilled[i][k];
      int occ = this->occupants[kk];
      if(occ <= this->max_beds+1) {
	count[occ]++;
      } else {
	count[this->max_beds+1]++;
      }
    }
    FRED_DEBUG(1, "DAY %4d BEDS %2d ", day, i);
    for(int j = 0; j <= this->max_beds+1; ++j) {
      FRED_DEBUG(1, "%3d ", count[j]);
    }
    FRED_DEBUG(1, "\n");
    fflush(stdout);
  }

}


void County::get_housing_data() {
  FRED_VERBOSE(1, "GET_HOUSING_DATA: FIPS = %d\n", this->fips, this->number_of_households);
  for(int i = 0; i < this->number_of_households; ++i) {
    Household* h = this->households[i];
    occupants[i] = h->get_size();
    FRED_VERBOSE(1, "GET_HOUSING_DATA: FIPS = %d i = %d curr = %d \n", this->fips, i, occupants[i]);
  }
  FRED_VERBOSE(1, "GET_HOUSING_DATA finished: FIPS = %d\n", this->fips, this->number_of_households);
}


void County::report_household_distributions() {
  FRED_VERBOSE(1, "report_hoisehold_distributions : FIPS = %d\n", this->fips);

  // get the current year
  int year = Date::get_year();

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < this->number_of_households; ++p) {
      int n = this->households[p]->get_size();
      if(n <= 10) {
	count[n]++;
      } else {
	count[10]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "UPDATE_HOUSING FIPS %d year %d Household curr sizes: total = %d ", fips, year, total);
    for(int c = 0; c <= 10; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) ", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");

    // original size distribution
    int hsize[20];
    total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
      hsize[c] = 0;
    }
    for(int p = 0; p < this->number_of_households; ++p) {
      int n = this->households[p]->get_orig_size();
      int hs = this->households[p]->get_size();
      if(n <= 10) {
	count[n]++;
	hsize[n] += hs;
      } else {
	count[10]++;
	hsize[10] += hs;
      }
      total++;
    }
    fprintf(Global::Statusfp, "UPDATE_HOUSING FIPS %d year %d Household orig sizes: total = %d ", fips, year, total);
    for(int c = 0; c <= 10; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) ", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
    fprintf(Global::Statusfp, "UPDATE_HOUSING FIPS %d year %d Household size comparison: total = %d ", fips, year, total);
    for(int c = 0; c <= 10; c++) {
      fprintf(Global::Statusfp, "%3d: %0.2f ", c, count[c] ? ((double)hsize[c] / (double)count[c]) : 0.0);
    }
    fprintf(Global::Statusfp, "\n");
  }
  FRED_VERBOSE(1, "report_household_distributions finished : FIPS = %d\n", this->fips);
  return;
}

void County::report_county_population() {
  if(Global::Report_County_Demographic_Information) {
    Utils::fred_report("County_Demographic_Information,fips[%d],date[%s]\n", this->get_fips(), Date::get_date_string().c_str());
    Utils::fred_report("County_Demographic_Information,Total,Males,Females\n");
    Utils::fred_report("County_Demographic_Information,%d,%d,%d\n", this->tot_current_popsize, this->tot_male_popsize, this->tot_female_popsize);
    Utils::fred_report("County_Demographic_Information,By Age Groups:\n");
    Utils::fred_report("County_Demographic_Information,Ages,Total,Males,Females\n");
    for(int i = 0; i <= Demographics::MAX_AGE; i += 5) {
      if(i == 5) { //want 0 - 5, then 6 - 10, 11 - 15, 16 - 20, etc.)
        i++;
      }
      int max =  (i == 0 ? i + 5 : (i + 4 > Demographics::MAX_AGE ? Demographics::MAX_AGE : i + 4));
      int males = this->get_current_popsize(i, max, 'M');
      int females = this->get_current_popsize(i, max, 'F');
      Utils::fred_report("County_Demographic_Information,(%d - %d),%d,%d,%d\n", i, max, males + females, males, females);
    }
    Utils::fred_report("County_Demographic_Information\n");
  }
}


// METHODS FOR SELECTING NEW SCHOOLS

void County::set_school_probabilities() {

  FRED_VERBOSE(0, "set_school_probablities for fips %d\n", this->fips);

  // list of schools attended by people in this county
  typedef std::unordered_map<int,int> attendance_map_t;
  typedef attendance_map_t::iterator attendance_map_itr_t;
  typedef std::unordered_map<int,School*> sid_map_t;

  attendance_map_t school_counts[GRADES];
  int total[GRADES];

  sid_map_t sid_to_school;
  sid_to_school.clear();

  for (int g = 0; g < GRADES; g++) {
    schools_attended[g].clear();
    school_probabilities[g].clear();
    school_counts[g].clear();
    total[g] = 0;
  }

  // get number of people in this county attending each school
  // at the start of the simulation
  int houses = this->households.size();
  for (int i = 0; i < houses; i++) {
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      School* school = person->get_school();
      int grade = person->get_age();
      if (school != NULL && grade < GRADES) {

	int state_fips = this->fips / 1000;
	int school_state_fips = school->get_state_fips();

	if (state_fips == school_state_fips) {
	  FRED_VERBOSE(1, "In-state school %s grade %d %d %d county %d %d\n",
		       school->get_label(), grade, school->get_county_fips(),
		       school_state_fips, this->fips, state_fips);
	}
	else {
	  FRED_VERBOSE(1, "Out of state school %s grade %d %d %d county %d %d\n",
		       school->get_label(), grade, school->get_county_fips(),
		       school_state_fips, this->fips, state_fips);
	}

	if (state_fips == school_state_fips || !(County::enable_within_state_school_assignment)) {
	  // add this person to the count for this school
	  // printf("school %s grade %d person %d\n", school->get_label(), grade, person->get_id()); fflush(stdout);
	  int sid = school->get_id();
	  if (school_counts[grade].find(sid) == school_counts[grade].end()) {
	    std::pair<int,int> new_school_count(sid,1);
	    school_counts[grade].insert(new_school_count);
	    if (sid_to_school.find(sid) == sid_to_school.end()) {
	      std::pair<int,School*> new_sid_map(sid,school);
	      sid_to_school.insert(new_sid_map);
	    }
	  }
	  else {
	    school_counts[grade][sid]++;
	  }
	  total[grade]++;
	} // endif
      } // endif school != NULL
    } // foreach housemate
  } // foreach household

  // convert to probabilities
  for (int g = 0; g < GRADES; g++) {
    if (total[g] > 0) {
      for (attendance_map_itr_t itr = school_counts[g].begin(); itr != school_counts[g].end(); ++itr) {
	int sid = itr->first;
	int count = itr->second;
	School* school = sid_to_school[sid];
	schools_attended[g].push_back(school);
	double prob = (double) count / (double) total[g];
	school_probabilities[g].push_back(prob);
	FRED_VERBOSE(1,"school %s fips %d grade %d attended by %d prob %f\n",
		     school->get_label(), school->get_county_fips(), g, count, prob);
      }
    }
  }
}


School* County::select_new_school(int grade) {

  // pick the school with largest vacancy rate in this grade
  School* selected = NULL;
  double max_vrate = 0.0;
  for (int i = 0; i < this->schools_attended[grade].size(); ++i) {
    School* school = this->schools_attended[grade][i];
    double target = school->get_orig_students_in_grade(grade);
    double vrate = (target-school->get_students_in_grade(grade))/target;
    if (vrate > max_vrate) {
      selected = school;
      max_vrate = vrate;
    }
  }
  if (selected != NULL) {
    return selected;
  }

  FRED_VERBOSE(1,"WARNING: NO SCHOOL VACANCIES found on day %d in fips = %d grade = %d schools = %d\n",
	       Global::Simulation_Day, this->fips, grade, (int) (this->schools_attended[grade].size()));

 // pick from the attendance distribution
  double r = Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->school_probabilities[grade].size(); ++i) {
    sum += this->school_probabilities[grade][i];
    if (r < sum) {
      return this->schools_attended[grade][i];
    }
  }
  FRED_VERBOSE(0,"WARNING: NO SCHOOL FOUND on day %d in fips = %d grade = %d schools = %d r = %f sum = %f\n",
	       Global::Simulation_Day, this->fips, grade, (int) (this->school_probabilities[grade].size()), r, sum);
  // assert(r < sum);
  // this person gets to skip school this year. try again next year.
  return NULL;
}


void County::report_school_sizes() {
  char filename[FRED_STRING_SIZE];
  int year = Date::get_year();
  sprintf(filename, "%s/schools-%d-%d-%d.txt",
	  Global::Simulation_directory,
	  this->fips,
	  year,
	  Global::Simulation_run_number);
  FILE *fp = fopen(filename,"w");
  assert(fp != NULL);
  for (int g = 0; g < GRADES; g++) {
    for(int i = 0; i < this->schools_attended[g].size(); ++i) {
      School* school = schools_attended[g][i];
      fprintf(fp, "year %d grade %d school %s curr %d orig %d\n",
	      year, g,
	      school->get_label(),
	      school->get_size(),
	      school->get_orig_size());
    }
  }
  fclose(fp);
}

// METHODS FOR SELECTING NEW WORKPLACES

void County::set_workplace_probabilities() {

  // list of workplaces attended by people in this county
  typedef std::unordered_map<int,int> attendance_map_t;
  typedef std::unordered_map<int,Workplace*> wid_map_t;
  typedef attendance_map_t::iterator attendance_map_itr_t;

  workplaces_attended.clear();
  workplace_probabilities.clear();

  // get number of people in this county attending each workplace
  // at the start of the simulation
  int houses = this->households.size();
  attendance_map_t workplace_counts;
  wid_map_t wid_to_workplace;
  workplace_counts.clear();
  wid_to_workplace.clear();
  int total = 0;
  for (int i = 0; i < houses; i++) {
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      Workplace* workplace = person->get_workplace();
      if (workplace != NULL) {
	int wid = workplace->get_id();
	if (workplace_counts.find(wid) == workplace_counts.end()) {
	  std::pair<int,int> new_workplace_count(wid,1);
	  workplace_counts.insert(new_workplace_count);
	  std::pair<int,Workplace*> new_wid_map(wid,workplace);
	  wid_to_workplace.insert(new_wid_map);
	}
	else {
	  workplace_counts[wid]++;
	}
	total++;
      }
    }
  }
  if (total == 0) {
    return;
  }

  // convert to probabilities
  for (attendance_map_itr_t itr = workplace_counts.begin(); itr != workplace_counts.end(); ++itr) {
    int wid = itr->first;
    int count = itr->second;
    Workplace* workplace = wid_to_workplace[wid];
    workplaces_attended.push_back(workplace);
    double prob = (double) count / (double) total;
    workplace_probabilities.push_back(prob);
    // printf("workplace %s attended by %d prob %f\n", workplace->get_label(), count, prob);
  }

}


Workplace* County::select_new_workplace() {
  double r = Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->workplace_probabilities.size(); ++i) {
    sum += this->workplace_probabilities[i];
    if (r < sum) {
      return this->workplaces_attended[i];
    }
  }
  assert(r < sum);
  return NULL;
}


void County::report_workplace_sizes() {
  char filename[FRED_STRING_SIZE];
  int year = Date::get_year();
  sprintf(filename, "%s/workplaces-%d-%d-%d.txt",
	  Global::Simulation_directory,
	  this->fips,
	  year,
	  Global::Simulation_run_number);
  FILE* fp = fopen(filename,"w");
  assert(fp != NULL);
  for(int i = 0; i < this->workplaces_attended.size(); ++i) {
    Workplace* workplace = workplaces_attended[i];
    fprintf(fp,"year %d workplace %s curr %d orig %d\n",
	    year,
	    workplace->get_label(),
	    workplace->is_group_quarters()?workplace->get_staff_size():workplace->get_size(),
	    workplace->is_group_quarters()?workplace->get_staff_size():workplace->get_orig_size());
  }
  fclose(fp);
}


// MIGRATION METHODS

void County::read_migration_parameters() {

  if (County::migration_parameters_read) {
    return;
  }
  else {
    County::migration_parameters_read = 1;
  }

  FRED_VERBOSE(1, "read_migration_parameters\n");

  char county_migration_file[FRED_STRING_SIZE];
  strcpy(county_migration_file, "none");

  Params::disable_abort_on_failure();
  Params::get_param_from_string("county_migration_file", county_migration_file);
  Params::set_abort_on_failure();

  if (strcmp(county_migration_file,"none")==0) {
    return;
  }

  FILE* fp = Utils::fred_open_file(county_migration_file);

  // read list of fips code for county in the state
  // end list with -1
  fscanf(fp, "counties: ");
  int fips = 0;
  while (fips > -1) {
    fscanf(fp, "%d ", &fips);
    if (fips > -1) {
      County::migration_fips.push_back(fips);
      FRED_VERBOSE(1, "read_migration_parameters: fips = %d\n", fips);
    }
  }

  // create a migration matrix with format
  // migration_rate[sex][age][source][dest]
  int fips_size = County::migration_fips.size();
  County::migration_rate = new double*** [2];
  for (int sex = 0; sex < 2; sex++) {
    County::migration_rate[sex] = new double** [AGE_GROUPS];
    for (int age_group = 0; age_group < AGE_GROUPS; age_group++) {
      County::migration_rate[sex][age_group] = new double* [fips_size];
      for (int source = 0; source < fips_size; source++) {
	County::migration_rate[sex][age_group][source] = new double [fips_size];
	for (int dest = 0; dest < fips_size; dest++) {
	  County::migration_rate[sex][age_group][source][dest] = 0;
	}
      }
    }
  }

  // read migration_rate matrix
  for (int age_group = 0; age_group < AGE_GROUPS; age_group++) {
    for (int sex = 0; sex < 2; sex++) {
      int low, high;
      if (sex==0) {
	if (age_group < AGE_GROUPS-1) {
	  fscanf(fp, "males ages %d to %d: ",&low,&high);
	} else {
	  fscanf(fp, "males ages %d+: ",&low);
	}
      } else {
	if (age_group < AGE_GROUPS-1) {
	  fscanf(fp, "females ages %d to %d: ",&low,&high);
	} else {
	  fscanf(fp, "females ages %d+: ",&low);
	}
      }
      assert(low==5*age_group);
      if (low < 85) {
	assert(high==5*age_group+4);
      }
      for (int source = 0; source < fips_size; source++) {
	for (int dest = 0; dest < fips_size; dest++) {
	  fscanf(fp, "%lf ", &(County::migration_rate[sex][age_group][source][dest]));
	}
      }
    }
  }
  fclose(fp);
  FRED_VERBOSE(0, "read_migration_file finished\n");
}

double County::get_migration_rate(int sex, int age_group, int src, int dst) {
  if (County::migration_fips.size() == 0) {
    return 0;
  }
  
  if (sex < 0 || 1 < sex || age_group < 0 || AGE_GROUPS-1 < age_group) {
    return 0;
  }

  int source = -1;
  int dest = -1;
  for (int i = 0; i < County::migration_fips.size(); i++) {
    if (src == County::migration_fips[i]) {
      source = i;
    }
    if (dst == County::migration_fips[i]) {
      dest = i;
    }
    if (source > -1 && dest > -1) {
      break;
    }
  }
  if (source > -1 && dest > -1) {
    double rate = County::migration_rate[sex][age_group][source][dest];
    if (rate < 0) {
      printf("migration rate sex %d age %d source %d dest %d = %f\n",
	     sex,age_group,source,dest,rate);
      fflush(stdout);
    }
    return rate;
  }
  else {
    return 0;
  }
}

void County::read_population_target_parameters() {

  if (County::population_target_parameters_read) { //only read in once but called for each fips
    return;
  }
  else {
    County::population_target_parameters_read = 1;
  }

  FRED_VERBOSE(0, "read_population_target_parameters\n");
  ///
  char migration_file[FRED_STRING_SIZE];
  strcpy(migration_file, "none"); // if there is no file

  ///
  Params::disable_abort_on_failure();
  Params::get_param_from_string("migration_file", migration_file);
  Params::set_abort_on_failure();

  if (strcmp(migration_file,"none")==0) { //should there be an error??
    FRED_VERBOSE(0, "no migration file\n");
    return;
  }

  // create a migration matrix with format
  // fips size should match county to county migration parameters, which should already have been read, assume are in same (ascending) order in both files
  // error if not?
  int fips_count = County::migration_fips.size();

  // make the male and female arrays
  County::male_migrants = new int** [fips_count];
  County::female_migrants = new int** [fips_count];
  for (int i = 0; i < fips_count; i++) {  //county fips
    County::male_migrants[i] = new int* [fips_count];
    County::female_migrants[i] = new int* [fips_count];
    for (int j = 0; j < TARGET_YEARS; j++) {  //year
      County::male_migrants[i][j] = new int [AGE_GROUPS]; //age category
      County::female_migrants[i][j] = new int [AGE_GROUPS];
    }
  }
  for (int i = 0; i < fips_count; i++) { //initialize to 0
    for (int j = 0; j < TARGET_YEARS; j++) {
      for (int col = 0; col < AGE_GROUPS; col++ ) {
	County::male_migrants[i][j][col] = 0;
	County::female_migrants[i][j][col] = 0;
      }
    }
  }

  // read in the migration file
  FILE* fp = Utils::fred_open_file(migration_file);

  if(fp != NULL) {

    // read list of fips code for county in the file
    // end list with -1
    fscanf(fp, "counties: ");
    int fips = 0;
    int fips_count = 0;
    while (fips > -1) {
      fscanf(fp, "%d ", &fips);
      if (fips > -1) {
	fips_count++;
	FRED_VERBOSE(1, "read_population_target_parameters: fips count = %d\n", fips_count);
      }
    }
    for (int first = 0; first < fips_count; first++) {
      printf("fips index %d ", first);
      for (int row = 0; row < TARGET_YEARS; row++) {
	int y;
	fscanf(fp, "%d ", &y);
	assert(y==2010+row*5);
	for (int col = 0; col < AGE_GROUPS; col++ ) {
	  int count;
	  fscanf(fp, "%d ", &count);
	  County::male_migrants[first][row][col] = count;
	}
      }
      printf("male migrants:\n");fflush(stdout);
      for (int i = 0; i < TARGET_YEARS; i++) {
	printf("%d ", 2010+i*5);
	for (int j = 0; j < AGE_GROUPS; j++) {
	  printf("%d ", County::male_migrants[first][i][j]);
	}
	printf("\n");
      }

      for (int row = 0; row < TARGET_YEARS; row++) {
	int y;
	fscanf(fp, "%d ", &y);
	assert(y==2010+row*5);
	for (int col = 0; col < AGE_GROUPS; col++ ) {
	  int count;
	  fscanf(fp, "%d", &count);
	  County::female_migrants[first][row][col] = count;
	}
      }
      printf("female migrants:\n");fflush(stdout);
      for (int i = 0; i < TARGET_YEARS; i++) {
	printf("%d ", 2010+i*5);
	for (int j = 0; j < AGE_GROUPS; j++) {
	  printf("%d ", County::female_migrants[first][i][j]);
	}
	printf("\n");
      }
      fflush(stdout);

    }
    fclose(fp);
  }
  else {
    printf("no migration file found");
  }

  FRED_VERBOSE(1, "read_population_target_file finished\n");
}

void County::county_to_county_migration() {

  // get the current year
  int year = Date::get_year();
  int day = Global::Simulation_Day;

  if (year < 2010) {
    return;
  }

  FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION entered fips %d year %d\n",
	       this->fips, year);

  int number_of_counties = Global::Places.get_number_of_counties();

  if (number_of_counties < 2) {
    FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION no other county found\n");
    FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION finished for county %d year %d\n", this->fips, year);
    return;
  }

  for (int c = 0; c < number_of_counties; c++) {
    int dest = County::migration_fips[c];
    if (dest == this->fips) {
      continue;
    }
    if (Global::Places.get_county(dest) == NULL) {
      continue;
    }

    FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION year %d from %d to %d\n",
		 year, this->fips, dest);
    int males[AGE_GROUPS];
    int females[AGE_GROUPS];
    for (int a = 0; a < AGE_GROUPS; a++) {
      int lower_age = 5*a;
      int upper_age = lower_age+4;
      if (lower_age == 85) {
	upper_age = Demographics::MAX_AGE;
      }
      // int current_males = get_current_popsize(lower_age, upper_age, 'M');
      // int current_females = get_current_popsize(lower_age, upper_age, 'F');
      int current_males = 0;
      int current_females = 0;
      for (int age = lower_age; age <= upper_age; age++) {
	current_males += this->males_of_age[age].size();
	current_females += this->females_of_age[age].size();
      }
      // FRED_VERBOSE(0, "current males = %d current females = %d\n", current_males, current_females);
      assert(current_males >= 0);
      assert(current_females >= 0);
      males[a] = current_males * get_migration_rate(0, a, this->fips, dest);
      females[a] = current_females * get_migration_rate(1, a, this->fips, dest);
      // FRED_VERBOSE(0, " males[%d] = %d females[%d] = %d\n", a, males[a], a, females[a]);
      assert(males[a] >= 0);
      assert(females[a] >= 0);
      FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION from county %d to county %d age %d, %d males, %d females on day %d year %d\n",
		   this->fips, dest, lower_age, males[a], females[a], day, year);
    }

    // select households that match the out migration targets

    // set up a random shuffle of households
    std::vector<int>shuff;
    // FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION shuffle %d households\n", this->number_of_households);
    shuff.reserve(this->number_of_households);
    shuff.clear();
    for (int i = 0; i < this->number_of_households; i++) {
      shuff[i]=i;
    }
    std::random_shuffle(shuff.begin(), shuff.end());

    int house_count = 0;
    for (int i = 0; i < this->number_of_households; i++) {
      int hnum = shuff[i];
      // see if this household is eligible to migrate
      Household* house = this->households[hnum];
      if (house->is_group_quarters()) {
	continue;
      }
      int hsize = house->get_size();
      if (hsize==0) {
	continue;
      }
      bool ok = true;
      for(int j = 0; ok && (j < hsize); ++j) {
	Person* person = house->get_enrollee(j);
	int age = person->get_age();
	char s = person->get_sex();
	int a = age/5;
	if (a > AGE_GROUPS-1) { a = AGE_GROUPS-1; }
	ok = (s=='M' ? males[a]>0 : females[a]>0);
      }
      if (ok) {
	// migrate this household
	house->set_migration_fips(dest);
	this->migration_households.push_back(hnum);
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION HOUSEHOLD id %d label %s dest %d\n",
		     house->get_id(), house->get_label(),
		     hnum, dest);
	FRED_VERBOSE(1, "MIGRATION HOUSEHOLD %d size %d: ", house->get_id(), hsize);
	for(int j = 0; j < hsize; ++j) {
	  house_count++;
	  Person* person = house->get_enrollee(j);
	  int age = person->get_age();
	  char s = person->get_sex();
	  int a = age/5;
	  if (a > AGE_GROUPS-1) { a = AGE_GROUPS-1; }
	  if (s=='M') {
	    males[a]--;
	  }
	  else {
	    females[a]--;
	  }
	  person->unset_eligible_to_migrate();
	  FRED_VERBOSE(1, "%c %d ", s, age);
	}
	FRED_VERBOSE(1, "\n");
      }
      else {
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION HOUSEHOLD %d not ok\n", house->get_id());
      }
    }

    int stragglers = 0;
    // select random migrants -- do stragglers before doing swap
    for (int a = 0; a < AGE_GROUPS; a++) {
      int lower_age = a*5;
      int upper_age = lower_age+4;
      if (lower_age == 85) {
	upper_age = Demographics::MAX_AGE;
      }
      if (males[a] > 0) {
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION source %d dest %d stragglers lower_age = %d males = %d\n",
		     this->fips, dest, lower_age, males[a]);
	select_migrants(day, males[a], lower_age, upper_age, 'M', dest);
	stragglers += males[a];
	males[a] = 0;
      }
      if (females[a] > 0) {
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION source %d dest %d stragglers lower_age = %d females = %d\n",
		     this->fips, dest, lower_age, females[a]);
	select_migrants(day, females[a], lower_age, upper_age, 'F', dest);
	stragglers += females[a];
	females[a] = 0;
      }
    }
    FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION source %d dest %d total stragglers %d\n",
		 this->fips, dest, stragglers);

  }// end for loop over other counties

  if ( this->fips == County::migration_fips[County::migration_fips.size()-1]){
    FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION fips %d year %d : households identified\n", this->fips, year);
    if (Global::Verbose > 1) {
      for (int i = 0; i < number_of_counties; i++) {
	int source = County::migration_fips[i];
	County* source_county = Global::Places.get_county(source);
	int source_households = source_county->get_number_of_households();
	for (int j=0; j < source_households; j++) {
	  FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION source fips = %d household id = %d dest = %d\n",
		       source, source_county->get_household(j)->get_id(),
		       source_county->get_household(j)->get_migration_fips());
	}
      }
    }
  }


  //do this after all counties have identified households to swap
  if ( this->fips == County::migration_fips[County::migration_fips.size()-1]){
    County::migration_swap_houses();

    //take care of remaining houses marked for migration
    FRED_VERBOSE(1, "COUNTY TO COUNTY MIGRATION REMAINING HOUSES\n");

    for (int k=0; k<number_of_counties; k++) {
      int remaining = 0;
      int source = County::migration_fips[k];
      County* source_county = Global::Places.get_county(source);
      int index = -1;
      FRED_VERBOSE(1, "MIGRATION REMAINING HOUSES source %d\n", source);
      int source_households = source_county->get_number_of_households();
      for (int m=0; m < source_households; m++) {
	//FRED_VERBOSE(0, "MIGRATION SWAP HOUSES swap matrix before m %d\n", m);
	//for each house to move, get the destination, get index for destination
	Household* hh = source_county->get_household(m);
	int dest_fips = hh->get_migration_fips();
	if (dest_fips >0) {
	  remaining++;
	  migrate_household_to_county(hh, dest_fips);
	  hh->clear_migration_fips();
	}
      }
      FRED_VERBOSE(1, "REMAINING HOUSES AFTER SWAPS source %d houses %d\n", source, remaining);
    }
  }
  FRED_VERBOSE(0, "COUNTY-TO-COUNTY MIGRATION finished for county %d year %d\n", this->fips, year);
}


void County::migration_swap_houses() {

  // get the current year
  int year = Date::get_year();
  int day = Global::Simulation_Day;

  if (year < 2010) {
    return;
  }

  FRED_VERBOSE(1, "MIGRATION SWAP HOUSES entered fips %d year %d\n", this->fips, year);
  int number_of_counties = County::migration_fips.size();
  // two-dim array of vectors of houses to swap
  HouselistT** swap_houselist;
  swap_houselist = new HouselistT*[number_of_counties];
  for(int s = 0; s < number_of_counties; ++s) {
    swap_houselist[s] = new HouselistT[number_of_counties];
    for(int d = 0; d < number_of_counties; ++d) {
      swap_houselist[s][d].clear();
    }
  }
  // for each county, get the houses moving to all other counties and set up swap matrix of vectors of households to swap

  FRED_VERBOSE(1, "MIGRATION SWAP HOUSES swap matrix before\n");
  for (int s=0; s<number_of_counties; s++) {
    int source = County::migration_fips[s];
    County* source_county = Global::Places.get_county(source);
    int d = -1;
    FRED_VERBOSE(1, "MIGRATION SWAP HOUSES swap matrix source %d\n", source);
    int source_households = source_county->get_number_of_households();
    for (int m = 0; m < source_households; m++) {
      //FRED_VERBOSE(0, "MIGRATION SWAP HOUSES swap matrix before m %d\n", m);
      //for each house to move, get the destination, get index d for destination
      Household* hh = source_county->get_household(m);
      int dest_fips = hh->get_migration_fips();
      if (dest_fips >0) {
	for (int z = 0; z < number_of_counties; z++) {
	  if (County::migration_fips[z] == dest_fips){
	    d = z;
	  }
	}
	FRED_VERBOSE(1, "MIGRATION SWAP HOUSES swap matrix dest %d index %d\n", dest_fips, d);
	swap_houselist[s][d].push_back(m);
      }
    }
  }

  // swap households between counties.
  for (int n=0; n<number_of_counties-1; n++) {
    for (int p=n+1; p<number_of_counties; p++) {
      int mig_source = County::migration_fips[n];
      County* mig_source_county = Global::Places.get_county(mig_source);
      int mig_dest = County::migration_fips[p];
      County* mig_dest_county = Global::Places.get_county(mig_dest);
      int hs_size = swap_houselist[n][p].size();
      int hd_size = swap_houselist[p][n].size();
      int num_to_swap = (hs_size<=hd_size ? hs_size : hd_size); //get smaller size to swap
      for (int s=0; s<num_to_swap; s++) {
	Household* hs = mig_source_county->households[swap_houselist[n][p][s]];
	Household* hd = mig_dest_county->households[swap_houselist[p][n][s]];
	Global::Places.swap_houses(hs, hd);
	// clear the migration status for households being swapped
	hs->clear_migration_fips();
	hd->clear_migration_fips();
      }
    }
  }
}


void County::migrate_to_target_popsize() {

  FRED_VERBOSE(0, "MIGRATE TO TARGET : FIPS = %d\n", this->fips);

  recompute_county_popsize();

  int year = Date::get_year();
  if (year < 2010 || 2040 < year) {
    return;
  }
  int day = Global::Simulation_Day;
  FRED_VERBOSE(0, "MIGRATE migration to target entered fips %d year %d\n", this->fips, year);

  // number of years left before next target year
  int years_to_target = (2100 - year) % 5;

  int target_year_index = (year - 2010)/5;   // current year gives target_year_index
  // cout <<"target_year_index " << ',' << target_year_index << endl;
  if (year % 5 > 0) {
    target_year_index++;
  }
  // record the initial population count for each age
  int male_survivors[Demographics::MAX_AGE+1]; //array big enough to move up by 4 years without losing
  int female_survivors[Demographics::MAX_AGE+1];
  for (int i=0; i<= Demographics::MAX_AGE; i++){
    male_survivors[i] = this->male_popsize[i];
    female_survivors[i] = this->female_popsize[i];
  }

  // get numbers in age group
  int estimated_males[AGE_GROUPS];
  int estimated_females[AGE_GROUPS];
  int male_total = 0;
  int female_total = 0;
  int age_group_count = 0;

  for (int age_group = 0; age_group < AGE_GROUPS-1; age_group++) {
    estimated_males[age_group] = 0;
    estimated_females[age_group] = 0;
    for (int j = 0 ; j < 5; j++) {
      estimated_males[age_group] += male_survivors[age_group*5+j];
      estimated_females[age_group] += female_survivors[age_group*5+j];
    }
    male_total += estimated_males[age_group];
    female_total += estimated_females[age_group];
  }
  int age_group = AGE_GROUPS-1;
  estimated_males[age_group] = 0;
  estimated_females[age_group] = 0;
  for (int j = 85 ; j <= Demographics::MAX_AGE ; j++) {
    estimated_males[age_group] += male_survivors[j];
    estimated_females[age_group] += female_survivors[j];
  }
  male_total += estimated_males[age_group];
  female_total += estimated_females[age_group];

  if (years_to_target == 0) {
    int tot_target_males = 0;
    int tot_target_females = 0;
    for (int age_group = 0; age_group < AGE_GROUPS; age_group++) {
      FRED_VERBOSE(1, "BEFORE MIGRATE lower age %d curr males = %d target males = %d curr females = %d target females = %d\n",
		   age_group*5,
		   estimated_males[age_group], target_males[age_group][target_year_index],
		   estimated_females[age_group], target_females[age_group][target_year_index]);
      tot_target_males += target_males[age_group][target_year_index];
      tot_target_females += target_females[age_group][target_year_index];
    }
    FRED_VERBOSE(1, "BEFORE MIGRATE curr males = %d target males = %d curr females = %d target females = %d  curr total = %d target total = %d\n",
		 male_total, tot_target_males, female_total, tot_target_females, male_total+female_total, tot_target_males+tot_target_females);
  }

  // estimate survivors in the current population at the next target year

  // store newborn counts (we assume they are constant over the new few years)
  int male_newborns = male_survivors[0];
  int female_newborns = female_survivors[0];

  for (int j=0; j<years_to_target; j++) {
    //apply mortality
    for (int k=0; k<=Demographics::MAX_AGE; k++) {
      male_survivors[k] -=  (male_survivors[k] * this->get_mortality_rate(k,'M'));
      female_survivors[k] -= (female_survivors[k] * this->get_mortality_rate(k,'F'));
    }
    // advance their ages
    for (int age=Demographics::MAX_AGE; age>0; age--) {
      male_survivors[age] =  male_survivors[age-1];
      female_survivors[age] = female_survivors[age-1];
    }
    // restore newborn counts (we assume they are constant over the new few years)
    male_survivors[0] = male_newborns;
    female_survivors[0] = female_newborns;
  }

  // get numbers in age group
  male_total = 0;
  female_total = 0;
  age_group_count = 0;
  for (int age_group = 0; age_group < AGE_GROUPS-1; age_group++) {
    estimated_males[age_group] = 0;
    estimated_females[age_group] = 0;
    for (int j = 0 ; j < 5; j++) {
      estimated_males[age_group] += male_survivors[age_group*5+j];
      estimated_females[age_group] += female_survivors[age_group*5+j];
    }
    male_total += estimated_males[age_group];
    female_total += estimated_females[age_group];
  }
  age_group = AGE_GROUPS-1;
  estimated_males[age_group] = 0;
  estimated_females[age_group] = 0;
  for (int j = 85 ; j <= Demographics::MAX_AGE ; j++) {
    estimated_males[age_group] += male_survivors[j];
    estimated_females[age_group] += female_survivors[j];
  }
  male_total += estimated_males[age_group];
  female_total += estimated_females[age_group];

  // get difference between estimates and targets
  // and adjust for number of years to target
  int males_to_migrate[AGE_GROUPS];
  int females_to_migrate[AGE_GROUPS];

  for (int age_group=0; age_group<AGE_GROUPS; age_group++) {
    males_to_migrate[age_group] = (target_males[age_group][target_year_index] - estimated_males[age_group])/ (years_to_target+1);
    females_to_migrate[age_group] = (target_females[age_group][target_year_index] - estimated_females[age_group]) / (years_to_target+1);
    FRED_VERBOSE(1,
		 "MIGRATE year = %d years to target = %d age group = %d  males = %d females = %d\n",
		 year, years_to_target, age_group*5, males_to_migrate[age_group], females_to_migrate[age_group]);
  }

  // add or remove migrants as needed
  int total_migrants = 0;
  for (int age_group = 0; age_group < AGE_GROUPS; age_group++) {

    int lower_age = 5*age_group;
    int upper_age = lower_age+4;
    if (lower_age == 85) {
      upper_age = Demographics::MAX_AGE;
    }
    lower_age -= years_to_target;
    upper_age -= years_to_target;
    if (lower_age < 0) {
      lower_age = 0;
    }

    total_migrants += abs(males_to_migrate[age_group]);
    total_migrants += abs(females_to_migrate[age_group]);

    if (males_to_migrate[age_group] > 0) {
      // add these migrants to the population
      FRED_VERBOSE(1,
		   "MIGRATE ADD lower age %d upper age %d males = %d year = %d\n",
		   lower_age, upper_age, males_to_migrate[age_group], year);
      for (int k = 0; k < males_to_migrate[age_group]; k++) {
	char sex = 'M';
	int my_age = Random::draw_random_int(lower_age, upper_age);
	add_immigrant(my_age, sex);
      }
    }
    else {
      // find outgoing migrants
      FRED_VERBOSE(1,
		   "MIGRATE REMOVE lower age %d upper age %d males = %d year = %d\n",
		   lower_age, upper_age, males_to_migrate[age_group], year);
      select_migrants(day, -males_to_migrate[age_group], lower_age, upper_age, 'M', 0);
    }

    if (females_to_migrate[age_group] > 0) {
      // add these migrants to the population
      FRED_VERBOSE(1,
		   "MIGRATE ADD lower age %d upper age %d females = %d year = %d\n",
		   lower_age, upper_age, females_to_migrate[age_group], year);
      for (int k = 0; k < females_to_migrate[age_group]; k++) {
	char sex = 'F';
	int my_age = Random::draw_random_int(lower_age, upper_age);
	add_immigrant(my_age, sex);
      }
    }
    else {
      // find outgoing migrants
      FRED_VERBOSE(1,
		   "MIGRATE REMOVE lower age %d upper age %d females = %d year = %d\n",
		   lower_age, upper_age, females_to_migrate[age_group], year);
      select_migrants(day, -females_to_migrate[age_group], lower_age, upper_age, 'F', 0);
    }
  }

  if (years_to_target==0) {

    // get numbers in age group
    male_total = 0;
    female_total = 0;
    age_group_count = 0;
    for (int age_group = 0; age_group < AGE_GROUPS-1; age_group++) {
      estimated_males[age_group] = 0;
      estimated_females[age_group] = 0;
      for (int j = 0 ; j < 5; j++) {
	estimated_males[age_group] += this->male_popsize[age_group*5+j];
	estimated_females[age_group] += this->female_popsize[age_group*5+j];
      }

      // adjust for fact that out-going migrants are not removed until the next day
      if (males_to_migrate[age_group] < 0) {
	estimated_males[age_group] += males_to_migrate[age_group];
      }
      if (females_to_migrate[age_group] < 0) {
	estimated_females[age_group] += females_to_migrate[age_group];
      }

      male_total += estimated_males[age_group];
      female_total += estimated_females[age_group];
    }
    age_group = AGE_GROUPS-1;
    estimated_males[age_group] = 0;
    estimated_females[age_group] = 0;
    for (int j = 85 ; j <= Demographics::MAX_AGE ; j++) {
      estimated_males[age_group] += this->male_popsize[j];
      estimated_females[age_group] += this->female_popsize[j];
    }
    // adjust for fact that out-going migrants are not removed until the next day
    if (males_to_migrate[age_group] < 0) {
      estimated_males[age_group] += males_to_migrate[age_group];
    }
    if (females_to_migrate[age_group] < 0) {
      estimated_females[age_group] += females_to_migrate[age_group];
    }
    male_total += estimated_males[age_group];
    female_total += estimated_females[age_group];
    
    int tot_target_males = 0;
    int tot_target_females = 0;
    for (int age_group = 0; age_group < AGE_GROUPS; age_group++) {
      FRED_VERBOSE(1, "AFTER MIGRATE lower age %d curr males = %d target males = %d curr females = %d target females = %d\n",
		   age_group*5,
		   estimated_males[age_group], target_males[age_group][target_year_index],
		   estimated_females[age_group], target_females[age_group][target_year_index]);
      tot_target_males += target_males[age_group][target_year_index];
      tot_target_females += target_females[age_group][target_year_index];
    }
    if (male_total+female_total != tot_target_males+tot_target_females) {
      FRED_VERBOSE(1, "AFTER MIGRATE TO TARGET fips %d curr males = %d target males = %d curr females = %d target females = %d  curr total = %d target total = %d\n",
		   this->fips, male_total, tot_target_males, female_total,
		   tot_target_females, male_total+female_total,
		   tot_target_males+tot_target_females);
    }
  }
  FRED_VERBOSE(0, "MIGRATE TO TARGET finished : FIPS = %d  total_migrants = %d\n", this->fips, total_migrants);
}


void County::migrate_household_to_county(Place* house, int dest) {
  int day = Global::Simulation_Day;
  County* dest_county = Global::Places.get_county(dest);
  int newsize = dest_county->get_current_popsize();
  FRED_VERBOSE(1, "migrate household to county dest %d popsize before %d \n",
	       dest_county->fips, newsize);
  int hsize = house->get_size();
  if (dest_county != NULL) {
    Place* newhouse = dest_county->select_new_house_for_immigrants(hsize);
    for(int j = 0; j < hsize; ++j) {
      Person* person = house->get_enrollee(j);
      if (person->is_eligible_to_migrate()) {
	person->change_household(newhouse);
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION household %d RELOCATE person %d age %d to house %d\n",
		     house->get_id(), person->get_id(), person->get_age(), newhouse->get_id());
      }
    }
  }
  else {
    for(int j = 0; j < hsize; ++j) {
      Person* person = house->get_enrollee(j);
      if (person->is_eligible_to_migrate()) {
	// prepare to remove person
	Global::Pop.prepare_to_migrate(day, person);
	FRED_VERBOSE(1, "COUNTY-TO-COUNTY MIGRATION household %d REMOVED person %d age %d\n",
		     house->get_id(), person->get_id(), person->get_age());
      }
    }
  }
  newsize = dest_county->get_current_popsize();
  FRED_VERBOSE(1, "migrate household to county popsize after %d \n",
	       newsize);
}


Place* County::select_new_house_for_immigrants(int hsize) {
  Place* house = NULL;
  while (house == NULL) {
    int hnum = Random::draw_random_int(0, this->households.size()-1);
    house = this->households[hnum];
    // don't migrate to group quarters
    if (house->is_group_quarters()) {
      house = NULL;
    }
  }
  return house;
}


void County::select_migrants(int day, int migrants, int lower_age, int upper_age, char sex, int dest) {

  County* target = NULL;
  if (dest > 0) {
    target = Global::Places.get_county(dest);
  }

  std::vector<Person*> people_to_migrate;
  people_to_migrate.clear();

  // add all the current county residents of the desired age and sex
  // except those that have already been marked for migration today
  if (sex == 'M') {
    for (int age = lower_age; age <= upper_age; age++) {
      int size = this->males_of_age[age].size();
      for (int i = 0; i < size ; i++) {
	Person* person = this->males_of_age[age][i];
	if (person->is_eligible_to_migrate() || person->get_household()->is_group_quarters()) {
	  people_to_migrate.push_back(person);
	}
      }
    }
  }
  else {
    for (int age = lower_age; age <= upper_age; age++) {
      int size = this->females_of_age[age].size();
      for (int i = 0; i < size ; i++) {
	Person* person = this->females_of_age[age][i];
	if (person->is_eligible_to_migrate() || person->get_household()->is_group_quarters()) {
	  people_to_migrate.push_back(person);
	}
      }
    }
  }

  std::random_shuffle(people_to_migrate.begin(), people_to_migrate.end());

  int count = 0;
  for (int i = 0; i < migrants && i < people_to_migrate.size(); ++i) {
    Person* person = people_to_migrate[i];
    FRED_VERBOSE(1, "MIGRATE select_migrant person %d age %d sex %c\n",person->get_id(),person->get_age(),sex);
    if (target == NULL) {
      Global::Pop.prepare_to_migrate(day, person);
    }
    else {
      target->add_immigrant(person);
    }
    person->unset_eligible_to_migrate();
    count++;
  }

  if (migrants != count) {
    FRED_VERBOSE(0, "MIGRATE select_migrants from %d to %d: wanted %d people found %d candidates between %d and %d sex %c, got %d\n",
		 this->fips, dest, migrants, 
		 people_to_migrate.size(), lower_age, upper_age, sex, count);
  }
}

void County::add_immigrant(int age, char sex) {
  int race = 0;
  int rel = 0;
  Place* school = NULL;
  Place* work = NULL;
  int day = Global::Simulation_Day;

  // pick a random household
  int hnum = Random::draw_random_int(0, this->number_of_households-1);
  Place* house = this->households[hnum];

  Person* person = Global::Pop.add_person_to_population(age, sex, race, rel, house, school, work, day, false);
  person->unset_native();
  person->get_demographics()->initialize_demographic_dynamics(person);
  person->update_profile_after_changing_household();
  if (Global::Verbose > 1) {
    printf("IMMIGRANT AGE %d profile |%c|\n", age, person->get_profile());fflush(stdout);
    person->print_activities();
  }
}


void County::add_immigrant(Person* person) {
  // pick a random household
  int hnum = Random::draw_random_int(0, this->number_of_households-1);
  Place* house = this->households[hnum];
  FRED_VERBOSE(1, "add_immigrant hnum %d fips %d \n",hnum, fips);
  person->change_household(house);
}


int County::get_current_popsize(int age_min, int age_max, char sex) {
  if(age_min < 0) {
    age_min = 0;
  }
  if(age_max > Demographics::MAX_AGE) {
    age_max = Demographics::MAX_AGE;
  }
  if(age_min > age_max) {
    age_min = 0;
  }
  if(age_min >= 0 && age_max >= 0 && age_min <= age_max) {
    if(sex == 'F' || sex == 'M') {
      int temp_count = 0;
      for(int i = age_min; i <= age_max; ++i) {
	temp_count += (sex == 'F' ? this->female_popsize[i] : this->male_popsize[i]);
      }
      return temp_count;
    }
  }
  return -1;
}


void County::recompute_county_popsize() {
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->female_popsize[i] = 0;
    this->male_popsize[i] = 0;
  }
  for (int i = 0; i < this->number_of_households; i++) {
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      int age = person->get_age();
      char sex = person->get_sex();
      if (sex == 'M') {
	this->male_popsize[age]++;
      }
      else {
	this->female_popsize[age]++;
      }
    }
  }
}

void County::group_population_by_sex_and_age(int reset) {
  FRED_VERBOSE(1, "County group_pop : FIPS = %d reset = %d\n", this->fips, reset);
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->males_of_age[i].clear();
    this->females_of_age[i].clear();
  }
  for (int i = 0; i < this->number_of_households; i++) {
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      if (person->is_deceased()) {
	continue;
      }
      if (reset && (person->get_household()->is_group_quarters()==false)) {
	person->set_eligible_to_migrate();
      }
      if (person->is_eligible_to_migrate()==false) {
	continue;
      }
      int age = person->get_age();
      char sex = person->get_sex();
      if (sex == 'M') {
	this->males_of_age[age].push_back(person);;
      }
      else {
	this->females_of_age[age].push_back(person);;
      }
    }
  }
  FRED_VERBOSE(1, "County group_pop finished : FIPS = %d reset = %d\n", this->fips, reset);
}

void County::report() {

  int year = Date::get_year();
  if (year < 2010) {
    return;
  }
  int males[18];
  int females[18];
  int male_count = 0;
  int female_count = 0;
  std::vector<double>ages;
  ages.clear();
  ages.reserve(this->tot_current_popsize);
  for (int i = 0; i < 18; i++) {
    males[i] = 0;
    females[i] = 0;
  }

  int popsize = 0;
  for (int i = 0; i < this->number_of_households; i++) {
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      ages.push_back(person->get_real_age());
      int age = person->get_age();
      int age_group = age / 5;
      if (age_group > 17) { 
	age_group = 17;
      }
      if (person->get_sex()=='M') {
	males[age_group]++;
	male_count++;
      }
      else {
	females[age_group]++;
	female_count++;
      }
      popsize++;
    }
  }
  this->tot_current_popsize = popsize;
  std::sort(ages.begin(), ages.end());
  double median = ages[popsize/2];

  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/pop-%d-%d.txt",
	  Global::Simulation_directory,
	  this->fips,
	  Global::Simulation_run_number);
  FILE *fp = NULL;
  if (year == 2010) {
    fp = fopen(filename,"w");
  }
  else {
    fp = fopen(filename,"a");
  }
  assert(fp != NULL);
  fprintf(fp, "%d total %d males %d females %d median_age %0.2f\n",
	  Date::get_year(),
	  popsize,
	  male_count, female_count, median);
  fclose(fp);

  if (year % 5 == 0) {
    sprintf(filename, "%s/pop-%d-ages-%d-%d.txt",
	    Global::Simulation_directory,
	    this->fips,
	    year,
	    Global::Simulation_run_number);
    fp = fopen(filename,"w");
    assert(fp != NULL);
    for (int i = 0; i < 18; i++) {
      int lower = 5*i;
      char label[16];
      if (lower < 85) {
	sprintf(label,"%d-%d",lower,lower+4);
      }
      else {
	sprintf(label,"85+");
      }
      fprintf(fp, "%d %s %d %d %d %d\n",
	      Date::get_year(), label, lower,
	      males[i], females[i], males[i]+females[i]);
    }
    fclose(fp);

    // check workplace sizes
    report_workplace_sizes();
    report_school_sizes();
  }

}

