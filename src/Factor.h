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

#ifndef _FRED_FACTOR_H
#define _FRED_FACTOR_H

#include <stdlib.h>
#include <string>

using namespace std;

class Person;

typedef double (*fptr_with_0_arg) ();
typedef double (*fptr_with_1_arg) (Person*);
typedef double (*fptr_with_2_arg) (Person*,int);
typedef double (*fptr_with_3_arg) (Person*,int,int);
typedef double (*fptr_with_4_arg) (Person*,int,int,int);
typedef double (*fptr_with_5_arg) (Person*,int,int,int,int);
typedef double (*fptr_with_6_arg) (Person*,int,int,int,int, int);
typedef double (*fptr_with_7_arg) (Person*,int,int,int,int, int, int);
typedef double (*fptr_with_8_arg) (Person*,int,int,int,int, int, int, int);
typedef double (*Fptr_with_2_arg) (Person*,Person*);
typedef double (*Fptr_with_3_arg) (Person*,Person*,int);

class Factor {
public:

  Factor(string s);
  bool parse();
  string get_name();
  double get_value(Person* person);
  double get_value(Person* person1, Person* person2);
  bool is_warning() {
    return this->warning;
  }
  
private:
  std::string name;
  double number;
  int number_of_args;
  int arg2, arg3, arg4, arg5, arg6, arg7, arg8;
  bool is_constant;
  bool warning;

  // function pointers
  fptr_with_0_arg f0;
  fptr_with_1_arg f1;
  fptr_with_2_arg f2;
  fptr_with_3_arg f3;
  fptr_with_4_arg f4;
  fptr_with_5_arg f5;
  fptr_with_6_arg f6;
  fptr_with_7_arg f7;
  fptr_with_8_arg f8;
  Fptr_with_2_arg F2;
  Fptr_with_3_arg F3;

  /// Factors drawn from a statistical distribution
  static double get_random();
  static double get_normal();
  static double get_exponential();

  /// Factors based on time and dates
  static double get_sim_day();
  static double get_sim_week();
  static double get_sim_month();
  static double get_sim_year();
  static double get_day_of_week();
  static double get_day_of_month();
  static double get_day_of_year();
  static double get_month();
  static double get_year();
  static double get_date();
  static double get_hour();
  static double get_epi_week();
  static double get_epi_year();

  /// Factors based on the agent's demographics
  static double get_id(Person* person);
  static double get_birth_year(Person* person);
  static double get_age_in_days(Person* person);
  static double get_age_in_weeks(Person* person);
  static double get_age_in_months(Person* person);
  static double get_age_in_years(Person* person);
  static double get_age(Person* person);
  static double get_sex(Person* person);
  static double get_race(Person* person);
  static double get_profile(Person* person);
  static double get_household_relationship(Person* person);
  static double get_number_of_children(Person* person);

  /// Factors based on agent's current state
  static double get_current_state(Person* person, int condition_id);
  static double get_time_since_entering_state(Person* person, int condition_id, int state);
  static double get_susceptibility(Person* person, int condition_id);
  static double get_transmissibility(Person* person, int condition_id);
  static double get_transmissions(Person* person, int condition_id);
  static double get_var(Person* person, int var_index, int is_global);
  static double get_list_size(Person* person, int list_var_index, int is_global);
  static double get_id_of_transmission_source(Person* person, int condition_id);

  /// Factors based on other agents in same group
  static double get_state_count(Person* person, int verb, int is_count,
			 int group_type_id, int condition_id, int state, int except_me);
  static double get_sum_of_vars_in_group(Person* person, int var_id, int group_type_id);
  static double get_ave_of_vars_in_group(Person* person, int var_id, int group_type_id);

  /// Factors based on groups
  static double get_group_id(Person* person, int group_type_id);
  static double get_admin_id(Person* person, int group_type_id);
  static double get_group_level(Person* person, int selection, int place_type_id);
  static double get_adi_state_rank(Person* person, int place_type_id);
  static double get_adi_national_rank(Person* person, int place_type_id);
  static double get_block_group_admin_code(Person* person, int place_type_id);
  static double get_census_tract_admin_code(Person* person, int place_type_id);
  static double get_county_admin_code(Person* person, int place_type_id);
  static double get_state_admin_code(Person* person, int place_type_id);

  /// Factors based on network
  static double get_network_in_degree(Person* person, int network_type_id);
  static double get_network_out_degree(Person* person, int network_type_id);
  static double get_network_degree(Person* person, int network_type_id);
  static double get_id_of_max_weight_inward_edge_in_network(Person* person, int network_type_id);
  static double get_id_of_max_weight_outward_edge_in_network(Person* person, int network_type_id);
  static double get_id_of_min_weight_inward_edge_in_network(Person* person, int network_type_id);
  static double get_id_of_min_weight_outward_edge_in_network(Person* person, int network_type_id);
  static double get_id_of_last_outward_edge_in_network(Person* person, int network_type_id);
  static double get_id_of_last_inward_edge_in_network(Person* person, int network_type_id);
  static double is_connected(Person* person1, Person* person2, int network_type_id);
  static double get_network_weight(Person* person1, Person* person2, int network_type_id);
  static double get_network_timestamp(Person* person1, Person* person2, int network_type_id);

};

#endif


