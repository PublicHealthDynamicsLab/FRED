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
// File: Epidemic.h
//

#ifndef _FRED_EPIDEMIC_H
#define _FRED_EPIDEMIC_H

#include "Global.h"
#include "Events.h"
#include "Person.h"
#include "Place.h"

class Group;
class Condition;
class Natural_History;

struct person_id_compare {
  bool operator()(const Person* x, const Person* y) const {
    return x->get_id() < y->get_id();
  }
};

struct place_id_compare {
  bool operator()(const Place* x, const Place* y) const {
    return x->get_id() < y->get_id();
  }
};

typedef  std::set<Person*, person_id_compare> person_set_t;
typedef  person_set_t::iterator person_set_iterator;

typedef  std::set<Place*, place_id_compare> place_set_t;
typedef  place_set_t::iterator place_set_iterator;

typedef std::unordered_map<Group*,int> group_counter_t;



class VIS_Location {
public:
  VIS_Location(double _lat, double _lon) {
    lat = _lat;
    lon = _lon;
  }
  double get_lat() { return this->lat;}
  double get_lon() { return this->lon;}
private:
  double lat;
  double lon;
};

typedef std::vector<VIS_Location*> vis_loc_vec_t;

class Epidemic {
public:

  /**
   * This static factory method is used to get an instance of a specific
   * Epidemic Model.  Depending on the model property, it will create a
   * specific Epidemic Model and return a pointer to it.
   *
   * @property a string containing the requested Epidemic model type
   * @return a pointer to a Epidemic model
   */
  static Epidemic* get_epidemic(Condition* condition);

  Epidemic(Condition* condition);
  ~Epidemic();
 
  void setup();
  void prepare_to_track_counts();
  void prepare();
  void initialize_person(Person* person, int day);
  void create_visualization_data_directories();
  void report(int day);
  void print_stats(int day);
  void print_visualization_data(int day);
  void report_serial_interval(int day);

  void get_imported_list(double_vector_t id_list);
  void select_imported_cases(int day, int max_imported, double per_cap, double lat, double lon, double radius, long long int admin_code, double min_age, double max_age, bool count_all);
  void become_exposed(Person* person, int day, int hour);
  void become_active(Person* person, int day);
  void inactivate(Person* person, int day, int hour);

  void update(int day, int hour);
  void prepare_for_new_day(int day);
  void update_state(Person* person, int day, int hour, int new_state, int loop_counter);

  void update_proximity_transmissions(int day, int hour);
  void find_active_places_of_type(int day, int hour, int place_type);
  void transmission_in_active_places(int day, int hour, int time_block);

  void update_network_transmissions(int day, int hour);

  int get_number_of_transmissible_people() {
    return this->transmissible_people_list.size();
  }

  double get_RR() {
    return this->RR;
  }

  void increment_cohort_host_count(int cohort_day) {
    ++(this->number_infected_by_cohort[cohort_day]);
  }

  int get_id() {
    return this->id;
  }

  double get_attack_rate();

  void delete_from_epidemic_lists(Person* person);

  int get_incidence_count(int state) {
    if (Global::Simulation_Day < 1) {
      return 0;
    }
    return this->daily_incidence_count[state][Global::Simulation_Day-1];
  }

  int get_current_count(int state) {
    return this->current_count[state];
  }

  int get_total_count(int state) {
    return this->total_count[state];
  }

  void track_group_state_counts(int type_id, int state) {
    if (type_id < 0) {
      return;
    }
    if (type_id < Group_Type::get_number_of_group_types()) {
      this->track_counts_for_group_state[state][type_id] = true;
    }
  }
  void increment_group_state_count(int place_type_id, Group* group, int state);
  void decrement_group_state_count(int place_type_id, Group* group, int state);
  void inc_state_count(Person* person, int state);
  void dec_state_count(Person* person, int state);
  int get_group_state_count(Group* group, int state);
  int get_total_group_state_count(Group* group, int state);
  bool health_records_are_enabled() {
    return this->enable_health_records;
  }

  void finish();
  void terminate_person(Person* person, int day);

protected:
  Condition* condition;
  char name[FRED_STRING_SIZE];
  int id;
  
  // boolean flags
  bool report_generation_time;

  // prevalence lists of people
  person_set_t active_people_list;
  person_set_t transmissible_people_list;

  // list of people with new status
  person_vector_t new_exposed_people_list;

  // places attended today by transmissible people:
  place_set_t active_places_list;

  // visualization data
  char visualization_directory[FRED_STRING_SIZE];

  // list of household locations of case_fatalities for each day
  vis_loc_vec_t vis_case_fatality_loc_list;

  // list of household locations of dormant states for each day
  vis_loc_vec_t* vis_dormant_loc_list;

  // visualization data
  bool* visualize_state;	  // if true, collect visualization data
  int* visualize_state_place_type; // place_type id of loc to visualize
  bool enable_visualization;

  // import agent
  Person* import_agent;

  // population health state counters
  int total_cases;
  int susceptible_count;

  // epidemic counters for each state of the condition
  int* incidence_count;		// number of people entering state today
  int* total_count;	       // number of people ever entering
  int* current_count;	  // number of people currently in state
  int** daily_incidence_count; // number of people entering state each day
  int** daily_current_count;	// number of people currently in state each day
  std::vector<group_counter_t> group_state_count;
  std::vector<group_counter_t> total_group_state_count;
  bool** track_counts_for_group_state;

  // used for computing reproductive rate:
  double RR;
  int* daily_cohort_size;
  int* number_infected_by_cohort;

  // serial interval
  double total_serial_interval;
  int total_secondary_cases;

  // link to natural history
  Natural_History* natural_history;

  // event queues
  int number_of_states;
  Events state_transition_event_queue;
  Events meta_agent_transition_event_queue;

  // report detailed changes in health records for this epidemic
  bool enable_health_records;

  // networks that support transmission of this condition
  network_vector_t transmissible_networks;

};

#endif // _FRED_EPIDEMIC_H
