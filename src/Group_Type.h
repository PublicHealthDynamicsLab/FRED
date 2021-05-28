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
// File: Group_Type.h
//

#ifndef _FRED_Group_Type_H
#define _FRED_Group_Type_H

#include "Global.h"

class Group;
class Person;



class Group_Type {
public:

  Group_Type(string _name);

  ~Group_Type();

  enum GTYPE { UNKNOWN = -1, HOUSEHOLD, NEIGHBORHOOD, SCHOOL,
	       CLASSROOM, WORKPLACE, OFFICE, HOSPITAL, HOSTED_GROUP = 1000000 };

  void get_properties();

  const char* get_name() {
    return this->name.c_str();
  }

  bool is_open();

  int get_time_block(int day, int hour);

  double get_proximity_contact_rate() {
    return this->proximity_contact_rate;
  }

  double get_proximity_same_age_bias() {
    return this->proximity_same_age_bias;
  }

  bool can_transmit(int condition_id) {
    return this->can_transmit_cond[condition_id];
  }

  int get_contact_count(int condition_id) {
    return this->contact_count_for_cond[condition_id];
  }

  double get_contact_rate(int condition_id) {
    return this->contact_rate_for_cond[condition_id];
  }

  bool use_deterministic_contacts(int condition_id) {
    return this->deterministic_contacts_for_cond[condition_id];
  }

  bool has_administrator() {
    return this->has_admin;
  }

  // static methods

  static Group_Type* get_group_type(int type_id) {
    if (0 <= type_id && type_id < Group_Type::get_number_of_group_types())
      return Group_Type::group_types[type_id];
    else
      return NULL;
  }

  static Group_Type* get_group_type(string name) {
    int type_id = Group_Type::get_type_id(name);
    if (type_id < 0) {
      return NULL;
    }
    else {
      return Group_Type::get_group_type(Group_Type::get_type_id(name));
    }
  }

  static int get_number_of_group_types() { 
    return Group_Type::group_types.size();
  }

  static void add_group_type(Group_Type* group_type) {
    Group_Type::group_types.push_back(group_type);
  }

  static int get_type_id(string group_type_name);

  static string get_group_type_name(int type_id) {
    if (type_id < 0) {
      return "UNKNOWN";
    }
    if (type_id < Group_Type::get_number_of_group_types())
      return Group_Type::names[type_id];
    else
      return "UNKNOWN";
  }

  static Group* get_group_hosted_by(Person* person) {
    std::unordered_map<Person*,Group*>::const_iterator found = host_group_map.find(person);
    if (found != host_group_map.end()) {
      return found->second;
    }
    else {
      return NULL;
    }
  }

protected:

  // group_type variables
  string name;

  // initialization
  int file_available;

  // proximity transmission properties
  int proximity_contact_count;
  double proximity_contact_rate;
  double proximity_same_age_bias;

  // condition-specific transmission properties
  int* can_transmit_cond;
  int* contact_count_for_cond;
  double* contact_rate_for_cond;
  bool* deterministic_contacts_for_cond;

  // hours of operation
  int starts_at_hour[7][24];
  int open_at_hour[7][24];

  // administrator
  bool has_admin;

  // lists of group types
  static std::vector <Group_Type*> group_types;
  static std::vector<std::string> names;
  static std::unordered_map<std::string, int> group_name_map;

  // host lookup
  static std::unordered_map<Person*, Group*> host_group_map;
};

#endif // _FRED_Group_Type_H
