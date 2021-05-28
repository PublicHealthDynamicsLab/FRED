/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: County.h
//

#ifndef _FRED_COUNTY_H
#define _FRED_COUNTY_H

#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>

#include "Demographics.h"
#include "Admin_Division.h"

class Household;
class Person;
class Place;

// 2-d array of lists
typedef std::vector<int> HouselistT;

// number of age groups: 0-4, 5-9, ... 85+
#define AGE_GROUPS 18

// number of target years: 2010, 2015, ... 2040
#define TARGET_YEARS 7

class County : public Admin_Division {
 public:

  County(int _admin_code);
  ~County();

  void setup();

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
  void recompute_county_popsize();

  void update(int day);
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
  int find_admin_code(int n);
  void get_housing_data();
  void report_household_distributions();
  void report_county_population();

  double get_mortality_rate(int age, char sex) { 
    if(sex == 'F') {
      if(age > Demographics::MAX_AGE) {
        return this->female_mortality_rate[Demographics::MAX_AGE];
      } else {
        return this->female_mortality_rate[age];
      }
    } else {
      if(age > Demographics::MAX_AGE) {
        return this->male_mortality_rate[Demographics::MAX_AGE];
      } else {
        return this->male_mortality_rate[age];
      }
    }
  } 

  // for selecting new workplaces
  void set_workplace_probabilities();
  Place* select_new_workplace();
  void report_workplace_sizes();

  // for selecting new schools
  void set_school_probabilities();
  Place* select_new_school(int grade);
  Place* select_new_school_in_county(int grade);
  void report_school_sizes();

  // county migration
  void read_population_target_properties();
  void county_to_county_migration();
  void migrate_household_to_county(Place* house, int dest);
  Place* select_new_house_for_immigrants(int hszie);
  void select_migrants(int day, int migrants, int lower_age, int upper_age, char sex, int dest_admin_code);
  void add_immigrant(Person* person);
  void add_immigrant(int age, char sex);
  void migration_swap_houses();
  void migrate_to_target_popsize();
  void read_migration_properties();
  double get_migration_rate(int sex, int age, int src, int dst);
  void group_population_by_sex_and_age(int reset);
  void report();
  void move_students();

  Household* get_hh(int i);

  static int get_number_of_counties() {
    return County::counties.size();
  }

  static County* get_county_with_index(int n) {
    return County::counties[n];
  }

  static County* get_county_with_admin_code(int county_admin_code);

  static void setup_counties();
  static void move_students_in_counties();

 private:

  int tot_current_popsize;
  int male_popsize[Demographics::MAX_AGE + 2];
  int tot_male_popsize;
  int female_popsize[Demographics::MAX_AGE + 2];
  int tot_female_popsize;

  double male_mortality_rate[Demographics::MAX_AGE + 2];
  double female_mortality_rate[Demographics::MAX_AGE + 2];
  bool is_initialized;
  double college_departure_rate;
  double military_departure_rate;
  double prison_departure_rate;
  double youth_home_departure_rate;
  double adult_home_departure_rate;
  int* beds;
  int* occupants;
  int max_beds;
  std::vector< pair<Person*, int> > ready_to_move;
  int target_males[AGE_GROUPS][TARGET_YEARS];
  int target_females[AGE_GROUPS][TARGET_YEARS];
  person_vector_t males_of_age[Demographics::MAX_AGE+1];
  person_vector_t females_of_age[Demographics::MAX_AGE+1];
  int number_of_households;

  // pointers to nursing homes
  std::vector<Household*> nursing_homes;
  int number_of_nursing_homes;

  // schools attended by people in this county, with probabilities
  place_vector_t schools_attended[Global::GRADES];
  std::vector<double> school_probabilities[Global::GRADES];

  // workplaces attended by people in this county, with probabilities
  place_vector_t workplaces_attended;
  std::vector<double> workplace_probabilities;

  std::vector<int> migration_households;  //vector of household IDs for migration
 
  // static vars
  static bool enable_migration_to_target_popsize;
  static bool enable_county_to_county_migration;
  static bool enable_within_state_school_assignment;
  static bool enable_within_county_school_assignment;
  static std::vector<int> migration_admin_code;
  static double**** migration_rate;
  static int migration_properties_read;
  static int population_target_properties_read;
  static int*** male_migrants;
  static int*** female_migrants;
  static string projection_directory;

  static std::random_device rd;
  static std::mt19937_64 mt_engine;

  static std::vector<County*> counties;
  static std::unordered_map<int,County*> lookup_map;

};

#endif // _FRED_COUNTY_H


