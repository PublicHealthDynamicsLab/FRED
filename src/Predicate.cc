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

#include "Predicate.h"
#include "Condition.h"
#include "Date.h"
#include "Expression.h"
#include "Global.h"
#include "Household.h"
#include "Person.h"
#include "Place.h"
#include "Network.h"
#include "Group_Type.h"
#include "Place_Type.h"
#include "Utils.h"
#include <regex>

std::map<std::string,int> compare_map = {
  {"eq", 1}, {"neq", 2}, {"lt", 3}, {"lte", 4},
  {"gt", 5}, {"gte", 6}
};

std::map<std::string,int> misc_map = {
  {"range", 1}, {"date_range", 2}, {"date", 3}
};

std::map<std::string,int> group_map = {
  {"at", 1}, {"member", 1}, {"admins", 1}, {"hosts", 1},{"admin", 1}, {"host", 1},
  {"open", 1},
  {"exposed_in", 1},
  {"exposed_externally", 1},
};

int pred_find_comma(string s) {
  char x [FRED_STRING_SIZE];
  strcpy(x, s.c_str());
  int inside = 0;
  int n = 0;
  while (x[n]!='\0') {
    if (x[n]==',' && !inside) {
      return n;
    }
    if (x[n]=='(') {
      inside++;
    }
    if (x[n]==')') {
      inside--;
    }
    n++;
  }
  return -1;
}




Predicate::Predicate(string s) {
  this->name = Utils::delete_spaces(s);
  this->name = s;
  this->predicate_str = "";
  this->predicate_index = 0;
  this->expression1 = NULL;
  this->expression2 = NULL;
  this->expression3 = NULL;
  this->func = NULL;
  this->group_type_id = -1;
  this->condition_id = -1;
  this->negate = false;
  this->warning = false;
}


bool Predicate::get_value(Person* person, Person* other) {

  bool result = false;

  if (func != NULL) {
    // evalute this predicate directly using a built-in function
    result = func(person, this->condition_id, this->group_type_id);
  }

  else if (compare_map.find(this->predicate_str)!=compare_map.end()) {
    double value1 = this->expression1->get_value(person,other);
    double value2 = this->expression2->get_value(person,other);
    switch(this->predicate_index) {
      case 1:					// "eq"
      result = (value1 == value2);
      // printf("QUAL |%s| person %d race %d expr1 |%s| value1 %f expr2 |%s| value2 %f results %d\n", this->name.c_str(), person->get_id(), person->get_race(), this->expression1->get_name().c_str(), value1, this->expression2->get_name().c_str(), value2, result? 1 : 0);
      break;
    case 2:					// "neq"
      result = (value1 != value2);
      break;
    case 3:					// "lt"
      result = (value1 < value2);
      break;
    case 4:					// "lte"
      result = (value1 <= value2);
      break;
    case 5:					// "gt"
      result = (value1 > value2);
      break;
    case 6:					// "gte"
      result = (value1 >= value2);
      break;
    }
  }

  else if (this->predicate_str=="range") {
    double value1 = this->expression1->get_value(person,other);
    double value2 = this->expression2->get_value(person,other);
    double value3 = this->expression3->get_value(person,other);
    result = (value2 <= value1 && value1 <= value3);
  }

  else if (this->predicate_str=="date") {
    int value1 = this->expression1->get_value(person);
    int today = Date::get_date_code();
    // FRED_VERBOSE(0, "RULE date %d %d: |%s|\n", (int)value1,(int)today,this->name.c_str());
    result = (value1 == today);
  }

  else if (this->predicate_str=="date_range") {
    int value1 = this->expression1->get_value(person);
    int value2 = this->expression2->get_value(person);
    int today = Date::get_date_code();
    result = false;
    if (value1 <= value2) {
      result = (value1 <= today && today <= value2);
    }
    else {
      result = (value1 <= today || today <= value2); 
    }
    // FRED_VERBOSE(0, "PRED date_range %s %s result = %d\n", Date::get_date_string().c_str(), this->name.c_str(), result);
  }

  if (this->negate) {
    return result==false;
  }
  else {
    return result;
  }
}
  

bool Predicate::parse() {

  // printf("RULE PREDICATE: parsing predicate |%s|\n", this->name.c_str()); fflush(stdout);

  string predicate = "";
  if (this->name.find("not(")==0) {
    int pos = this->name.find_last_of(")");
    if (pos==string::npos) {
      FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED PREDICATE = |%s|\n", this->name.c_str());
      return false;
    }
    predicate = this->name.substr(4,pos-4);
    // FRED_VERBOSE(0, "RULE predicate without NOT: |%s|\n", predicate.c_str());
    this->negate = true;
  }
  else {
    predicate = this->name;
  }
  predicate = Predicate::get_prefix_notation(predicate);

  int pos1 = predicate.find("(");
  int pos2 = predicate.find_last_of(")");
  if (pos1==string::npos || pos2==string::npos || pos2 < pos1) {

    // check for zero-arg predicates

    // activity profile
    if (predicate == "is_student") {
      func = &is_student;
      return true;
    }

    // meta agent
    if (predicate == "is_import_agent") {
      func = &is_import_agent;
      return true;
    }

    // deprecated:

    if (predicate == "household_is_in_low_vaccination_school") {
      func = &household_is_in_low_vaccination_school;
      return true;
    }

    if (predicate == "household_refuses_vaccines") {
      func = &household_refuses_vaccines;
      return true;
    }

    if (predicate == "attends_low_vaccination_school") {
      func = &attends_low_vaccination_school;
      return true;
    }

    if (predicate == "refuses_vaccine") {
      func = &refuses_vaccines;
      return true;
    }

    if (predicate == "is_ineligible_for_vaccine") {
      func = &is_ineligible_for_vaccine;
      return true;
    }

    if (predicate == "has_received_vaccine") {
      func = &has_received_vaccine;
      return true;
    }

    FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED PREDICATE = |%s|\n", this->name.c_str());
    return false;
  }

  this->predicate_str = predicate.substr(0,pos1);
  if (compare_map.find(this->predicate_str)==compare_map.end() &&
      misc_map.find(this->predicate_str)==misc_map.end() &&
      group_map.find(this->predicate_str)==group_map.end()) {
    FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED PREDICATE |%s| in |%s|\n", this->predicate_str.c_str(), this->name.c_str());
    return false;
  }
  // FRED_VERBOSE(0, "RULE predicate: |%s|\n", this->predicate_str.c_str());

  // discard outer parentheses
  string inner = predicate.substr(pos1+1,pos2-pos1-1);
  // FRED_VERBOSE(0, "RULE inner: |%s|\n", inner.c_str());

  if (compare_map.find(this->predicate_str)!=compare_map.end()) {
    this->predicate_index = compare_map[this->predicate_str];
    int pos_comma = pred_find_comma(inner);
    if (-1 < pos_comma) {
      // get args
      string first = inner.substr(0,pos_comma);
      this->expression1 = new Expression(first);
      if (this->expression1->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 1st ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression1->is_warning();
	return false;
      }

      string second = inner.substr(pos_comma+1);

      // handle symbolic state names
      if (first.find("current_state_in")==0) {
	// get condition _id
	int pos1 = strlen("current_state_in_");
	string cond_name = first.substr(pos1);
	int cond_id = Condition::get_condition_id(cond_name);
	if (cond_id < 0) {
	  FRED_VERBOSE(0, "HELP: RULE BAD 1st ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	  return false;
	}
	int state_id = Condition::get_condition(cond_id)->get_state_from_name(second);
	if (state_id < 0) {
	  FRED_VERBOSE(0, "HELP: RULE BAD 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	  return false;
	}
	second = std::to_string(state_id);
      }
      this->expression2 = new Expression(second);
      if (this->expression2->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression2->is_warning();
	return false;
      }
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: RULE MISSING 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
      return false;
    }
  }
  else if (this->predicate_str=="range") {
    int pos_comma = pred_find_comma(inner);
    if (-1 < pos_comma) {
      // get args
      string first = inner.substr(0,pos_comma);
      this->expression1 = new Expression(first);
      if (this->expression1->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 1st ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression1->is_warning();
	return false;
      }
      inner = inner.substr(pos_comma+1);
      pos_comma = pred_find_comma(inner);
      string second = inner.substr(0,pos_comma);
      this->expression2 = new Expression(second);
      if (this->expression2->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression2->is_warning();
	return false;
      }
      string third = inner.substr(pos_comma+1);
      this->expression3 = new Expression(third);
      if (this->expression3->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 3rd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression3->is_warning();
	return false;
      }
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: RULE MISSING 2nd and 3rd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
      return false;
    }
  }
  else if (this->predicate_str=="date_range") {
    int pos_comma = pred_find_comma(inner);
    if (-1 < pos_comma) {
      // get args
      string first = inner.substr(0,pos_comma);
      int date_code1 = Date::get_date_code(first);
      if (date_code1 < 0) {
	FRED_VERBOSE(0, "HELP: RULE illegal DATE SPEC |%s| PREDICATE |%s|\n", first.c_str(), this->name.c_str());
	return false;
      }
      this->expression1 = new Expression(std::to_string(date_code1));
      if (this->expression1->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 1st ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression1->is_warning();
	return false;
      }
      string second = inner.substr(pos_comma+1);
      int date_code2 = Date::get_date_code(second);
      if (date_code2 < 0) {
	FRED_VERBOSE(0, "HELP: RULE illegal DATE SPEC |%s| PREDICATE |%s|\n", second.c_str(), this->name.c_str());
	return false;
      }
      this->expression2 = new Expression(std::to_string(date_code2));
      if (this->expression2->parse()==false) {
	FRED_VERBOSE(0, "HELP: RULE BAD 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
	this->warning = this->expression2->is_warning();
	return false;
      }
      // FRED_VERBOSE(0, "OK DATE SPEC |%s-%s| date_codes %d %d PREDICATE |%s|\n", first.c_str(), second.c_str(), date_code1, date_code2, this->name.c_str());
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: RULE MISSING 2nd ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
      return false;
    }
  }
  else if (this->predicate_str=="date") {
    int date_code1 = Date::get_date_code(inner);
    if (date_code1 < 0) {
      FRED_VERBOSE(0, "HELP: RULE illegal DATE SPEC |%s| PREDICATE |%s|\n", inner.c_str(), this->name.c_str());
      return false;
    }
    this->expression1 = new Expression(std::to_string(date_code1));
    if (this->expression1->parse()==false) {
      FRED_VERBOSE(0, "HELP: RULE BAD 1st ARG for QUAL %s = |%s|\n", this->predicate_str.c_str(), this->name.c_str());
      this->warning = this->expression1->is_warning();
      return false;
    }
    return true;
  }
  else {

    if (this->predicate_str=="at") {
      this->group_type_id = Group_Type::get_type_id(inner);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	return false;
      }
      this->func = &is_at;
      return true;
    }

    if (this->predicate_str=="member") {
      this->group_type_id = Group_Type::get_type_id(inner);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	return false;
      }
      this->func = &is_member;
      return true;
    }

    if (this->predicate_str=="admins" || this->predicate_str=="admin") {
      this->group_type_id = Group_Type::get_type_id(inner);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	return false;
      }
      this->func = &is_admin;
      return true;
    }

    if (this->predicate_str=="hosts" || this->predicate_str=="host") {
      this->group_type_id = Group_Type::get_type_id(inner);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	return false;
      }
      this->func = &is_host;
      return true;
    }

    if (this->predicate_str=="open") {
      this->group_type_id = Group_Type::get_type_id(inner);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	return false;
      }
      this->func = &is_open;
      return true;
    }

    if (this->predicate_str=="exposed_in") {
      string cond = inner.substr(0,inner.find(","));
      string group_type = inner.substr(inner.find(",")+1);
      this->condition_id = Condition::get_condition_id(cond);
      if (this->condition_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED condition |%s| PREDICATE = |%s|\n", cond.c_str(), this->name.c_str());
	this->warning = true;
	return false;
      }
      this->group_type_id = Group_Type::get_type_id(group_type);
      if (this->group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED group_type |%s| PREDICATE = |%s|\n", group_type.c_str(), this->name.c_str());
	return false;
      }
      this->func = &was_exposed_in;
      return true;
    }

     if (this->predicate_str=="exposed_externally") {
      this->condition_id = Condition::get_condition_id(inner);
      if (this->condition_id < 0) {
	FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED condition |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
	this->warning = true;
	return false;
      }
      this->group_type_id = -1;
      this->func = &was_exposed_externally;
      return true;
     }
     
     FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED predicate |%s| PREDICATE = |%s|\n", inner.c_str(), this->name.c_str());
     return false;

  }
  FRED_VERBOSE(0, "HELP: RULE UNRECOGNIZED PROBLEM WITH PREDICATE = |%s|\n", this->name.c_str());
  return false;
}


bool Predicate::is_at(Person* person, int condition_id, int group_type_id) {
  if (is_open(person, condition_id, group_type_id)) {
    Group* group = person->get_group_of_type(group_type_id);
    return person->is_present(Global::Simulation_Day, group);
  }
  return false;
}

bool Predicate::is_member(Person* person, int condition_id, int group_type_id) {
  Group* group = person->get_group_of_type(group_type_id);
  if (group == NULL) {
    return false;
  }
  return true;
}

bool Predicate::is_admin(Person* person, int condition_id, int group_type_id) {
  Group* group = person->get_admin_group();
  if (group == NULL) {
    return false;
  }
  return group->get_type_id() == group_type_id;
}

bool Predicate::is_host(Person* person, int condition_id, int group_type_id) {
  Group* group = Group_Type::get_group_hosted_by(person);
  if (group == NULL) {
    return false;
  }
  else {
    return group->get_type_id() == group_type_id;
  }
}

bool Predicate::is_open(Person* person, int condition_id, int group_type_id) {
  Group* group = person->get_group_of_type(group_type_id);
  if (group == NULL) {
    return false;
  }
  if (group->is_a_place()) {
    return static_cast<Place*>(group)->is_open(Global::Simulation_Day);
  }
  else {
    return group->is_open();
  }
}

bool Predicate::was_exposed_in(Person* person, int condition_id, int group_type_id) {
  if (group_type_id == Group_Type::get_type_id("School")) {
    return person->get_exposure_group_type_id(condition_id)==Group_Type::get_type_id("School")
      || person->get_exposure_group_type_id(condition_id)==Group_Type::get_type_id("Classroom");
  }
  else {
    if (group_type_id == Group_Type::get_type_id("Workplace")) {
      return person->get_exposure_group_type_id(condition_id)==Group_Type::get_type_id("Workplace")
	|| person->get_exposure_group_type_id(condition_id)==Group_Type::get_type_id("Office");
    }
    else {
      return (group_type_id == person->get_exposure_group_type_id(condition_id));
    }
  }
}

bool Predicate::was_exposed_externally(Person* person, int condition_id, int group_type_id) {
  return person->was_exposed_externally(condition_id);
}

bool Predicate::is_student(Person* person, int condition_id, int group_type_id) {
  return person->is_student();
}

bool Predicate::is_import_agent(Person* person, int condition_id, int group_type_id) {
  return (person == Person::get_import_agent());
}

bool Predicate::is_employed(Person* person, int condition_id, int group_type_id) {
  return person->is_employed();
}

bool Predicate::is_unemployed(Person* person, int condition_id, int group_type_id) {
  return person->is_employed() == false;
}

bool Predicate::is_teacher(Person* person, int condition_id, int group_type_id) {
  return person->is_teacher();
}

bool Predicate::is_retired(Person* person, int condition_id, int group_type_id) {
  return person->is_retired();
}

bool Predicate::lives_in_group_quarters(Person* person, int condition_id, int group_type_id) {
  return person->lives_in_group_quarters();
}

bool Predicate::is_college_dorm_resident(Person* person, int condition_id, int group_type_id) {
  return person->is_college_dorm_resident();
}

bool Predicate::is_nursing_home_resident(Person* person, int condition_id, int group_type_id) {
  return person->is_nursing_home_resident();
}

bool Predicate::is_military_base_resident(Person* person, int condition_id, int group_type_id) {
  return person->is_military_base_resident();
}

bool Predicate::is_prisoner(Person* person, int condition_id, int group_type_id) {
  return person->is_prisoner();
}

bool Predicate::is_householder(Person* person, int condition_id, int group_type_id) {
  return person->is_householder();
}

bool Predicate::household_is_in_low_vaccination_school(Person* person, int condition_id, int group_type_id) {
  if (person->get_household() != NULL) {
    return person->get_household()->is_in_low_vaccination_school();
  }
  else {
    return 0;
  }
}

bool Predicate::household_refuses_vaccines(Person* person, int condition_id, int group_type_id) {
  if (person->get_household() != NULL) {
    return person->get_household()->refuses_vaccines();
  }
  else {
    return 0;
  }
}

bool Predicate::attends_low_vaccination_school(Person* person, int condition_id, int group_type_id) {
  if (person->get_school() != NULL) {
    return person->get_school()->is_low_vaccination_place();
  }
  else {
    return 0;
  }
}

bool Predicate::refuses_vaccines(Person* person, int condition_id, int group_type_id) {
  return person->refuses_vaccines();
}

bool Predicate::is_ineligible_for_vaccine(Person* person, int condition_id, int group_type_id) {
  return person->is_ineligible_for_vaccine();
}

bool Predicate::has_received_vaccine(Person* person, int condition_id, int group_type_id) {
  return person->has_received_vaccine();
}

std::string Predicate::get_prefix_notation(std::string s) {
  int pos = s.find("==");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+2);
    std::string result = "eq(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  pos = s.find("!=");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+2);
    std::string result = "neq(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  pos = s.find("<=");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+2);
    std::string result = "lte(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  pos = s.find(">=");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+2);
    std::string result = "gte(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  pos = s.find(">");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+1);
    std::string result = "gt(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  pos = s.find("<");
  if (pos!=std::string::npos) {
    std::string first = s.substr(0,pos);
    std::string second = s.substr(pos+1);
    std::string result = "lt(" + first + "," + second + ")";
    printf("PREFIX: |%s| => |%s|\n", s.c_str(), result.c_str());
    return result;
  }

  printf("PREFIX: |%s| => |%s|\n", s.c_str(), "NO CHANGE");
  return s;
}

