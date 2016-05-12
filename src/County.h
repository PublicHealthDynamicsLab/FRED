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

#include <vector>
#include <unordered_map>
using namespace std;

#include "Demographics.h"
class Household;
class Person;
class Place;
class School;
class Workplace;

// for lists of schools
#define GRADES 20

// 2-d array of lists
typedef std::vector<int> HouselistT;


class County {
public:

  County(int _fips);
  ~County();

  int get_fips() {
    return this->fips;
  }

  int get_current_popsize() {
    return this->tot_current_popsize;
  }

  int get_tot_female_popsize() {
    return this->tot_female_popsize;
  }

  int get_tot_male_popsize() {
    return this->tot_male_popsize;
  }

  int get_current_popsize(int age) {
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    if(age >= 0) {
      return this->female_popsize[age] + this->male_popsize[age];
    }
    return -1;
  }

  int get_current_popsize(int age, char sex) {
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    if(age >= 0) {
      if(sex == 'F') {
        return this->female_popsize[age];
      } else if(sex == 'M') {
        return this->male_popsize[age];
      }
    }
    return -1;
  }

  int get_current_popsize(int age_min, int age_max, char sex);

  bool increment_popsize(Person* person);

  bool decrement_popsize(Person* person);

  void add_household(Household* h) {
    this->households.push_back(h);
  }
  
  void update(int day);
  void set_initial_popsize(int popsize) {
    this->target_popsize = popsize;
  }

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
  void report_county_population();
  double get_pregnancy_rate(int age) {
    return this->pregnancy_rate[age];
  }

  double get_mortality_rate(int age, char sex) { 
    if(sex == 'F') {
      if(age > Demographics::MAX_AGE) {
        return this->adjusted_female_mortality_rate[Demographics::MAX_AGE];
      } else {
        return this->adjusted_female_mortality_rate[age];
      }
    } else {
      if(age > Demographics::MAX_AGE) {
        return this->adjusted_male_mortality_rate[Demographics::MAX_AGE];
      } else {
        return this->adjusted_male_mortality_rate[age];
      }
    }
  } 

  void external_migration();
  void county_to_county_migration();
  void migrate_household_to_county(Place* house, int dest);
  Place* select_new_house_for_immigrants(int hszie);
  void select_migrants(int day, int migrants, int lower_age, int upper_age, char sex, int dest);
  void select_migrants(int day, int migrants, int lower_age, int upper_age, char sex);
  void add_immigrant(Person* person);
  void add_immigrant(int age, char sex);
  void report_age_distribution();
  void migration_swap_houses();
  void migrate_to_target();
  void read_migration_parameters();
  double get_migration_rate(int sex, int age, int src, int dst);

  void read_population_target_parameters();
  void clear_population_target_parameters();
  int get_population_target(int sex, int age, int fips, int year);
  int set_population_target(int sex, int age, int fips, int year);

  void set_workplace_probabilities();
  Workplace* select_new_workplace();
  void report_workplace_sizes();

  void set_school_probabilities();
  School* select_new_school(int grade);
  void report_school_sizes();

private:
  int fips;
  int tot_current_popsize;
  int male_popsize[Demographics::MAX_AGE + 1];
  int tot_male_popsize;
  int female_popsize[Demographics::MAX_AGE + 1];
  int tot_female_popsize;
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
  int* beds;
  int* occupants;
  int max_beds;
  int max_occupants;
  std::vector< pair<Person*, int> > ready_to_move;
  int male_targets[17][7];
  int female_targets[17][7];
  int enable_within_state_school_assignment;


  // pointers to households
  std::vector<Household*> households;
  int houses;

  // schools attended by people in this county, with probabilities
  std::vector<School*> schools_attended[GRADES];
  std::vector<double> school_probabilities[GRADES];

  // workplaces attended by people in this county, with probabilities
  std::vector<Workplace*> workplaces_attended;
  std::vector<double> workplace_probabilities;

  // migration arrays
  static int*** male_migrants;
  static int*** female_migrants;

  // county to county migration arrays
  static int** county_male_migrants;
  static int** county_female_migrants;

  std::vector<int> migration_households;  //vector of household IDs for migration
 
  // static vars
  static std::vector<int> migration_fips;
  static double**** migration_rate;
  static int migration_parameters_read;
  static int population_target_parameters_read;

};

#endif // _FRED_COUNTY_H
