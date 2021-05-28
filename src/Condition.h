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
// File: Condition.h
//

#ifndef _FRED_Condition_H
#define _FRED_Condition_H

#include "Global.h"

class Epidemic;
class Expression;
class Natural_History;
class Network;
class Person;
class Preference;
class Place;
class Rule;
class Transmission;


class Condition {
public:

  /**
   * Default constructor
   */
  Condition();
  ~Condition();

  void get_properties(int condition, string name);

  /**
   * Set all of the attributes for the Condition
   */
  void setup();

  void setup(int phase);
  void prepare();

  static void initialize_person(Person* person);

  void initialize_person(Person* person, int day);

  /**
   * @return this Condition's id
   */
  int get_id() {
    return this->id;
  }

  /**
   * @return the transmissibility
   */
  double get_transmissibility() {
    return this->transmissibility;
  }

  void set_transmissibility(double value) {
    this->transmissibility = value;
  }

  /**
   * @return the Epidemic's attack ratio
   * @see Epidemic::get_attack_rate()
   */
  double get_attack_rate();

  void report(int day);

  /**
   * @return the epidemic with which this Condition is associated
   */
  Epidemic* get_epidemic() {
    return this->epidemic;
  }

  void increment_group_state_count(int group_type_id, Group* place, int state);

  void decrement_group_state_count(int pgroup_type_id, Group* place, int state);

  /**
   * @return the probability that agent's will stay home
   */
  static double get_prob_stay_home();

  /**
   * @property the new probability that agent's will stay home
   */
  static void set_prob_stay_home(double);

  void increment_cohort_host_count(int day);
 
  void update(int day, int hour);

  void terminate_person(Person* person, int day);

  char* get_name() {
    return this->condition_name;
  }

  Natural_History* get_natural_history() {
    return this->natural_history;
  }

  bool is_external_update_enabled();

  int get_condition_to_transmit(int state);

  bool state_gets_external_updates(int state);

  // transmission mode

  char* get_transmission_mode() {
    return this->transmission_mode;
  }

  Transmission* get_transmission() {
    return this->transmission;
  }

  int get_number_of_states();

  string get_state_name(int i);

  int get_state_from_name(string state_name);

  void finish();

  void track_group_state_counts(int type_id, int state);

  int get_incidence_group_state_count(Group* group, int state) { return 0; }

  int get_current_group_state_count(Group* group, int state);

  int get_total_group_state_count(Group* group, int state);

  int get_incidence_count(int state);
  int get_current_count(int state);
  int get_total_count(int state);

  int get_place_type_to_transmit();

  bool health_records_are_enabled();

  Network* get_transmission_network() {
    return this->transmission_network;
  }

  rule_vector_t get_action_rules(int state);

  bool is_absent(int state, int group_type_id);
  bool is_closed(int state, int group_type_id);

  static void include_condition(string cond) {
    int size = Condition::condition_names.size();
    for (int i = 0; i < size; i++) {
      if (cond == Condition::condition_names[i]) {
	return;
      }
    }
    Condition::condition_names.push_back(cond);
  }

  static void exclude_condition(string cond) {
    int size = Condition::condition_names.size();
    for (int i = 0; i < size; i++) {
      if (cond == Condition::condition_names[i]) {
	for (int j = i+1; j < size; j++) {
	  Condition::condition_names[j-1] = Condition::condition_names[j];	  
	}
	Condition::condition_names.pop_back();
      }
    }
  }

  static void get_condition_properties();

  static void setup_conditions();

  static Condition* get_condition(int condition_id) {
    return Condition::conditions[condition_id];
  }

  static Condition* get_condition(string condition_name);

  static int get_condition_id(string condition_name);

  static string get_name(int condition_id) {
    return Condition::condition_names[condition_id];
  }

  static int get_number_of_conditions() { 
    return Condition::number_of_conditions;
  }

  static void prepare_conditions();

  static void prepare_to_track_group_state_counts();

  static void finish_conditions();

private:

  // condition identifiers
  int id;
  char condition_name[FRED_STRING_SIZE];

  // the course of the condition within a host
  Natural_History* natural_history;

  // how the condition spreads
  char transmission_mode[FRED_STRING_SIZE];
  Transmission* transmission;

  // the transmission network
  char transmission_network_name[FRED_STRING_SIZE];
  Network* transmission_network;

  // contagiousness
  double transmissibility;
  
  // the course of infection at the population level
  Epidemic* epidemic;

  static std::vector <Condition*> conditions;
  static std::vector<string> condition_names;
  static int number_of_conditions;
};

#endif // _FRED_Condition_H
