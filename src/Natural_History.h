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

#ifndef _FRED_NATURAL_HISTORY_H
#define _FRED_NATURAL_HISTORY_H

#include "Global.h"

class Condition;
class Expression;
class Preference;
class Person;
class State_Space;
class Rule;

typedef long long int longint;

class Natural_History {

public:

  Natural_History();

  ~Natural_History();

  void setup(Condition *condition);

  void get_properties();

  void setup(int phase);

  void prepare();

  void prepare_rules();

  void compile_rules();

  void print();

  Expression* get_duration_expression(int state) {
    return this->duration_expression[state];
  }

  Expression* get_edge_expression(int state) {
    return this->edge_expression[state];
  }

  char* get_name();

  int get_number_of_states() {
    return this->number_of_states;
  }

  std::string get_state_name(int i);

  int get_next_state(Person* person, int state);

  int select_next_state(int state, double* transition_prob);

  int get_next_transition_step(Person* person, int state, int day, int hour);

  int get_exposed_state() {
    return this->exposed_state;
  }

  bool is_maternity_state(int state) {
    return this->maternity_state[state];
  }

  bool is_fatal_state(int state) {
    return this->fatal_state[state];
  }

  bool is_dormant_state(int state) {
    return (this->state_is_dormant[state] == 1);
  }

  double get_transmissibility() {
    return this->transmissibility;
  }

  int get_condition_to_transmit(int state) {
    return this->condition_to_transmit[state];
  }

  bool is_external_update_enabled() {
    return this->enable_external_update;
  }

  bool state_gets_external_updates(int state) {
    return this->update_vars_externally[state];
  }

  int get_place_type_to_join(int state) {
    return this->place_type_to_join[state];
  }

  int get_place_type_to_quit(int state) {
    return this->place_type_to_quit[state];
  }

  rule_vector_t get_action_rules(int state) {
    return this->action_rules[state];
  }

  int get_network_type(int state) {
    return this->network_type[state];
  }

  double get_network_mean_degree(int state) {
    return this->network_mean_degree[state];
  }

  int get_network_max_degree(int state) {
    return this->network_max_degree[state];
  }

  int should_start_hosting(int state) {
    return this->start_hosting[state];
  }
  void finish() {}

  int get_place_type_to_transmit() {
    return this->place_type_to_transmit;
  }

  int get_import_start_state() {
    return this->import_start_state;
  }

  int get_import_count(int state) {
    return this->import_count[state];
  }

  double get_import_per_capita_transmissions(int state) {
    return this->import_per_capita_transmissions[state];
  }

  double get_import_latitude(int state) {
    return this->import_latitude[state];
  }

  double get_import_longitude(int state) {
    return this->import_longitude[state];
  }

  double get_import_radius(int state) {
    return this->import_radius[state];
  }

  double get_import_min_age(int state) {
    return this->import_min_age[state];
  }

  double get_import_max_age(int state) {
    return this->import_max_age[state];
  }

  longint get_import_admin_code(int state) {
    return this->import_admin_code[state];
  }

  bool is_absent(int state, int group_type_id);

  bool is_closed(int state, int group_type_id);

  Rule* get_import_count_rule(int state) {
    return this->import_count_rule[state];
  }

  Rule* get_import_per_capita_rule(int state) {
    return this->import_per_capita_rule[state];
  }

  Rule* get_import_ages_rule(int state) {
    return this->import_ages_rule[state];
  }

  Rule* get_import_location_rule(int state) {
    return this->import_location_rule[state];
  }

  Rule* get_import_admin_code_rule(int state) {
    return this->import_admin_code_rule[state];
  }

  Rule* get_import_list_rule(int state) {
    return this->import_list_rule[state];
  }

  bool all_import_attempts_count(int state) {
    return this->count_all_import_attempts[state];
  }

protected:

  Condition* condition;
  char name[FRED_STRING_SIZE];
  int id;

  // STATE MODEL
  State_Space* state_space;
  int number_of_states;

  // RULES
  rule_vector_t* action_rules;
  Rule** wait_rule;
  Rule* exposure_rule;
  rule_vector_t** next_rules;
  Rule** default_rule;

  // STATE SIDE EFFECTS
  Rule** susceptibility_rule;
  Rule** transmissibility_rule;
  Expression** edge_expression;
  int* condition_to_transmit;
  int* place_type_to_join;
  int* place_type_to_quit;
  int* network_action;
  int* network_type;
  double* network_mean_degree;
  int* network_max_degree;
  int* start_hosting;
  bool* maternity_state;
  bool* fatal_state;

  // PERSONAL VARIABLES
  bool* update_vars;
  bool* update_vars_externally;
  bool enable_external_update;

  // IMPORT STATE
  int import_start_state;
  int* import_count;
  double* import_per_capita_transmissions;
  double* import_latitude;
  double* import_longitude;
  double* import_radius;
  longint* import_admin_code;
  double* import_min_age;
  double* import_max_age;
  Rule** import_count_rule;
  Rule** import_per_capita_rule;
  Rule** import_ages_rule;
  Rule** import_location_rule;
  Rule** import_admin_code_rule;
  Rule** import_list_rule;
  bool* count_all_import_attempts;

  // STATE CONTACT RESTRICTIONS
  bool** absent_groups;
  bool** close_groups;

  // TRANSMISSIBILITY
  double transmissibility;
  double R0;
  double R0_a;
  double R0_b;
  int place_type_to_transmit;
  int exposed_state;

  // TRANSITION MODEL
  Expression** duration_expression;
  int* transition_day;
  std::string* transition_date;
  int* transition_days;
  int* transition_hour;
  int* default_next_state;
  int* state_is_dormant;

};

#endif
