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
  this->fips = _fips;
  this->current_popsize = 0;
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
    this->pregnancy_rate[i] =
      0.25 * this->birth_rate[i] + 0.75 * this->birth_rate[i+1];
  }
  this->pregnancy_rate[Demographics::MAX_AGE] = 0.0;

  if(Global::Verbose) {
    for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
      fprintf(Global::Statusfp, "BIRTH RATE     for age %d %e\n", i, birth_rate[i]);
      fprintf(Global::Statusfp, "PREGNANCY RATE for age %d %e\n", i, pregnancy_rate[i]);
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
    this->female_mortality_rate[i] = mortality_rate_multiplier*female_rate;
    this->male_mortality_rate[i] = mortality_rate_multiplier*male_rate;
  }
  fclose(fp);

  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->adjusted_female_mortality_rate[i] = this->female_mortality_rate[i];
    this->adjusted_male_mortality_rate[i] = this->male_mortality_rate[i];
  }

}


void County::update(int day) {

  // update housing periodically
  if(day == 0 || (Date::get_month() == 6 && Date::get_day_of_month() == 30)) {
    FRED_VERBOSE(0, "County::update_housing = %d\n", day);
    this->update_housing(day);
    FRED_VERBOSE(0, "County::update_housing = %d\n", day);
  }

  // update birth and death rates on July 1
  if(Date::get_month() == 7 && Date::get_day_of_month() == 1) {
    this->update_population_dynamics(day);
  }

}

void County::update_population_dynamics(int day) {
  static int first = 1;

  // no adjustment for first year, to avoid overreacting to low birth rate
  if (first) {
    first = 0;
    return;
  }

  // get the current year
  int year = Date::get_year();

  // get the target population size for the end of the coming year
  this->target_popsize = (1.0 + 0.01 * this->population_growth_rate)
    * this->target_popsize;

  double error = (this->current_popsize - this->target_popsize) / (1.0*this->target_popsize);
  
  // empirical tuning parameter
  double adjustment_weight = 7.5;
  
  double death_rate_adjustment = 1.0 + adjustment_weight * error;
  
  // adjust mortality rates
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    this->adjusted_female_mortality_rate[i] = death_rate_adjustment * this->adjusted_female_mortality_rate[i];
    this->adjusted_male_mortality_rate[i] = death_rate_adjustment * this->adjusted_male_mortality_rate[i];
  }

  // message to LOG file
  FRED_VERBOSE(0,
	       "COUNTY POP_DYN: year %d  popsize = %d  target = %d  pct_error = %0.2f death_adj = %0.4f\n",
	       year, this->current_popsize, this->target_popsize, 100.0*error, death_rate_adjustment); 
  
}

void County::get_housing_imbalance(int day) {
  get_housing_data(beds, occupants);
  int imbalance = 0;
  int houses = households.size();
  for (int i = 0; i < houses; ++i) {
    // skip group quarters
    if (households[i]->is_group_quarters()) continue;
    imbalance += abs(this->beds[i] - this->occupants[i]);
  }
  printf("DAY %d HOUSING: houses = %d, imbalance = %d\n", day, houses, imbalance);
}

int County::fill_vacancies(int day) {
  // move ready_to_moves into underfilled units
  int moved = 0;
  if(ready_to_move.size() > 0) {
    // first focus on the empty units
    int houses = households.size();
    for (int newhouse = 0; newhouse < houses; ++newhouse) {
      if(this->occupants[newhouse] > 0) {
        continue;
      }
      int vacancies = this->beds[newhouse] - this->occupants[newhouse];
      if(vacancies > 0) {
	Household * houseptr = this->households[newhouse];
	// skip group quarters
	if(houseptr->is_group_quarters()) {
	  continue;
	}
	for(int j = 0; (j < vacancies) && (ready_to_move.size() > 0); ++j) {
	  Person* person = ready_to_move.back().first;
	  int oldhouse = ready_to_move.back().second;
	  Household * ohouseptr = this->households[oldhouse];
	  ready_to_move.pop_back();
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
	Household * houseptr = this->households[newhouse];
	// skip group quarters
	if(houseptr->is_group_quarters()) {
	  continue;
	}
	for(int j = 0; (j < vacancies) && (ready_to_move.size() > 0); ++j) {
	  Person* person = ready_to_move.back().first;
	  int oldhouse = ready_to_move.back().second;
	  Household * ohouseptr = this->households[oldhouse];
	  ready_to_move.pop_back();
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
    this->target_popsize = current_popsize;
  }

  // reserve ready_to_move vector:
  ready_to_move.reserve(current_popsize);

  get_housing_data(this->beds, this->occupants);
  max_beds = -1;
  max_occupants = -1;
  for(int i = 0; i < houses; ++i) {
    if(this->beds[i] > max_beds) {
      max_beds = this->beds[i];
    }
    if(this->occupants[i] > max_occupants) {
      max_occupants = this->occupants[i];
    }
  }
  printf("DAY %d HOUSING: houses = %d, max_beds = %d max_occupants = %d\n",
  day, houses, max_beds, max_occupants);
  printf("BEFORE "); get_housing_imbalance(day);

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
  ready_to_move.clear();
  int college = 0;
  // find students ready to move off campus
  int houses = this->households.size();
  for(int i = 0; i < houses; ++i) {
    Household * house = households[i];
    if(house->is_college()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN COLLEGE DORM %s\n", person->get_id(), house->get_label());
	assert(person->is_college_dorm_resident());
	college++;
	// some college students leave each year
	if(RANDOM() < this->college_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
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
  ready_to_move.clear();
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
    printf("NO COLLEGE VACANCIES FOUND\n");
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
	Person* person = house->get_housemate(j);
	int age = person->get_age();
	if(18 < age && age < 40 && person->get_number_of_children() == 0) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("COLLEGE APPLICANTS %d\n", (int)ready_to_move.size());

  if(ready_to_move.size() == 0) {
    printf("NO COLLEGE APPLICANTS FOUND\n");
    return;
  }

  // shuffle the applicants
  FYShuffle< pair<Person*, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < dorm_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = dorm_rooms[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    /*
      printf("VACANT DORM %s ORIG %d SIZE %d\n", houseptr->get_label(),
      houseptr->get_orig_size(),houseptr->get_size());
    */
    Person* person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    /*
      printf("APPLICANT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
      person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
      ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    */
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ACCEPTED %d COLLEGE STUDENTS, CURRENT = %d  MAX = %d\n", day, moved, college+moved, college + dorm_vacancies);
}

void County::move_military_personnel_out_of_barracks(int day) {
  // printf("MOVE FORMER MILITARY =======================\n");
  ready_to_move.clear();
  int military = 0;
  // find military personnel to discharge
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_military_base()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN MILITARY BARRACKS %s\n", person->get_id(), house->get_label());
	assert(person->is_military_base_resident());
	military++;
	// some military leave each each year
	if(RANDOM() < this->military_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
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
  ready_to_move.clear();
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
    printf("NO MILITARY VACANCIES FOUND\n");
    return;
  }

  // find recruits to fill the dorms
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	int age = person->get_age();
	if(18 < age && age < 40 && person->get_number_of_children() == 0) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("MILITARY RECRUITS %d\n", (int)ready_to_move.size());

  if(ready_to_move.size() == 0) {
    printf("NO MILITARY RECRUITS FOUND\n");
    return;
  }

  // shuffle the recruits
  FYShuffle< pair<Person*, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < barracks_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = barracks_units[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED BARRACKS %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    // printf("RECRUIT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d MILITARY, CURRENT = %d  MAX = %d\n",day,moved,military+moved,military+barracks_vacancies);
}

void County::move_inmates_out_of_prisons(int day) {
  // printf("RELEASE PRISONERS =======================\n");
  ready_to_move.clear();
  int prisoners = 0;
  // find former prisoners still in jail
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_prison()) {
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN PRISON %s\n", person->get_id(), house->get_label());
	assert(person->is_prisoner());
	prisoners++;
	// some prisoners get out each year
	if(RANDOM() < this->prison_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d FORMER PRISONERS\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("DAY %d RELEASED %d PRISONERS, TOTAL NOW %d\n",day,moved,prisoners-moved);
}

void County::move_inmates_into_prisons(int day) {
  // printf("GENERATE NEW PRISON RESIDENTS =======================\n");
  ready_to_move.clear();
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
    printf("NO PRISON VACANCIES FOUND\n");
    return;
  }

  // find inmates to fill the jail_cells
  for(int i = 0; i < this->houses; ++i) {
    Household* house = this->households[i];
    if(house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if(hsize <= house->get_orig_size()) continue;
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	int age = person->get_age();
	if((18 < age && person->get_number_of_children() == 0) || (age < 50)) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("PRISON POSSIBLE INMATES %d\n", (int)ready_to_move.size());

  if(ready_to_move.size() == 0) {
    printf("NO INMATES FOUND\n");
    return;
  }

  // shuffle the inmates
  FYShuffle< pair<Person*, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for(int i = 0; i < jail_cell_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = jail_cell_units[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED JAIL_CELL %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person* person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    // printf("INMATE %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    this->occupants[oldhouse]--;
    this->occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d PRISONERS, CURRENT = %d  MAX = %d\n",day,moved,prisoners+moved,prisoners+jail_cell_vacancies);
}

void County::move_patients_into_nursing_homes(int day) {
  // printf("NEW NURSING HOME RESIDENTS =======================\n");
  ready_to_move.clear();
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
    printf("DAY %d ADDED %d NURSING HOME PATIENTS, TOTAL NOW %d BEDS = %d\n", day, 0, nursing_home_residents, beds);
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
	Person* person = house->get_housemate(j);
	int age = person->get_age();
	if(60 <= age) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("NURSING HOME POSSIBLE PATIENTS %d\n", (int)ready_to_move.size());

  // shuffle the patients
  FYShuffle< pair<Person*, int> >(ready_to_move);

  // pick the top of the list to move into nursing_home
  for(int i = 0; i < nursing_home_vacancies && ready_to_move.size() > 0; ++i) {
    int newhouse = nursing_home_units[i];
    Place* houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED NURSING_HOME UNIT %s ORIG %d SIZE %d\n", houseptr->get_label(),houseptr->get_orig_size(),houseptr->get_size());
    Person* person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place* ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
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
  printf("DAY %d ADDED %d NURSING HOME PATIENTS, CURRENT = %d  MAX = %d\n",day,moved,nursing_home_residents+moved,beds);
}

void County::move_young_adults(int day) {
  // printf("MOVE YOUNG ADULTS =======================\n");
  ready_to_move.clear();

  // find young adults in overfilled units who are ready to move out
  for(int i = 0; i < this->houses; ++i) {
    if(this->occupants[i] > this->beds[i]) {
      Household* house = this->households[i];
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	Person* person = house->get_housemate(j);
	int age = person->get_age();
	if(18 <= age && age < 30) {
	  if(RANDOM() < this->youth_home_departure_rate) {
	    ready_to_move.push_back(make_pair(person,i));
	    // printf("READY: PERSON %d AGE %d OLDHOUSE %d\n", person->get_id(), person->get_age(),i);
	  }
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE young adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("MOVED %d YOUNG ADULTS =======================\n",moved);
}


void County::move_older_adults(int day) {
  // printf("MOVE OLDER ADULTS =======================\n");
  ready_to_move.clear();

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
	int age = house->get_housemate(j)->get_age();
	if(age > max_age) {
	  max_age = age; pos = j;
	}
	if(age > 20) {
	  adults++;
	}
      }
      if(adults > 1) {
	Person* person = house->get_housemate(pos);
	if(RANDOM() < this->adult_home_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE older adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("MOVED %d OLDER ADULTS =======================\n",moved);
}

void County::report_ages(int day, int house_id) {
  Household* house = this->households[house_id];
  printf("HOUSE %d BEDS %d OCC %d AGES ",
	 house->get_id(), this->beds[house_id], this->occupants[house_id]);
  int hsize = house->get_size();
  for(int j = 0; j < hsize; ++j) {
    int age = house->get_housemate(j)->get_age();
    printf("%d ", age);
  }
}


void County::swap_houses(int day) {
  printf("SWAP HOUSES =======================\n");

  // two-dim array of vectors of imbalanced houses
  HouselistT** houselist;
  houselist = new HouselistT* [13];
  for(int i = 0; i < 13; ++i) {
    houselist[i] = new HouselistT [13];
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
    for(int j = i+1; j < 10; ++j) {
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
      printf("DAY %d PROBLEM HOUSE %d BEDS %d OCC %d AGES ",
	     day, house->get_id(), beds[i], occupants[i]);
      int hsize = house->get_size();
      for(int j = 0; j < hsize; ++j) {
	int age = house->get_housemate(j)->get_age();
	printf("%d ", age);
      }
      printf("\n");
    }
  }

  // make lists of overfilled houses
  vector<int>* overfilled;
  overfilled = new vector<int> [max_beds+1];
  for(int i = 0; i <= max_beds; ++i) {
    overfilled[i].clear();
  }
  
  // make lists of underfilled houses
  vector<int>* underfilled;
  underfilled = new vector<int> [max_beds + 1];
  for(int i = 0; i <= max_beds; ++i) {
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
      underfilled[beds[i]].push_back(i);
    }
  }

  int count[100];
  for(int i = 0; i <= max_beds; ++i) {
    for (int j = 0; j <= max_beds+1; ++j) {
      count[j] = 0;
    }
    for(int k = 0; k < (int) overfilled[i].size(); ++k) {
      int kk = overfilled[i][k];
      int occ = this->occupants[kk];
      if (occ <= max_beds + 1) {
	count[occ]++;
      } else {
	count[max_beds+1]++;
      }
    }
    for(int k = 0; k < (int) underfilled[i].size(); ++k) {
      int kk = underfilled[i][k];
      int occ = this->occupants[kk];
      if(occ <= max_beds+1) {
	count[occ]++;
      } else {
	count[max_beds+1]++;
      }
    }
    printf("DAY %4d BEDS %2d ", day, i);
    for(int j = 0; j <= max_beds+1; ++j) {
      printf("%3d ", count[j]);
    }
    printf("\n");
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
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) ",
	      c, count[c], (100.0 * count[c]) / total);
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
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) %0.2f ",
	      c, count[c], (100.0 * count[c]) / total, count[c] ? ((double)hsize[c] / (double)count[c]) : 0.0);
    }
    fprintf(Global::Statusfp, "\n");
  }
  return;
}
