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

#ifndef _FRED_PREDICATE_H
#define _FRED_PREDICATE_H

class Expression;

#include <string>
#include <vector>
#include <stdio.h>
using namespace std;


class Person;

typedef bool (*fptr) (Person*,int,int);


class Predicate {
public:

  Predicate(string s);
  string get_name() {
    return this->name;
  }
  bool get_value(Person* person1, Person* person2 = NULL);
  bool parse();
  bool is_warning() {
    return this->warning;
  }

private:
  std::string name;
  std::string predicate_str;
  int predicate_index;
  Expression* expression1;
  Expression* expression2;
  Expression* expression3;
  int group_type_id;
  int condition_id;
  bool negate;
  fptr func;
  bool warning;

  static bool is_at(Person* person, int condition_id, int group_type_id);
  static bool is_member(Person* person, int condition_id, int group_type_id);
  static bool is_admin(Person* person, int condition_id, int group_type_id);
  static bool is_import_agent(Person* person, int condition_id, int group_type_id);
  static bool is_host(Person* person, int condition_id, int group_type_id);
  static bool is_open(Person* person, int condition_id, int group_type_id);
  static bool is_marked(Person* person, int condition_id, int group_type_id);
  static bool was_exposed_in(Person* person, int condition_id, int group_type_id);
  static bool was_exposed_externally(Person* person, int condition_id, int group_type_id);
  static bool is_student(Person* person, int condition_id, int group_type_id);
  static bool is_employed(Person* person, int condition_id, int group_type_id);
  static bool is_unemployed(Person* person, int condition_id, int group_type_id);
  static bool is_teacher(Person* person, int condition_id, int group_type_id);
  static bool is_retired(Person* person, int condition_id, int group_type_id);
  static bool lives_in_group_quarters(Person* person, int condition_id, int group_type_id);
  static bool is_college_dorm_resident(Person* person, int condition_id, int group_type_id);
  static bool is_nursing_home_resident(Person* person, int condition_id, int group_type_id);
  static bool is_military_base_resident(Person* person, int condition_id, int group_type_id);
  static bool is_prisoner(Person* person, int condition_id, int group_type_id);
  static bool is_householder(Person* person, int condition_id, int group_type_id);
  static bool household_is_in_low_vaccination_school(Person* person, int condition_id, int group_type_id);
  static bool household_refuses_vaccines(Person* person, int condition_id, int group_type_id);
  static bool attends_low_vaccination_school(Person* person, int condition_id, int group_type_id);
  static bool refuses_vaccines(Person* person, int condition_id, int group_type_id);
  static bool is_ineligible_for_vaccine(Person* person, int condition_id, int group_type_id);
  static bool has_received_vaccine(Person* person, int condition_id, int group_type_id);
  static std::string get_prefix_notation(std::string s);
};

#endif


