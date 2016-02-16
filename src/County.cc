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

#include "County.h"

#include "Population.h"
#include "Age_Map.h"
#include "Person.h"
#include "Random.h"
#include "Global.h"
#include "Date.h"
#include "Utils.h"
#include "Place_List.h"
#include "Household.h"

class Global;

County::~County() {
}

County::County(int _fips) {
  this->is_initialized = false;
  this->fips = _fips;
  this->tot_current_popsize = 0;
  this->tot_female_popsize = 0;
  this->tot_male_popsize = 0;
  this->target_popsize = 0;
  this->population_growth_rate = 0.0;
  this->college_departure_rate = 0.0;
  this->military_departure_rate = 0.0;
  this->prison_departure_rate = 0.0;
  this->youth_home_departure_rate = 0.0;
  this->adult_home_departure_rate = 0.0;
  this->houses = 0;
  this->beds = NULL;
  this->occupants = NULL;
  this->max_beds = -1;
  this->max_occupants = -1;
  this->ready_to_move.clear();
  this->households.clear();
  this->mortality_rate_adjustment_weight = 0.0;

  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->female_popsize[i] = 0;
    this->male_popsize[i] = 0;
  }

  if(Global::Enable_Population_Dynamics == false) {
    return;
  }

  char mortality_rate_file[FRED_STRING_SIZE];
  char birth_rate_file[FRED_STRING_SIZE];
  double birth_rate_multiplier;
  double mortality_rate_multiplier;

  Params::get_param_from_string("population_growth_rate", &(this->population_growth_rate));
  Params::get_param_from_string("birth_rate_file", birth_rate_file);
  Params::get_param_from_string("birth_rate_multiplier", &birth_rate_multiplier);
  Params::get_param_from_string("mortality_rate_multiplier", &mortality_rate_multiplier);
  Params::get_param_from_string("mortality_rate_file", mortality_rate_file);
  Params::get_param_from_string("mortality_rate_adjustment_weight", &(this->mortality_rate_adjustment_weight));
  Params::get_param_from_string("college_departure_rate", &(this->college_departure_rate));
  Params::get_param_from_string("military_departure_rate", &(this->military_departure_rate));
  Params::get_param_from_string("prison_departure_rate", &(this->prison_departure_rate));
  Params::get_param_from_string("youth_home_departure_rate", &(this->youth_home_departure_rate));
  Params::get_param_from_string("adult_home_departure_rate", &(this->adult_home_departure_rate));
  
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

  // read in the migration file
  this->male_migrants = new int* [7];
  this->female_migrants = new int* [7];
  for (int i = 0; i < 7; i++) {
    this->male_migrants[i] = new int [18];
    this->female_migrants[i] = new int [18];
  }
  for (int i = 0; i < 7; i++) {
    for (int col = 0; col < 18; col++ ) {
      this->male_migrants[i][col] = 0;
      this->female_migrants[i][col] = 0;
    }
  }

  char migration_file[FRED_STRING_SIZE];
  Params::get_param_from_string("migration_file", migration_file);
  fp = Utils::fred_open_file(migration_file);

  if(fp != NULL) {
    for (int row = 0; row < 7; row++) {
      int y;
      fscanf(fp, "%d ", &y);
      assert(y==2010+row*5);
      for (int col = 0; col < 18; col++ ) {
	int count;
	fscanf(fp, "%d ", &count);
	this->male_migrants[row][col] = count;
      }
    }
    printf("male migrants");fflush(stdout);
    for (int i = 0; i < 7; i++) {
      printf("%d ", 2010+i*5);
      for (int j = 0; j < 18; j++) {
	printf("%d ", this->male_migrants[i][j]);
      }
      printf("\n");
    }

    for (int row = 0; row < 7; row++) {
      int y;
      fscanf(fp, "%d ", &y);
      assert(y==2010+row*5);
      for (int col = 0; col < 18; col++ ) {
	int count;
	fscanf(fp, "%d", &count);
	this->female_migrants[row][col] = count;
      }
    }
    printf("female migrants");fflush(stdout);
    for (int i = 0; i < 7; i++) {
      printf("%d ", 2010+i*5);
      for (int j = 0; j < 18; j++) {
	printf("%d ", this->female_migrants[i][j]);
      }
      printf("\n");
    }
    fflush(stdout);
    fclose(fp);
  }
  else {
    printf("no migration file found for fips %d\n", this->fips);
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

  // TODO: test and enable
  return;

  // update housing periodically
  if(day == 0 || (Date::get_month() == 6 && Date::get_day_of_month() == 30)) {
    FRED_VERBOSE(0, "County::update_housing = %d\n", day);
    update_housing(day);
    FRED_VERBOSE(0, "County::update_housing = %d\n", day);
  }

  // update birth and death rates on July 1
  if(Date::get_month() == 7 && Date::get_day_of_month() == 1) {
    update_population_dynamics(day);
  }

}

void County::update_population_dynamics(int day) {

  static double total_adj = 1.0;

  // no adjustment for first year, to avoid overreacting to low birth rate
  if(day < 1) {
    return;
  }

  // get the current year
  int year = Date::get_year();

  // get the target population size for the end of the coming year
  this->target_popsize = (1.0 + 0.01 * this->population_growth_rate) * this->target_popsize;

  double error = (this->tot_current_popsize - this->target_popsize) / (1.0 * this->target_popsize);
  
  double mortality_rate_adjustment = 1.0 + this->mortality_rate_adjustment_weight * error;
  
  total_adj *= mortality_rate_adjustment;

  // adjust mortality rates
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->adjusted_female_mortality_rate[i] = mortality_rate_adjustment * this->adjusted_female_mortality_rate[i];
    this->adjusted_male_mortality_rate[i] = mortality_rate_adjustment * this->adjusted_male_mortality_rate[i];
  }

  // message to LOG file
  FRED_VERBOSE(0, "COUNTY %d POP_DYN: year %d  popsize = %d  target = %d  pct_error = %0.2f death_adj = %0.4f  total_adj = %0.4f\n",
	             this->fips, year, this->tot_current_popsize, this->target_popsize, 100.0 * error, mortality_rate_adjustment, total_adj);
  
}

void County::get_housing_imbalance(int day) {
  get_housing_data(this->beds, this->occupants);
  int imbalance = 0;
  int houses = this->households.size();
  for (int i = 0; i < houses; ++i) {
    // skip group quarters
    if(this->households[i]->is_group_quarters()) {
      continue;
    }
    imbalance += abs(this->beds[i] - this->occupants[i]);
  }
  FRED_DEBUG(1, "DAY %d HOUSING: houses = %d, imbalance = %d\n", day, houses, imbalance);
}

int County::fill_vacancies(int day) {
  // move ready_to_moves into underfilled units
  int moved = 0;
  if(this->ready_to_move.size() > 0) {
    // first focus on the empty units
    int houses = this->households.size();
    for (int newhouse = 0; newhouse < houses; ++newhouse) {
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
	        if(ohouseptr->is_group_quarters() || (this->occupants[oldhouse] - this->beds[oldhouse] > 0)) {
	          // move person to new home
	          person->move_to_new_house(houseptr);
	          this->occupants[oldhouse]--;
	          this->occupants[newhouse]++;
	          moved++;
	        }
	      }
      }
    }

    // now consider any vacancy
    for(int newhouse = 0; newhouse < houses; ++newhouse) {
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
	        if(ohouseptr->is_group_quarters() || (this->occupants[oldhouse] - this->beds[oldhouse] > 0)) {
	          // move person to new home
	          person->move_to_new_house(houseptr);
	          this->occupants[oldhouse]--;
	          this->occupants[newhouse]++;
	          moved++;
	        }
	      }
      }
    }
  }
  return moved;
}

void County::update_housing(int day) {

  this->houses = this->households.size();
  FRED_VERBOSE(0, "houses = %d\n", houses);

  if(day == 0) {
    // initialize house data structures
    this->beds = new int[houses];
    this->occupants = new int[houses];
    this->target_popsize = tot_current_popsize;
  }

  // reserve ready_to_move vector:
  this->ready_to_move.reserve(tot_current_popsize);

  get_housing_data(this->beds, this->occupants);
  this->max_beds = -1;
  this->max_occupants = -1;
  for(int i = 0; i < this->houses; ++i) {
    if(this->beds[i] > this->max_beds) {
      this->max_beds = this->beds[i];
    }
    if(this->occupants[i] > this->max_occupants) {
      this->max_occupants = this->occupants[i];
    }
  }
  FRED_DEBUG(1, "DAY %d HOUSING: houses = %d, max_beds = %d max_occupants = %d\n",
	 day, this->houses, this->max_beds, this->max_occupants);
  FRED_DEBUG(1, "BEFORE ");
  get_housing_imbalance(day);

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

  move_young_adults(day);
  get_housing_imbalance(day);

  move_older_adults(day);
  get_housing_imbalance(day);

  swap_houses(day);
  get_housing_imbalance(day);

  report_household_distributions();
  // Global::Places.report_school_distributions(day);
  return;
}

void County::move_college_students_out_of_dorms(int day) {
  // printf("MOVE FORMER COLLEGE RESIDENTS =======================\n");
  this->ready_to_move.clear();
  int college = 0;
  // find students ready to move off campus
  int houses = this->households.size();
  for(int i = 0; i < houses; ++i) {
    Household* house = this->households[i];
    if(house->is_college()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      // printf("PERSON %d LIVES IN COLLEGE DORM %s\n", person->get_id(), house->get_label());
	      assert(person->is_college_dorm_resident());
	      college++;
	      // some college students leave each year
	      if(Random::draw_random() < this->college_departure_rate) {
          this->ready_to_move.push_back(make_pair(person,i));
	      }
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d COLLEGE\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  // printf("DAY %d MOVED %d COLLEGE STUDENTS\n",day,moved);
  // printf("DAY %d COLLEGE COUNT AFTER DEPARTURES %d\n", day, college-moved); fflush(stdout);
}

void County::move_college_students_into_dorms(int day) {
  // printf("GENERATE NEW COLLEGE RESIDENTS =======================\n");
  this->ready_to_move.clear();
  int moved = 0;
  int college = 0;

  // find vacant doom rooms
  std::vector<int>dorm_rooms;
  dorm_rooms.clear();
  for(int i = 0; i < this->houses; ++i) {
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
  // printf("COLLEGE COUNT %d VACANCIES %d\n", college, dorm_vacancies);
  if(dorm_vacancies == 0) {
    FRED_DEBUG(1, "NO COLLEGE VACANCIES FOUND\n");
    return;
  }

  // find students to fill the dorms
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters() == false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) {
        continue;
      }
      for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      int age = person->get_age();
	      if(18 < age && age < 40 && person->get_number_of_children() == 0) {
	        this->ready_to_move.push_back(make_pair(person,i));
	      }
      }
    }
  }
  // printf("COLLEGE APPLICANTS %d\n", (int)ready_to_move.size());

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(1, "NO COLLEGE APPLICANTS FOUND\n");
    return;
  }

  // shuffle the applicants
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < dorm_vacancies &&this->ready_to_move.size() > 0; ++i) {
    int newhouse = dorm_rooms[i];
    Place* houseptr = Global::Places.get_household(newhouse);
//      printf("VACANT DORM %s ORIG %d SIZE %d\n", houseptr->get_label(),
//      houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    this->ready_to_move.pop_back();
    // move person to new home
//      printf("APPLICANT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
//      person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
//      ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_DEBUG(1, "DAY %d ACCEPTED %d COLLEGE STUDENTS, CURRENT = %d  MAX = %d\n", day, moved, college+moved, college + dorm_vacancies);
}

void County::move_military_personnel_out_of_barracks(int day) {
  // printf("MOVE FORMER MILITARY =======================\n");
  this->ready_to_move.clear();
  int military = 0;
  // find military personnel to discharge
  for(int i = 0; i < this->houses; ++i) {
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
  // printf("DAY %d READY TO MOVE %d FORMER MILITARY\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  // printf("DAY %d RELEASED %d MILITARY, TOTAL NOW %d\n",day,moved,military-moved);
}

void County::move_military_personnel_into_barracks(int day) {
  // printf("GENERATE NEW MILITARY BASE RESIDENTS =======================\n");
  this->ready_to_move.clear();
  int moved = 0;
  int military = 0;

  // find unfilled barracks units
  std::vector<int>barracks_units;
  barracks_units.clear();
  for(int i = 0; i < this->houses; ++i) {
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
  // printf("MILITARY VACANCIES %d\n", barracks_vacancies);
  if(barracks_vacancies == 0) {
    FRED_DEBUG(1, "NO MILITARY VACANCIES FOUND\n");
    return;
  }

  // find recruits to fill the dorms
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      int age = person->get_age();
	      if(18 < age && age < 40 && person->get_number_of_children() == 0) {
	        this->ready_to_move.push_back(make_pair(person,i));
	      }
      }
    }
  }
  // printf("MILITARY RECRUITS %d\n", (int)ready_to_move.size());

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(1, "NO MILITARY RECRUITS FOUND\n");
    return;
  }

  // shuffle the recruits
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < barracks_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = barracks_units[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED BARRACKS %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    this->ready_to_move.pop_back();
    // move person to new home
    // printf("RECRUIT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_DEBUG(1, "DAY %d ADDED %d MILITARY, CURRENT = %d  MAX = %d\n", day, moved, military + moved, military + barracks_vacancies);
}

void County::move_inmates_out_of_prisons(int day) {
  // printf("RELEASE PRISONERS =======================\n");
  this->ready_to_move.clear();
  int prisoners = 0;
  // find former prisoners still in jail
  for(int i = 0; i < this->houses; ++i) {
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
  FRED_DEBUG(1, "DAY %d RELEASED %d PRISONERS, TOTAL NOW %d\n", day, moved, prisoners - moved);
}

void County::move_inmates_into_prisons(int day) {
  // printf("GENERATE NEW PRISON RESIDENTS =======================\n");
  this->ready_to_move.clear();
  int moved = 0;
  int prisoners = 0;

  // find unfilled jail_cell units
  std::vector<int>jail_cell_units;
  jail_cell_units.clear();
  for(int i = 0; i < this->houses; ++i) {
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
  // printf("PRISON VACANCIES %d\n", jail_cell_vacancies);
  if(jail_cell_vacancies == 0) {
    FRED_DEBUG(1, "NO PRISON VACANCIES FOUND\n");
    return;
  }

  // find inmates to fill the jail_cells
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      int age = person->get_age();
	      if((18 < age && person->get_number_of_children() == 0) || (age < 50)) {
	        this->ready_to_move.push_back(make_pair(person,i));
	      }
      }
    }
  }
  // printf("PRISON POSSIBLE INMATES %d\n", (int)ready_to_move.size());

  if(this->ready_to_move.size() == 0) {
    FRED_DEBUG(1, "NO INMATES FOUND\n");
    return;
  }

  // shuffle the inmates
  FYShuffle< pair<Person*, int> >(this->ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < jail_cell_vacancies && this->ready_to_move.size() > 0; ++i) {
    int newhouse = jail_cell_units[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED JAIL_CELL %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    this->ready_to_move.pop_back();
    // move person to new home
    // printf("INMATE %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_DEBUG(1, "DAY %d ADDED %d PRISONERS, CURRENT = %d  MAX = %d\n", day, moved, prisoners + moved, prisoners + jail_cell_vacancies);
}

void County::move_patients_into_nursing_homes(int day) {
  // printf("NEW NURSING HOME RESIDENTS =======================\n");
  this->ready_to_move.clear();
  int moved = 0;
  int nursing_home_residents = 0;
  int beds = 0;

  // find unfilled nursing_home units
  std::vector<int>nursing_home_units;
  nursing_home_units.clear();
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if (house->is_nursing_home()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; ++j) { nursing_home_units.push_back(i); }
      nursing_home_residents += house->get_size();;
      beds += house->get_orig_size();
    }
  }
  int nursing_home_vacancies = (int)nursing_home_units.size();
  // printf("NURSING HOME VACANCIES %d\n", nursing_home_vacancies);
  if(nursing_home_vacancies == 0) {
    FRED_DEBUG(1, "DAY %d ADDED %d NURSING HOME PATIENTS, TOTAL NOW %d BEDS = %d\n", day, 0, nursing_home_residents, beds);
    return;
  }

  // find patients to fill the nursing_homes
  for(int i = 0; i < this->houses; ++i) {
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
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED NURSING_HOME UNIT %s ORIG %d SIZE %d\n", houseptr->get_label(),houseptr->get_orig_size(),houseptr->get_size());
    Person* person = this->ready_to_move.back().first;
    int oldhouse = this->ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    this->ready_to_move.pop_back();
    // move person to new home
    /*
      printf("PATIENT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
      person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
      ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    */
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  FRED_DEBUG(1, "DAY %d ADDED %d NURSING HOME PATIENTS, CURRENT = %d  MAX = %d\n",day,moved,nursing_home_residents+moved,beds);
}

void County::move_young_adults(int day) {
  // printf("MOVE YOUNG ADULTS =======================\n");
  this->ready_to_move.clear();

  // find young adults in overfilled units who are ready to move out
  for(int i = 0; i < this->houses; ++i) {
    if(this->occupants[i] > this->beds[i]) {
      Household* house = this->households[i];
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      int age = person->get_age();
	      if(18 <= age && age < 30) {
	        if(Random::draw_random() < this->youth_home_departure_rate) {
	          this->ready_to_move.push_back(make_pair(person,i));
	        }
	      }
      }
    }
  }
  // printf("DAY %d READY TO MOVE young adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  FRED_DEBUG(1, "MOVED %d YOUNG ADULTS =======================\n",moved);
}


void County::move_older_adults(int day) {
  // printf("MOVE OLDER ADULTS =======================\n");
  this->ready_to_move.clear();

  // find older adults in overfilled units
  for(int i = 0; i < this->houses; ++i) {
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
  // printf("DAY %d READY TO MOVE older adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  FRED_DEBUG(1, "MOVED %d OLDER ADULTS =======================\n",moved);
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
  FRED_DEBUG(1, "SWAP HOUSES =======================\n");

  // two-dim array of vectors of imbalanced houses
  HouselistT** houselist;
  houselist = new HouselistT*[13];
  for(int i = 0; i < 13; ++i) {
    houselist[i] = new HouselistT[13];
    for(int j = 0; j < 13; ++j) {
      houselist[i][j].clear();
    }
  }
  for(int i = 0; i < this->houses; ++i) {
    // skip group quarters
    if(Global::Places.get_household(i)->is_group_quarters()) {
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
	      // printf("AFTER:\n"); report_ages(day,hi); report_ages(day,hj);
      }
    }
  }

  return; 

  // refill-vectors
  for(int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      houselist[i][j].clear();
    }
  }
  for(int i = 0; i < this->houses; ++i) {
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

  for(int i = 0; i < this->houses; ++i) {
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

  for(int i = 0; i < this->houses; ++i) {
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


int County::get_housing_data(int* target_size, int* current_size) {
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    Household* h = this->households[i];
    current_size[i] = h->get_size();
    target_size[i] = h->get_orig_size();
  }
  return num_households;
}


void County::report_household_distributions() {
  int houses = this->households.size();

  if(Global::Verbose) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < houses; ++p) {
      int n = this->households[p]->get_size();
      if(n <= 10) {
	      count[n]++;
      } else {
	      count[10]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "Household size distribution: N = %d ", total);
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
    for(int p = 0; p < houses; ++p) {
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
    fprintf(Global::Statusfp, "Household orig distribution: N = %d ", total);
    for(int c = 0; c <= 10; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) %0.2f ", c, count[c],
             (100.0 * count[c]) / total, count[c] ? ((double)hsize[c] / (double)count[c]) : 0.0);
    }
    fprintf(Global::Statusfp, "\n");
  }
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

void County::migration() {
  // get the current year
  int year = Date::get_year();
  int day = Global::Simulation_Day;

  if (Global::Test==0) {
    return;
  }

  if (2010 <= year && year <= 2040 && year % 5 == 0) {
    int i = (year-2010)/5;
    for (int j = 0; j < 18; j++) {
      int lower_age = 5*j;
      int upper_age = lower_age+4;
      int males = this->male_migrants[i][j];
      int females = this->female_migrants[i][j];
      if (males > 0) {
	// add these migrants to the population
	for (int k = 0; k < males; k++) {
	  char sex = 'M';
	  int my_age = Random::draw_random_int(lower_age, upper_age);
	  add_immigrant(my_age, sex);
	}
      }
      else {
	// delete members of the population
	males = -males;
	// set up a random shuffle
	std::vector<int>shuff;
	int size = this->households.size();
	shuff.reserve(size);
	for (int i = 0; i < size; i++) {
	  shuff[i]=i;
	}
	std::random_shuffle(shuff.begin(), shuff.end());
	int n = 0;
	int rounds = 0;
	for (int k = 0; k < males; k++) {
	  int found = 0;
	  while (!found) {
	    int hnum = shuff[n++];
	    if (size <= n) {
	      rounds++;
	      if (rounds < 2) {
		n = 0;
		hnum = shuff[n++];
	      }
	      else {
		Utils::fred_abort("Can't find person with age %d to %d to emigrate!",
				  lower_age, upper_age);
	      }
	    }
	    Place* house = Global::Places.get_household(hnum);
	    int hsize = house->get_size();
	    for(int j = 0; j < hsize; ++j) {
	      Person* person = house->get_enrollee(j);
	      int age = person->get_age();
	      if (lower_age <= age && age <= upper_age) {
		// prepare to remove person
		// Global::Pop.remove_dead_person_from_population(day, person);
		found = 1;
	      }
	    }
	  }
	}	
      }

      if (females > 0) {
	// add these migrants to the population
	for (int k = 0; k < females; k++) {
	  char sex = 'F';
	  int my_age = Random::draw_random_int(lower_age, lower_age+4);
	  add_immigrant(my_age, sex);
	}
      }
    }
  }
}

void County::add_immigrant(int age, char sex) {
  int race = 0;
  int rel = 0;
  Place* school = NULL;
  Place* work = NULL;
  int day = Global::Simulation_Day;

  // pick a random household
  int hnum = Random::draw_random_int(0, this->households.size()-1);
  Place* house = Global::Places.get_household(hnum);

  Global::Pop.add_person(age, sex, race, rel, house, school, work, day, false);
}


void County::report_age_distribution() {
  char filename[FRED_STRING_SIZE];
  FILE* fp = NULL;
  
  // get the current year
  int year = Date::get_year();

  if ((year % 5) > 0) {
    return;
  }

  sprintf(filename, "%s/ages-%d-%d.txt", Global::Simulation_directory, fips, year);
  fp = fopen(filename, "w");
  for (int lower = 0; lower <= 85; lower += 5) {
    int upper = lower+4;
    if (lower == 85) {
      upper = 120;
    }
    int females = get_current_popsize(lower, upper, 'F');
    int males = get_current_popsize(lower, upper, 'M');
    if (lower < 85) {
      fprintf(fp, "%d-%d %d %d\n", lower, upper, males, females);
    }
    else {
      fprintf(fp, "%d+ %d %d\n", lower, males, females);
    }
  }
  fclose(fp);
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

