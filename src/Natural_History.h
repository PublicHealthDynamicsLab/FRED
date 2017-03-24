/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_NATURAL_HISTORY_H
#define _FRED_NATURAL_HISTORY_H

#include <string>
#include <vector>
using namespace std;

#include "Global.h"

class Condition;
class Logit;
class Person;
class State_Space;


class Natural_History {

public:

  Natural_History();

  ~Natural_History();

  static Natural_History* get_new_natural_history(char* natural_history_model);

  virtual void setup(Condition *condition);

  virtual void get_parameters();

  void prepare();

  void print();

  virtual int get_symptoms_level(int s) {
    return state_symptoms_level[s];
  }

  virtual double get_infectivity(int s) {
    return state_infectivity[s];
  }

  virtual double get_susceptibility(int s) {
    return state_susceptibility[s];
  }

  virtual double get_fatality(int s) {
    return state_fatality[s];
  }

  virtual bool is_fatal(int s) { return false; }

  virtual bool is_fatal(Person* person, int s);

  char* get_name();

  int get_number_of_states() {
    return this->number_of_states;
  }

  std::string get_state_name(int i);

  int get_initial_state(Person* person);

  int get_next_state(Person* person, int state);

  int get_next_state(int state, double* transition_prob);

  double get_transition_period(int state);

  void read_transition_matrix();

  int get_exposed_state() {
    return this->exposed_state;
  }

  bool is_recovered_state(int state) {
    return (this->state_is_recovered[state] == 1);
  }

  double get_min_age() {
    return this->min_age;
  }

  double get_max_age() {
    return this->max_age;
  }

  int get_symptoms_level_from_string(char* level_str) {
    if (strcmp(level_str, "none")==0) {
      return Global::NO_SYMPTOMS;
    }
    if (strcmp(level_str, "mild")==0) {
      return Global::MILD_SYMPTOMS;
    }
    if (strcmp(level_str, "moderate")==0) {
      return Global::MODERATE_SYMPTOMS;
    }
    if (strcmp(level_str, "severe")==0) {
      return Global::SEVERE_SYMPTOMS;
    }
    return -1;
  }

  string get_symptoms_string(int level) {
    switch (level) {
    case Global::NO_SYMPTOMS:
      return string("none");
    case Global::MILD_SYMPTOMS:
      return string("mild");
    case Global::MODERATE_SYMPTOMS:
      return string("moderate");
    case Global::SEVERE_SYMPTOMS:
      return string("severe");
    }
    return string("unknown");
  }

  virtual bool is_case_fatality_enabled() {
    return this->enable_case_fatality;
  }

  virtual bool is_fatal(double age, int symptoms_level, int days_symptomatic) {
    return false;
  }

  virtual bool is_fatal(Person* per, int symptoms_level, int days_symptomatic) {
    return false;
  }

  double get_transmissibility() {
    return this->transmissibility;
  }

  bool is_confined_to_household(Person* person);

  bool is_confined_to_household(int state);

  bool is_household_confinement_enabled(Person *person);

  bool is_confined_to_household_due_to_contact_tracing(Person *person);

  bool get_daily_household_confinement(Person* person);

  double get_contact_tracing_effectiveness() {
    return this->contact_tracing_effectiveness;
  }
  
  double get_contact_tracing_trigger() {
    return this->contact_tracing_trigger;
  }
  
  double get_transmission_modifier(int state, int condition_id) {
    return this->transmission_modifier[state][condition_id];
  }

  double get_susceptibility_modifier(int state, int condition_id) {
    return this->susceptibility_modifier[state][condition_id];
  }

  double get_symptoms_modifier(int state, int condition_id) {
    return this->symptoms_modifier[state][condition_id];
  }

  int get_state_modifier(int state, int condition_id, int src_state) {
    return this->state_modifier[state][condition_id][src_state];
  }

  void end_of_run() {}

protected:

  Condition* condition;
  char name[FRED_STRING_SIZE];
  int id;

  double transmissibility;
  double R0;
  double R0_a;
  double R0_b;

  State_Space* state_space;
  int number_of_states;

  std::vector<double>state_infectivity;
  std::vector<double>state_susceptibility;
  std::vector<int>state_symptoms_level;
  std::vector<double>state_fatality;
  std::vector<int>state_is_recovered;
  std::vector<double>probability_of_household_confinement;
  std::vector<double>decide_household_confinement_daily;

  double** transmission_modifier;
  double** susceptibility_modifier;
  double** symptoms_modifier;
  int*** state_modifier;

  double contact_tracing_effectiveness;
  int contact_tracing_trigger;

  int exposed_state;
  double min_age;
  double max_age;

  char initialization_model[256];
  double* initialization_probability;
  Logit* initialization_logit;

  Logit* immunity_logit;

  char transition_model[256];
  double** transition_matrix;
  double* transition_period_median;
  double* transition_period_dispersion;
  double* transition_period_offset;
  double* transition_period_upper_bound;
  Logit* onset_logit;
  Logit* transition_logit;

  char onset_model[256];

  int enable_case_fatality;
  char case_fatality_model[256];
  Logit* case_fatality_logit;

};

#endif
