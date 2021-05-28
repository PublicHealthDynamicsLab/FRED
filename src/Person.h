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
// File: Person.h
//
#ifndef _FRED_PERSON_H
#define _FRED_PERSON_H

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>

using namespace std;

#include "Date.h"
#include "Demographics.h"
#include "Global.h"
#include "Link.h"
#include "Network_Type.h"
#include "Place_Type.h"
#include "Group_Type.h"
#include "Place.h"
#include "Utils.h"

class Activities_Tracking_Data;
class Condition;
class Expression;
class Factor;
class Group;
class Hospital;
class Household;
class Infection;
class Natural_History;
class Network;
class Place;
class Population;
class Preference;

typedef struct {
  int person_index;
  int person_id;
  Person* person;
  Expression* expression;
  std::vector<int> change_day;
  std::vector<double> value_on_day;
} report_t;


typedef struct {
  int state;
  int last_transition_step;
  int next_transition_step;

  // transmission info
  double susceptibility;
  double transmissibility;
  int exposure_day;
  Person* source;
  Group* group;
  int number_of_hosts;

  // time each state was last entered
  int* entered;

  // status flags
  bool is_fatal;
  bool sus_set;
  bool trans_set;

} condition_t;


// The following enum defines symbolic names for Insurance Company Assignment.
// The last element should always be UNSET.
namespace Insurance_assignment_index {
  enum e {
    PRIVATE,
    MEDICARE,
    MEDICAID,
    HIGHMARK,
    UPMC,
    UNINSURED,
    UNSET
  };
};



// household relationship codes (ver 2)
/**
 * The following enum defines symbolic names for an agent's relationship
 * to the householder.  The last element should always be
 * HOUSEHOLD_RELATIONSHIPS
 */
namespace Household_Relationship {
  enum e {
    HOUSEHOLDER = 0,
    SPOUSE = 1,
    CHILD = 2,
    SIBLING = 3,
    PARENT = 4,
    GRANDCHILD = 5,
    IN_LAW = 6,
    OTHER_RELATIVE = 7,
    BOARDER = 8,
    HOUSEMATE = 9,
    PARTNER = 10,
    FOSTER_CHILD = 11,
    OTHER_NON_RELATIVE = 12,
    INSTITUTIONALIZED_GROUP_QUARTERS_POP = 13,
    NONINSTITUTIONALIZED_GROUP_QUARTERS_POP = 14,
    HOUSEHOLD_RELATIONSHIPS
  };
};

/**
 * The following enum defines symbolic names for an agent's race.  to
 * the householder.  The last element should always be RACES
 */

namespace Race {
  enum e {
    UNKNOWN_RACE = -1,
    WHITE = 1,
    AFRICAN_AMERICAN,
    AMERICAN_INDIAN,
    ALASKA_NATIVE,
    TRIBAL,
    ASIAN,
    HAWAIIAN_NATIVE,
    OTHER_RACE,
    MULTIPLE_RACE,
    RACES
  };
};

#define MAX_MOBILITY_AGE 100

/**
 * The following enum defines symbolic names an agent's activity
 * profile.  The last element should always be ACTIVITY_PROFILE
 */
namespace Activity_Profile {
  enum e {
    INFANT,
    PRESCHOOL,
    STUDENT,
    TEACHER,
    WORKER,
    WEEKEND_WORKER,
    UNEMPLOYED,
    RETIRED,
    PRISONER,
    COLLEGE_STUDENT,
    MILITARY,
    NURSING_HOME_RESIDENT,
    UNDEFINED,
    ACTIVITY_PROFILE
  };
};




class Person {
public:

  Person();
  ~Person();
  void setup(int index, int id, int age, char sex, int race, int rel,
	     Place* house, Place* school, Place* work, int day,
	     bool today_is_birthday);
  void print(FILE* fp, int condition_id);
  bool is_present(int sim_day, Group* group);
  std::string to_string();
  int get_id() const {
    return this->id;
  }
  void set_pop_index(int idx) {
    this->index = idx;
  }
  int get_pop_index() {
    return this->index;
  }
  void start_reporting(int condition_id, int state_id);
  void start_reporting(Rule *rule);
  double get_x();
  double get_y();
  bool is_alive() const {
    return this->alive;
  }
  void terminate(int day);


  // DEMOGRAPHICS
  int get_birthday_sim_day() {
    return birthday_sim_day;
  }
  int get_birth_year() {
    return Date::get_year(this->birthday_sim_day);
  }
  double get_age_in_years() const;
  int get_age_in_days() const;
  int get_age_in_weeks() const;
  int get_age_in_months() const;
  double get_real_age() const;
  int get_age() const;
  short int get_init_age() const {
    return this->init_age;
  }
  const char get_sex() const {
    return this->sex;
  }
  short int get_race() const {
    return this->race;
  }
  void set_number_of_children(int n) {
    this->number_of_children = n;
  }
  int get_number_of_children() const {
    return this->number_of_children;
  }
  void update_birth_stats(int day, Person* self);
  bool is_adult() {
    return get_age() >= Global::ADULT_AGE;
  }
  bool is_child() {
    return get_age() < Global::ADULT_AGE;
  }
  bool is_white() {
    return get_race()==Race::WHITE;
  }
  bool is_nonwhite() {
    return get_race()!=Race::WHITE;
  }
  bool is_african_american() {
    return get_race()==Race::AFRICAN_AMERICAN;
  }
  bool is_american_indian() {
    return get_race()==Race::AMERICAN_INDIAN;
  }
  bool is_alaska_native() {
    return get_race()==Race::ALASKA_NATIVE;
  }
  bool is_hawaiian_native() {
    return get_race()==Race::HAWAIIAN_NATIVE;
  }
  bool is_tribal() {
    return get_race()==Race::TRIBAL;
  }
  bool is_asian() {
    return get_race()==Race::ASIAN;
  }
  bool is_other_race() {
    return get_race()==Race::OTHER_RACE;
  }
  bool is_multiple_race() {
    return get_race()==Race::MULTIPLE_RACE;
  }
  int get_income(); 
  int get_adi_state_rank();
  int get_adi_national_rank();
  char* get_household_structure_label();
  const bool is_deceased() const {
    return this->deceased;
  }
  void set_deceased() {
    this->deceased = true;
  }
  void set_household_relationship(int rel) {
    this->household_relationship = rel;
  }
  const int get_household_relationship() const {
    return this->household_relationship;
  }
  bool is_householder() {
    return this->household_relationship == Household_Relationship::HOUSEHOLDER;
  }
  void make_householder() {
    this->household_relationship = Household_Relationship::HOUSEHOLDER;
  }


  // PROFILE
  int get_profile() {
    return this->profile;
  }
  void set_profile(int _profile) {
    this->profile = _profile;
  }
  void assign_initial_profile();
  void update_profile_after_changing_household();
  void update_profile_based_on_age();
  bool become_a_teacher(Place* school);
  bool is_teacher() {
    return this->profile == Activity_Profile::TEACHER;
  }
  bool is_student() {
    return this->profile == Activity_Profile::STUDENT;
  }
  bool is_retired() {
    return this->profile == Activity_Profile::RETIRED;
  }
  bool is_employed() {
    return get_workplace() != NULL;
  }
  bool is_college_student() {
    return this->profile == Activity_Profile::COLLEGE_STUDENT;
  }
  bool is_prisoner() {
    return this->profile == Activity_Profile::PRISONER;
  }
  bool is_college_dorm_resident() {
    return this->profile == Activity_Profile::COLLEGE_STUDENT;
  }
  bool is_military_base_resident() {
    return this->profile == Activity_Profile::MILITARY;
  }
  bool is_nursing_home_resident() {
    return this->profile == Activity_Profile::NURSING_HOME_RESIDENT;
  }
  bool is_hospital_staff();
  bool is_prison_staff();
  bool is_college_dorm_staff();
  bool is_military_base_staff();
  bool is_nursing_home_staff();
  bool lives_in_group_quarters() {
    return (is_college_dorm_resident() || is_military_base_resident()
	    || is_prisoner() || is_nursing_home_resident());
  }
  bool lives_in_parents_home() {
    return this->in_parents_home;
  }
  static std::string get_profile_name(int prof);
  static int get_profile_from_name(std::string profile_name);


  // ACTIVITIES
  void setup_activities(Place* house, Place* school, Place* work);
  void set_activity_group(int place_type_id, Group* group);
  void prepare_activities() {}
  void clear_activity_groups();
  void unset_in_parents_home() {
    this->in_parents_home = false;
  }
  void schedule_activity(int day, int place_type_id);
  void cancel_activity(int day, int place_type_id);
  void begin_membership_in_activity_group(int i);
  void begin_membership_in_activity_groups();
  void end_membership_in_activity_group(int i);
  void end_membership_in_activity_groups();
  void store_activity_groups();
  void restore_activity_groups();
  int get_activity_group_id(int i);
  const char* get_activity_group_label(int i);
  void update_activities(int sim_day);
  void select_activity_of_type(int place_type_id);
  std::string activities_to_string();
  Group* get_activity_group(int i) {
    if (0 <= i && i < Group_Type::get_number_of_group_types()) { 
      return this->link[i].get_group();
    }
    else {
      return NULL;
    }
  }
  void set_household(Place* p) {
    set_activity_group(Group_Type::get_type_id("Household"), p);
  }
  void set_neighborhood(Place* p) {
    set_activity_group(Group_Type::get_type_id("Neighborhood"), p);
  }
  void set_school(Place* p) {
    set_activity_group(Group_Type::get_type_id("School"), p);
    if (p != NULL) {
      set_last_school(p);
    }
  }
  void set_last_school(Place* school);
  void set_classroom(Place* p) {
    set_activity_group(Group_Type::get_type_id("Classroom"), p);
  }
  void set_workplace(Place* p) {
    set_activity_group(Group_Type::get_type_id("Workplace"), p);
  }
  void set_office(Place* p) {
    set_activity_group(Group_Type::get_type_id("Office"), p);
  }
  void set_hospital(Place* p) {
    set_activity_group(Group_Type::get_type_id("Hospital"), p);
  }
  void terminate_activities();


  // PLACES
  void set_place_of_type(int type_id, Place* place);
  Place* get_place_of_type(int type_id);
  Group* get_group_of_type(int type_id);
  int get_place_size(int type_id);
  Network* get_network_of_type(int type_id);
  int get_network_size(int type_id);
  Household* get_stored_household();
  void assign_school();
  void assign_classroom();
  void assign_workplace();
  void assign_office();
  Place* get_neighborhood() {
    return static_cast<Place*>(get_activity_group(Place_Type::get_type_id("Neighborhood")));
  }
  Household* get_household();
  Place* get_school() {
    return static_cast<Place*>(get_activity_group(Place_Type::get_type_id("School")));
  }
  Place* get_classroom() {
    return static_cast<Place*>(get_activity_group(Place_Type::get_type_id("Classroom")));
  }
  Place* get_workplace() {
    return static_cast<Place*>(get_activity_group(Place_Type::get_type_id("Workplace")));
  }
  Place* get_office() {
    return static_cast<Place*>(get_activity_group(Place_Type::get_type_id("Office")));
  }
  int get_household_size() {
    return get_place_size(Place_Type::get_type_id("Household"));
  }
  int get_neighborhood_size() {
    return get_place_size(Place_Type::get_type_id("Neighborhood"));
  }
  int get_school_size() {
    return get_place_size(Place_Type::get_type_id("School"));
  }
  int get_classroom_size() {
    return get_place_size(Place_Type::get_type_id("Classroom"));
  }
  int get_workplace_size() {
    return get_place_size(Place_Type::get_type_id("Workplace"));
  }
  int get_office_size() {
    return get_place_size(Place_Type::get_type_id("Office"));
  }
  int get_hospital_size() {
    return get_place_size(Place_Type::get_type_id("Hospital"));
  }
  int get_place_elevation(int type);
  int get_place_income(int type);
  void start_hosting(int place_type_id);
  void select_place_of_type(int place_type_id);
  void quit_place_of_type(int place_type_id);
  void join_network(int network_type_id);
  void quit_network(int network_type_id);
  void report_place_size(int place_type_id);
  Place* get_place_with_type_id(int place_type_id);
  person_vector_t get_placemates(int place_type_id, int maxn);
  void run_action_rules(int condition_id, int state, rule_vector_t rules);

  // SCHEDULE
  std::string schedule_to_string(int sim_day);
  void print_schedule(int sim_day);

  // MIGRATION
  bool is_eligible_to_migrate() {
    return this->eligible_to_migrate;
  }
  void set_eligible_to_migrate() {
    this->eligible_to_migrate = true;
  }
  void unset_eligible_to_migrate() {
    this->eligible_to_migrate = false;
  }
  void change_household(Place* house);
  void change_school(Place* place);
  void change_workplace(Place* place, int include_office = 1);
  void set_native() {
    this->native = true;
  }
  void unset_native() {
    this->native = false;
  }
  bool is_native() {
    return this->native;
  }
  void set_original() {
    this->original = true;
  }
  void unset_original() {
    this->original = false;
  }
  bool is_original() {
    return this->original;
  }

  // TRAVEL
  Household* get_permanent_household() {
    return get_household();
    /*
    if(this->is_traveling && this->is_traveling_outside) {
      return get_stored_household();
    } else if(Global::Enable_Hospitals && this->is_hospitalized) {
      return get_stored_household();
    } else {
      return get_household();
    }
    */
  }
  void start_traveling(Person* visited);
  void stop_traveling();
  bool get_travel_status() {
    return this->is_traveling;
  }
  void set_return_from_travel_sim_day(int sim_day) {
    this->return_from_travel_sim_day = sim_day;
  }
  int get_return_from_travel_sim_day() {
    return this->return_from_travel_sim_day;
  }
  // list of activity locations, stored while traveling
  Group** stored_activity_groups;


  // GROUPS
  void update_member_index(Group* group, int pos);
  int get_number_of_people_in_group_in_state(int type_id, int condition_id, int state_id);
  int get_number_of_other_people_in_group_in_state(int type_id, int condition_id, int state_id);
  int get_group_size(int index);
  int get_group_size(string group_name) {
    return get_group_size(Group_Type::get_type_id(group_name));
  }


  // MATERNITY
  Person* give_birth(int day);


  // HEALTHCARE
  void assign_hospital(Place* place);
  void assign_primary_healthcare_facility();
  Hospital* get_primary_healthcare_facility() {
    return this->primary_healthcare_facility;
  }
  void start_hospitalization(int sim_day, int length_of_stay);
  void end_hospitalization();
  Hospital* get_hospital();
  bool person_is_hospitalized() {
    return this->is_hospitalized;
  }
  int get_visiting_health_status(Place* place, int day, int condition_id);


  // NETWORKS
  void join_network(Network* network);
  void quit_network(Network* network);
  int get_degree();
  bool is_member_of_network(Network* network);
  void add_edge_to(Person* person, Network* network);
  void add_edge_from(Person* person, Network* network);
  void delete_edge_to(Person* person, Network* network);
  void delete_edge_from(Person* person, Network* network);
  bool is_connected_to(Person* person, Network* network);
  bool is_connected_from(Person* person, Network* network);
  int get_id_of_max_weight_inward_edge_in_network(Network* network);
  int get_id_of_max_weight_outward_edge_in_network(Network* network);
  int get_id_of_min_weight_inward_edge_in_network(Network* network);
  int get_id_of_min_weight_outward_edge_in_network(Network* network);
  int get_id_of_last_inward_edge_in_network(Network* network);
  int get_id_of_last_outward_edge_in_network(Network* network);
  void set_weight_to(Person* person, Network* network, double value);
  void set_weight_from(Person* person, Network* network, double value);
  double get_weight_to(Person* person, Network* network);
  double get_weight_from(Person* person, Network* network);
  double get_timestamp_to(Person* person, Network* network);
  double get_timestamp_from(Person* person, Network* network);
  int get_out_degree(Network* network);
  int get_in_degree(Network* network);
  int get_degree(Network* network);
  person_vector_t get_outward_edges(Network* network, int max_dist = 1);
  person_vector_t get_inward_edges(Network* network, int max_dist = 1);
  void clear_network(Network* network);
  Person* get_outward_edge(int n, Network* network);
  Person* get_inward_edge(int n, Network* network);

  void print_activities();

  // VARIABLES
  double get_var(int index);
  void set_var(int index, double value);
  double_vector_t get_list_var(int index);
  int get_list_size(int list_var_id);

  static void include_variable(string name) {
    int size = Person::var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::var_name[i]) {
	return;
      }
    }
    Person::var_name.push_back(name);
  }

  static void exclude_variable(string name) {
    int size = Person::var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::var_name[i]) {
	for (int j = i+1; j < size; j++) {
	  Person::var_name[j-1] = Person::var_name[j];	  
	}
	Person::var_name.pop_back();
      }
    }
  }

  static void include_list_variable(string name) {
    int size = Person::list_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::list_var_name[i]) {
	return;
      }
    }
    Person::list_var_name.push_back(name);
  }

  static void exclude_list_variable(string name) {
    int size = Person::list_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::list_var_name[i]) {
	for (int j = i+1; j < size; j++) {
	  Person::list_var_name[j-1] = Person::list_var_name[j];	  
	}
	Person::list_var_name.pop_back();
      }
    }
  }

  static void include_global_variable(string name) {
    int size = Person::global_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::global_var_name[i]) {
	return;
      }
    }
    Person::global_var_name.push_back(name);
  }

  static void exclude_global_variable(string name) {
    int size = Person::global_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::global_var_name[i]) {
	for (int j = i+1; j < size; j++) {
	  Person::global_var_name[j-1] = Person::global_var_name[j];	  
	}
	Person::global_var_name.pop_back();
      }
    }
  }

  static void include_global_list_variable(string name) {
    int size = Person::global_list_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::global_list_var_name[i]) {
	return;
      }
    }
    Person::global_list_var_name.push_back(name);
  }

  static void exclude_global_list_variable(string name) {
    int size = Person::global_list_var_name.size();
    for (int i = 0; i < size; i++) {
      if (name == Person::global_list_var_name[i]) {
	for (int j = i+1; j < size; j++) {
	  Person::global_list_var_name[j-1] = Person::global_list_var_name[j];	  
	}
	Person::global_list_var_name.pop_back();
      }
    }
  }

  static int get_number_of_global_vars() {
    return Person::number_of_global_vars;
  }
  static int get_number_of_global_list_vars() {
    return Person::number_of_global_list_vars;
  }
  static std::string get_global_var_name(int index);
  static std::string get_global_list_var_name(int index);
  static int get_global_var_id(string var_name);
  static int get_global_list_var_id(string var_name);
  static double get_global_var(int index);
  static double_vector_t get_global_list_var(int index);
  static int get_global_list_size(int list_var_id);
  static void push_back_global_list_var(int list_var_id, double value);


  // CONDITIONS
  void setup_conditions();
  Natural_History* get_natural_history (int condition_id) const;
  void initialize_conditions(int day);
  void update_condition(int day, int condition_id);
  void become_exposed(int condition_id, Person* source, Group* group, int day, int hour);
  void become_case_fatality(int condition_id, int day);
  void expose(Person* host, int source_condition_id, int condition_id, Group* group, int day, int hour);
  void update_group_counts(int day, int condition_id, Group* group);
  void terminate_conditions(int day);
  void set_state(int condition_id, int state, int step);
  int get_state(int condition_id) const {
    return this->condition[condition_id].state;
  }
  int get_time_entered(int condition_id, int state) const {
    return this->condition[condition_id].entered[state];
  }
  void set_last_transition_step(int condition_id, int step) {
    this->condition[condition_id].last_transition_step = step;
  }
  int get_last_transition_step(int condition_id) const {
    return this->condition[condition_id].last_transition_step;
  }
  void set_next_transition_step(int condition_id, int step) {
    this->condition[condition_id].next_transition_step = step;
  }
  int get_next_transition_step(int condition_id) const {
    return this->condition[condition_id].next_transition_step;
  }
  void set_exposure_day(int condition_id, int day) {
    this->condition[condition_id].exposure_day = day;
  }
  int get_exposure_day(int condition_id) const {
    return this->condition[condition_id].exposure_day;
  }
  double get_susceptibility(int condition_id) const;
  double get_transmissibility(int condition_id) const;
  int get_transmissions(int condition_id) const;
  bool is_case_fatality(int condition_id) const {
    return this->condition[condition_id].is_fatal;
  }
  void set_case_fatality(int condition_id) {
    this->condition[condition_id].is_fatal = true;
  }
  void set_source(int condition_id, Person* source) {
    this->condition[condition_id].source = source;
  }
  Person* get_source(int condition_id) const {
    return this->condition[condition_id].source;
  }
  void set_group(int condition_id, Group* group) {
    this->condition[condition_id].group = group;
  }
  Group* get_group(int condition_id) const {
    return this->condition[condition_id].group;
  }
  int get_exposure_group_id(int condition_id) const {
    return get_group_id(condition_id);
  }
  char* get_exposure_group_label(int condition_id) const {
    return get_group_label(condition_id);
  }
  int get_exposure_group_type_id(int condition_id) const {
    return get_group_type_id(condition_id);
  }
  int get_hosts(int condition_id) const {
    return get_number_of_hosts(condition_id);
  }
  Group* get_exposure_group(int condition_id) const {
    return get_group(condition_id);
  }
  int get_group_id(int condition) const;
  char* get_group_label(int condition) const;
  int get_group_type_id(int condition) const;
  void increment_number_of_hosts(int condition_id) {
    this->condition[condition_id].number_of_hosts++;
  }
  int get_number_of_hosts(int condition_id) const {
    return this->condition[condition_id].number_of_hosts;
  }
  bool is_susceptible(int condition_id) const {
    return get_susceptibility(condition_id) > 0.0;
  }
  bool is_transmissible(int condition_id) const {
    return get_transmissibility(condition_id) > 0.0;
  }
  bool is_transmissible() const {
    for (int i = 0; i < this->number_of_conditions; ++i) {
      if (is_transmissible(i)) {
	return true;
      }
    }
    return false;
  }
  bool was_ever_exposed(int condition_id) {
    return (0 <= get_exposure_day(condition_id));
  }
  bool was_exposed_externally(int condition_id) {
    return was_ever_exposed(condition_id) && (get_group(condition_id)==NULL);
  }
  int get_days_in_health_state(int condition_id, int day) {
    return (day - 24*get_last_transition_step(condition_id));
  }
  int get_new_health_state(int condition_id);
  void request_external_updates(FILE* fp, int day);
  void get_external_updates(FILE* fp, int day);
  bool was_ever_in_state(int condition_id, int state) {
    return this->condition[condition_id].entered[state] > -1;
  }

  // VACCINES
  void set_vaccine_refusal() {
    this->vaccine_refusal = true;
  }
  bool refuses_vaccines() {
    return this->vaccine_refusal;
  }
  void set_ineligible_for_vaccine() {
    this->ineligible_for_vaccine = true;
  }
  bool is_ineligible_for_vaccine() {
    return this->ineligible_for_vaccine;
  }
  void set_received_vaccine() {
    this->received_vaccine = true;
  }
  bool has_received_vaccine() {
    return this->received_vaccine;
  }


  // META AGENTS
  bool is_meta_agent() {
    return this->id < 0; 
  }
  void set_admin_group(Group* group);
  Group* get_admin_group();
  bool has_closure();

  void get_record_string(char* result);

  //// STATIC METHODS

  static void get_population_properties();
  static void get_activity_properties();
  static void update(int sim_day);
  static void initialize_static_variables();
  static void initialize_static_activity_variables();
  static void setup();
  static void read_all_populations();
  static void read_population(const char* pop_dir, const char* pop_type );
  static void get_person_data(char* line, bool gq);
  static Person* add_person_to_population(int age, char sex, int race, int rel, Place* house,
					  Place* school, Place* work, int day, bool today_is_birthday);
  static int get_population_size() {
    return Person::pop_size;
  }
  static Person* get_person(int p) {
    if (p < Person::pop_size) {
      return Person::people[p];
    }
    else {
      return NULL;
    }
  }
  static Person* get_person_with_id(int id) {
    if (0 <= id && id < Person::id_map.size()) {
      int pos = Person::id_map[id];
      if (pos < 0) {
	// person no longer in the population
	return NULL;
      }
      else {
	return Person::people[pos];
      }
    }
    else if (id < 0) {
      // meta agent
      if (id== -1) {
	return Person::get_import_agent();
      }
      else {
	// admin agents have id -2 or less
	int pos = -id-2;
	if (pos < Person::admin_agents.size()) {
	  return Person::admin_agents[pos];
	}
	else {
	  // invalid meta-agent
	  return NULL;
	}
      }
    }
    else {
      // person never existed
      return NULL;
    }
  }
  static Person* select_random_person();
  static void prepare_to_die(int day, Person* person);
  static void remove_dead_from_population(int day);
  static void prepare_to_migrate(int day, Person* person);
  static void remove_migrants_from_population(int day);
  static void delete_person_from_population(int day, Person *person);
  static void assign_classrooms();
  static void assign_partitions();
  static void assign_primary_healthcare_facilities();
  static void report(int day);
  static void get_network_stats(char* directory);
  static void print_age_distribution(char* dir, char* date_string, int run);
  static void quality_control();
  static void finish();
  static void get_age_distribution(int* count_males_by_age, int* count_females_by_age);
  static void initialize_activities();
  static bool is_load_completed() {
    return Person::load_completed;
  }
  static void initialize_health_insurance();
  static void set_health_insurance(Person* p);
  static void update_health_interventions(int day);
  static void update_population_demographics(int day);
  static void get_external_updates(int day);
  static Person* get_import_agent() {
    return Person::Import_agent;
  }
  static Person* create_meta_agent();
  static Person* create_admin_agent();
  static int get_number_of_admin_agents() {
    return Person::admin_agents.size();
  }
  static Person* get_admin_agent(int p) {
    if (p < get_number_of_admin_agents()) {
      return Person::admin_agents[p];
    }
    else {
      return NULL;
    }
  }
  static std::string get_household_relationship_name(int rel);
  static int get_household_relationship_from_name(std::string rel_name);
  static std::string get_race_name(int race);
  static int get_race_from_name(std::string name);
  static int get_number_of_vars() {
    return Person::number_of_vars;
  }
  static int get_number_of_list_vars() {
    return Person::number_of_list_vars;
  }
  static std::string get_var_name(int index);
  static int get_var_id(string var_name);
  static std::string get_list_var_name(int index);
  static int get_list_var_id(string var_name);

  // HEALTH INSURANCE

  Insurance_assignment_index::e get_insurance_type() const {
    return this->insurance_type;
  }
  void set_insurance_type(Insurance_assignment_index::e insurance_type) {
    this->insurance_type = insurance_type;
  }
  static const char* insurance_lookup(Insurance_assignment_index::e idx) {
    assert(idx >= Insurance_assignment_index::PRIVATE);
    assert(idx <= Insurance_assignment_index::UNSET);
    switch(idx) {
    case Insurance_assignment_index::PRIVATE:
      return "Private";
    case Insurance_assignment_index::MEDICARE:
      return "Medicare";
    case Insurance_assignment_index::MEDICAID:
      return "Medicaid";
    case Insurance_assignment_index::HIGHMARK:
      return "Highmark";
    case Insurance_assignment_index::UPMC:
      return "UPMC";
    case Insurance_assignment_index::UNINSURED:
      return "Uninsured";
    case Insurance_assignment_index::UNSET:
      return "UNSET";
    default:
      Utils::fred_abort("Invalid Health Insurance Type", "");
    }
    return NULL;
  }
  static Insurance_assignment_index::e get_insurance_type_from_int(int insurance_type) {
    switch(insurance_type) {
    case 0:
      return Insurance_assignment_index::PRIVATE;
    case 1:
      return Insurance_assignment_index::MEDICARE;
    case 2:
      return Insurance_assignment_index::MEDICAID;
    case 3:
      return Insurance_assignment_index::HIGHMARK;
    case 4:
      return Insurance_assignment_index::UPMC;
    case 5:
      return Insurance_assignment_index::UNINSURED;
    default:
      return Insurance_assignment_index::UNSET;
    }
  }
  static Insurance_assignment_index::e get_health_insurance_from_distribution();


private:

  // id: Person's unique identifier (never reused)
  int id;

  // index: Person's location in population container; once set, will be unique at any given time,
  // but can be reused over the course of the simulation for different people (after death/removal)
  int index;

  // demographics
  int birthday_sim_day;		  // agent's birthday in simulation time
  short int init_age;			     // Initial age of the agent
  short int number_of_children;			// number of births
  short int household_relationship; // relationship to the householder (see Global.h)
  short int race;			  // see Global.h for race codes
  char sex;					// male or female?
  bool alive;
  bool deceased;				// is the agent deceased
  bool in_parents_home;			       // still in parents home?
  Place* home_neighborhood;
  Place* last_school;

  // migration
  bool eligible_to_migrate;
  bool native;
  bool original;

  // vaccine
  bool vaccine_refusal;
  bool ineligible_for_vaccine;
  bool received_vaccine;

  // conditions
  int number_of_conditions;
  condition_t* condition;

  //Insurance Type
  Insurance_assignment_index::e insurance_type;

  //Primary Care Location
  Hospital* primary_healthcare_facility;

  // previous infection serotype (for dengue)
  int previous_infection_serotype;

  // personal variables
  double* var;
  double_vector_t* list_var;

  // links to groups
  Link* link;

  // activity schedule:
  std::bitset<64> on_schedule; // true iff activity location is on schedule
  int schedule_updated;			 // date of last schedule update
  bool is_traveling;				// true if traveling
  bool is_traveling_outside;   // true if traveling outside modeled area
  short int profile;                              // activities profile type
  bool is_hospitalized;
  int return_from_travel_sim_day;
  int sim_day_hospitalization_ends;

  // STATIC VARIABLES

  // population

  static person_vector_t people;
  static person_vector_t admin_agents;
  static person_vector_t death_list;	  // list of agents to die today
  static person_vector_t migrant_list; // list of agents to out migrate today
  static int pop_size;
  static int next_id;
  static int next_meta_id;
  static std::vector<int> id_map;

  // personal variables
  static int number_of_vars;
  static std::vector<std::string> var_name;
  static int number_of_list_vars;
  static std::vector<std::string> list_var_name;
  static double* var_init_value;

  // global variables
  static int number_of_global_vars;
  static std::vector<std::string> global_var_name;
  static int number_of_global_list_vars;
  static std::vector<std::string> global_list_var_name;
  static double* global_var;
  static double_vector_t* global_list_var;

  // meta_agents
  static Person* Import_agent;

  // used during input
  static bool is_initialized;
  static bool load_completed;
  static int enable_copy_files;
  static void parse_lines_from_stream(std::istream &stream, bool is_group_quarters_pop);
  
  // schedule
  static bool is_weekday;     // true if current day is Monday .. Friday
  static int day_of_week;     // day of week index, where Sun = 0, ... Sat = 6

  // output
  static int report_initial_population;
  static int output_population;
  static char pop_outfile[FRED_STRING_SIZE];
  static char output_population_date_match[FRED_STRING_SIZE];
  static void write_population_output_file(int day);
  static int Popsize_by_age [Demographics::MAX_AGE+1];
  static person_vector_t report_person;
  static std::vector<report_t*> report_vec;
  static int max_reporting_agents;

  // counters
  static double Sim_based_prob_stay_home_not_needed;
  static double Census_based_prob_stay_home_not_needed;
  static int entered_school;
  static int left_school;

  // health insurance probabilities
  static double health_insurance_distribution[Insurance_assignment_index::UNSET];
  static int health_insurance_cdf_size;

  // admin lookup
  static std::unordered_map<Person*, Group*> admin_group_map;

  static bool record_location;
};

#endif // _FRED_PERSON_H
