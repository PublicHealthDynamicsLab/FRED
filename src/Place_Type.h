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
// File: Place_Type.h
//

#ifndef _FRED_Place_Type_H
#define _FRED_Place_Type_H

#include "Global.h"
#include "Group_Type.h"


class Person;

class Place_Type : public Group_Type {
public:

  Place_Type(int _id, string _name);
  ~Place_Type();

  void get_properties();
  void prepare();

  void finish();

  void add_place(Place* place);

  Place* get_place(int n) {
    if (n < this->places.size()) {
      return this->places[n];
    }
    else {
      return NULL;
    }
  }

  int get_number_of_places() {
    return this->places.size();
  }

  Place* select_place(Person* person);

  void report_contacts_for_place_type();

  void setup_partitions();

  int get_partition_type_id() {
    return this->partition_type_id;
  }

  string get_partition_name() {
    return this->partition_name;
  }

  Place_Type* get_partition() {
    return get_place_type(this->partition_type_id);
  }

  string get_partition_basis() {
    return this->partition_basis;
  }

  int get_partition_capacity() {
    return this->partition_capacity;
  }

  void prepare_vaccination_rates();

  bool is_vaccination_rate_enabled() {
    return this->enable_vaccination_rates;
  }

  double get_default_vaccination_rate() {
    return this->default_vaccination_rate;
  }

  double get_medical_vacc_exempt_rate() {
    return this->medical_vacc_exempt_rate;
  }

  Place* get_new_place();

  Place* generate_new_place(Person* person);

  void report(int day);

  // cutoffs

  int get_size_quartile(int n);

  int get_size_quintile(int n);

  int get_income_quartile(int n);

  int get_income_quintile(int n);

  int get_elevation_quartile(double n);

  int get_elevation_quintile(double n);

  double get_income_first_quartile() {
    return this->income_cutoffs.first_quartile;
  }

  double get_income_second_quartile() {
    return this->income_cutoffs.second_quartile;
  }

  double get_income_third_quartile() {
    return this->income_cutoffs.third_quartile;
  }


  // static methods

  static void set_cutoffs(cutoffs_t* cutoffs, double* values, int size);

  static void get_place_type_properties();

  static void prepare_place_types();

  static void read_places(const char* pop_dir);

  static void report_place_size(int place_type_id);

  static void finish_place_types();

  static Place_Type* get_place_type(int type_id) {
    return static_cast<Place_Type*>(Group_Type::get_group_type(type_id));
  }

  static Place_Type* get_place_type(string name) {
    return static_cast<Place_Type*>(Group_Type::get_group_type(name));
  }

  const char* get_name() {
    return this->name.c_str();
  }

  static string get_place_type_name(int type_id) {
    if(type_id < 0) {
      return "UNKNOWN";
    }
    if(type_id < Place_Type::get_number_of_place_types())
    {
      return Place_Type::names[type_id];
    } else {
      return "UNKNOWN";
    }
  }

  static int get_number_of_place_types() { 
    return Place_Type::place_types.size();
  }

  static void include_place_type(string name) {
    int size = Place_Type::names.size();
    for(int i = 0; i < size; ++i) {
      if(name == Place_Type::names[i]) {
        return;
      }
    }
    Place_Type::names.push_back(name);
  }

  static void exclude_place_type(string name) {
    /*
    int size = Place_Type::names.size();
    for (int i = 0; i < size; i++) {
      if (name == Place_Type::names[i]) {
	for (int j = i+1; j < size; j++) {
	  Place_Type::names[j-1] = Place_Type::names[j];	  
	}
	Place_Type::names.pop_back();
      }
    }
    */
  }

  static bool is_predefined(string value);

  static bool is_not_predefined(string value) {
    return is_predefined(value) == false;
  }

  static Place_Type* get_household_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Household")];
  }

  static Place_Type* get_neighborhood_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Neighborhood")];
  }

  static Place_Type* get_school_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("School")];
  }

  static Place_Type* get_classroom_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Classroom")];
  }

  static Place_Type* get_workplace_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Workplace")];
  }

  static Place_Type* get_office_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Office")];
  }

  static Place_Type* get_hospital_place_type() {
    return Place_Type::place_types[Place_Type::get_type_id("Hospital")];
  }

  static void add_places_to_neighborhood_layer();

  static Place* select_place_of_type(int place_type_id, Person* person);

  static void report_contacts();

  static Place* generate_new_place(int place_type_id, Person* person);

  static Place* get_place_hosted_by(Person* person) {
    std::unordered_map<Person*,Place*>::const_iterator found = host_place_map.find(person);
    if(found != Place_Type::host_place_map.end()) {
      return found->second;
    }
    else {
      return NULL;
    }
  }

  static bool is_a_host(Person* person) {
    return (Place_Type::get_place_hosted_by(person) != NULL);
  }

  double get_max_dist() {
    return this->max_dist;
  }

  long long int get_next_sp_id() {
    return this->next_sp_id++;
  }

private:

  // list of places of this type
  place_vector_t places;

  // place type features
  double max_dist;
  string partition_name;
  int partition_type_id;
  string partition_basis;
  int partition_capacity;
  int min_age_partition;
  int max_age_partition;
  int base_type_id;

  // plotting and visualizatiom
  int enable_visualization;
  int report_size;

  // vaccination rates
  int enable_vaccination_rates;
  double default_vaccination_rate;
  int need_to_get_vaccination_rates;
  char vaccination_rate_file[FRED_STRING_SIZE];
  double medical_vacc_exempt_rate;

  // cutoffs for quintiles and quartiles
  cutoffs_t elevation_cutoffs;
  cutoffs_t size_cutoffs;
  cutoffs_t income_cutoffs;

  // next sp_id for this type;
  long long int next_sp_id;

  // lists of place types
  static std::vector <Place_Type*> place_types;
  static std::vector<std::string> names;

  // host lookup
  static std::unordered_map<Person*, Place*> host_place_map;

};

#endif // _FRED_Place_Type_H
