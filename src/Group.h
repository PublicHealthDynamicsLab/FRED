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
// File: Group.h
//

#ifndef _FRED_GROUP_H_
#define _FRED_GROUP_H_

#include "Global.h"
#include <iomanip>
#include <limits>

class Person;
class Group_Type;

class Group {
public:

  static char TYPE_UNSET;
  static char SUBTYPE_NONE;


  Group(const char* lab, int _type_id);
  ~Group();

  int get_id() const {
    return this->id;
  }

  void set_id(int _id) {
    this->id = _id;
  }

  int get_index() const {
    return this->index;
  }

  void set_index(int _index) {
    this->index = _index;
  }

  int get_type_id() const {
    return this->type_id;
  }

  Group_Type* get_group_type();

  char get_subtype() const {
    return this->subtype;
  }

  void set_subtype(char _subtype) {
    this->subtype = _subtype;
  }

  char* get_label() {
    return this->label;
  }

  int get_adults();
  int get_children();

  int get_income() {
    return this->income;
  }
  void set_income(int _income) {
    this->income = _income;
    // printf("set_income %s %d\n", this->label, this->income);
  }

  // begin_membership / end_membership:
  int begin_membership(Person* per);
  void end_membership(int pos);

  double get_proximity_same_age_bias();
  double get_proximity_contact_rate();

  bool can_transmit(int condition_id);
  double get_contact_rate(int condition_id);
  int get_contact_count(int condition_id);
  bool use_deterministic_contacts(int condition_id);

  int get_size() {
    return this->members.size();
  }

  int get_original_size() {
    return this->N_orig;
  }

  person_vector_t* get_members() {
    return &(this->members);
  }

  person_vector_t* get_transmissible_people(int  condition_id) {
    return &(this->transmissible_people[condition_id]);
  }

  Person* get_member(int i) {
    return this->members[i];
  }

  void record_transmissible_days(int day, int condition_id);

  void print_transmissible(int condition_id);

  void clear_transmissible_people(int condition_id) {
    this->transmissible_people[condition_id].clear();
  }

  void add_transmissible_person(int condition_id, Person* person);

  int get_number_of_transmissible_people(int condition_id) {
    return this->transmissible_people[condition_id].size();
  }
  
  Person* get_transmissible_person(int condition_id, int n) {
    assert(n < this->transmissible_people[condition_id].size());
    return this->transmissible_people[condition_id][n];;
  }

  bool is_transmissible(int condition_id) {
    return this->transmissible_people[condition_id].size() > 0;
  }

  void set_host(Person* person) {
    this->host = person;
  }

  Person* get_host() {
    return this->host;
  }

  double get_sum_of_var(int var_id);

  double get_median_of_var(int var_id);

  void report_size(int day);

  int get_size_on_day(int day);

  void start_reporting_size() {
    this->reporting_size = true;
  }

  bool is_reporting_size() {
    return this->reporting_size;
  }

  void create_administrator();

  Person* get_administrator() {
    return this->admin;
  }

  bool is_open();

  bool has_admin_closure();

  bool is_a_place();

  bool is_a_network();

  void set_sp_id(long long int value);

  long long int get_sp_id() {
    return this->sp_id;
  }

  static bool is_a_place(int type_id);

  static bool is_a_network(int type_id);

  static Group* get_group_from_sp_id(long long int sp_id) {
    if(Group::sp_id_exists(sp_id)) {
      return Group::sp_id_map.find(sp_id)->second;
    } else {
      return NULL;
    }
  }

  static bool sp_id_exists(long long int sp_id) {
    return Group::sp_id_map.find(sp_id) != Group::sp_id_map.end();
  }

protected:
  int id; // id
  int index; // index of place of this type
  int type_id;
  char label[32]; // external id
  char subtype;
  int N_orig;     // orig number of members
  long long int sp_id;

  // epidemic counters
  int* first_transmissible_day; // first day when visited by transmissible people
  int* first_transmissible_count; // number of transmissible people on first_transmissible_day
  int* first_susceptible_count; // number of susceptible people on first_transmissible_day
  int* last_transmissible_day; // last day when visited by transmissible people

  // lists of people
  person_vector_t members;
  person_vector_t* transmissible_people;
  Person* host;    // person hosting this group
  Person* admin;   // person administering this group

  std::vector<int> size_change_day;
  std::vector<int> size_on_day;

  // should report size?
  bool reporting_size;

  // ave income
  int income;

  // map to retrieve group object from sp_id (must be unique)
  static std::map<long long int, Group*> sp_id_map;

};

#endif /* _FRED_GROUP_H_ */
