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
// File: County.h
//

#ifndef _FRED_COUNTY_H
#define _FRED_COUNTY_H

#include "Demographics.h"
#include <vector>
class Person;
class Household;

using namespace std;

// 2-d array of lists
typedef std::vector<int> HouselistT;

class County {
public:

  County(int _fips);
  ~County();

  int get_fips() { return fips; }
  int get_popsize() { return current_popsize; }
  void increment_popsize() { current_popsize++; }
  void decrement_popsize() { current_popsize--; }
  void add_household(Household * h) { households.push_back(h); }
  
  void update(int day);
  void set_initial_popsize(int popsize) { target_popsize = popsize; }
  void update_population_dynamics(int day);
  void get_housing_imbalance(int day);
  int fill_vacancies(int day);
  void update_housing(int day);
  void move_college_students_out_of_dorms(int day);
  void move_college_students_into_dorms(int day);
  void move_military_personnel_out_of_barracks(int day);
  void move_military_personnel_into_barracks(int day);
  void move_inmates_out_of_prisons(int day);
  void move_inmates_into_prisons(int day);
  void move_patients_into_nursing_homes(int day);
  void move_young_adults(int day);
  void move_older_adults(int day);
  void report_ages(int day, int house_id);
  void swap_houses(int day);
  int find_fips_code(int n);
  int get_housing_data(int* target_size, int* current_size);
  void report_household_distributions();
  double get_pregnancy_rate(int age) { return pregnancy_rate[age]; } 
  double get_mortality_rate(int age, char sex) { 
    if (sex == 'F') {
      return adjusted_female_mortality_rate[age]; 
    }
    else {
      return adjusted_male_mortality_rate[age]; 
    }
  } 

private:
  int fips;
  int current_popsize;
  int target_popsize;
  double male_mortality_rate[Demographics::MAX_AGE + 1];
  double female_mortality_rate[Demographics::MAX_AGE + 1];
  double mortality_rate_adjustment_weight;
  double adjusted_male_mortality_rate[Demographics::MAX_AGE + 1];
  double adjusted_female_mortality_rate[Demographics::MAX_AGE + 1];
  double birth_rate[Demographics::MAX_AGE + 1];
  double adjusted_birth_rate[Demographics::MAX_AGE + 1];
  double pregnancy_rate[Demographics::MAX_AGE + 1];
  bool is_initialized;
  double population_growth_rate;
  double college_departure_rate;
  double military_departure_rate;
  double prison_departure_rate;
  double youth_home_departure_rate;
  double adult_home_departure_rate;
  int * beds;
  int * occupants;
  int max_beds;
  int max_occupants;
  std::vector< pair<Person*, int> > ready_to_move;

  // pointers to households
  std::vector<Household*> households;
  int houses;

};

#endif // _FRED_COUNTY_H
