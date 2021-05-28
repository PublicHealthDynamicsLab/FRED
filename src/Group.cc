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
// File: Group.cc
//

#include "Group.h"
#include "Condition.h"
#include "Person.h"
#include "Utils.h"

char Group::TYPE_UNSET = 'U';

char Group::SUBTYPE_NONE = 'X';
std::map<long long int, Group*> Group::sp_id_map;

Group::Group(const char* lab, int _type_id) {

  this->sp_id = -1;
  this->index = -1;
  this->id = -1;
  this->type_id = _type_id;
  this->subtype = Group::SUBTYPE_NONE;

  strcpy(this->label, lab);

  this->N_orig = 0;             // orig number of members
  this->income = -1;

  // lists of people
  this->members.clear();

  int conditions = Condition::get_number_of_conditions();
  this->transmissible_people = new person_vector_t[conditions];

  // epidemic counters
  this->first_transmissible_count = new int[conditions];
  this->first_susceptible_count = new int[conditions];
  this->first_transmissible_day = new int[conditions];
  this->last_transmissible_day = new int[conditions];

  // zero out all condition-specific counts
  for(int d = 0; d < conditions; ++d) {
    this->transmissible_people[d].clear();
    this->first_transmissible_count[d] = 0;
    this->first_susceptible_count[d] = 0;
    this->first_transmissible_day[d] = -1;
    this->last_transmissible_day[d] = -2;
  }

  this->size_change_day.clear();
  this->size_on_day.clear();
  this->reporting_size = false;
  this->admin = NULL;
  this->host = NULL;
}

Group::~Group() {
  if(this->transmissible_people != NULL) {
    delete this->transmissible_people;
  }
  if(this->transmissible_people != NULL) {
    delete this->transmissible_people;
  }
  if(this->transmissible_people != NULL) {
    delete this->transmissible_people;
  }
}

int Group::begin_membership(Person* per) {
  if(this->get_size() == this->members.capacity()) {
    // double capacity if needed (to reduce future reallocations)
    this->members.reserve(2 * this->get_size());
  }
  this->members.push_back(per);
  FRED_VERBOSE(1, "Enroll person %d age %d in group %d %s\n", per->get_id(), per->get_age(), this->get_id(), this->get_label());
  return this->members.size()-1;
}

void Group::end_membership(int pos) {
  int size = this->members.size();
  if(!(0 <= pos && pos < size)) {
    FRED_VERBOSE(1, "group %d %s pos = %d size = %d\n", this->get_id(), this->get_label(), pos, size);
  }
  assert(0 <= pos && pos < size);
  Person* removed = this->members[pos];
  if(pos < size - 1) {
    Person* moved = this->members[size - 1];
    FRED_VERBOSE(1, "UNENROLL group %d %s pos = %d size = %d removed %d moved %d\n",
      this->get_id(), this->get_label(), pos, size, removed->get_id(), moved->get_id());
    this->members[pos] = moved;
    moved->update_member_index(this, pos);
  } else {
    FRED_VERBOSE(1, "UNENROLL group %d %s pos = %d size = %d removed %d moved NONE\n",
     this->get_id(), this->get_label(), pos, size, removed->get_id());
  }
  this->members.pop_back();
  FRED_VERBOSE(1, "UNENROLL group %d %s size = %d\n", this->get_id(), this->get_label(), this->members.size());
}

void Group::print_transmissible(int condition_id) {
  printf("INFECTIOUS in Group %s Condition %d: ", this->get_label(), condition_id);
  int size = this->transmissible_people[condition_id].size();
  for(int i = 0; i < size; ++i) {
    printf(" %d", this->transmissible_people[condition_id][i]->get_id());
  }
  printf("\n");
}

int Group::get_children() {
  int children = 0;
  for(int i = 0; i < this->members.size(); ++i) {
    children += (this->members[i]->get_age() < Global::ADULT_AGE ? 1 : 0);
  }
  return children;
}

int Group::get_adults() {
  return (this->members.size() - this->get_children());
}

void Group::add_transmissible_person(int condition_id, Person* person) {
  FRED_VERBOSE(1, "ADD_INF: person %d mix_group %s\n", person->get_id(), this->label);
  this->transmissible_people[condition_id].push_back(person);
}

void Group::record_transmissible_days(int day, int condition_id) {
  if(this->first_transmissible_day[condition_id] == -1) {
    this->first_transmissible_day[condition_id] = day;
    this->first_transmissible_count[condition_id] = get_number_of_transmissible_people(condition_id);
    this->first_susceptible_count[condition_id] = get_size() - get_number_of_transmissible_people(condition_id);
  }
  this->last_transmissible_day[condition_id] = day;
}

double Group::get_sum_of_var(int var_id) {
  double sum = 0.0;
  for(int i = 0; i < this->members.size(); ++i) {
    Person* person = this->members[i];
    sum += person->get_var(var_id);
  }
  return sum;
}

double Group::get_median_of_var(int var_id) {
  int size = get_size();
  double median = 0.0;
  if(size > 0) {
    std::vector<double> values;
    values.clear();
    for(int i = 0; i < this->members.size(); ++i) {
      Person* person = this->members[i];
      values.push_back(person->get_var(var_id));
    }
    std::sort(values.begin(), values.end());
    median = values[size / 2];
  }
  return median;
}

void Group::report_size(int day) {
  int vec_size = this->size_on_day.size();
  if(vec_size == 0 || get_size() != this->size_on_day[vec_size - 1]) {
    this->size_change_day.push_back(day);
    this->size_on_day.push_back(get_size());
  }
}

int Group::get_size_on_day(int day) {
  int size = 0;
  int vec_size = this->size_on_day.size();
  for(int i = 0; i < vec_size; ++i) {
    if(this->size_change_day[i] > day) {
      return size;
    } else {
      size = this->size_on_day[i];
    }
  }
  return size;
}

double Group::get_proximity_same_age_bias() {
  return Group_Type::get_group_type(this->type_id)->get_proximity_same_age_bias();
}

double Group::get_proximity_contact_rate() {
  return Group_Type::get_group_type(this->type_id)->get_proximity_contact_rate();
}

double Group::get_contact_rate(int condition_id) {
  return Group_Type::get_group_type(this->type_id)->get_contact_rate(condition_id);
}

bool Group::use_deterministic_contacts(int condition_id) {
  return Group_Type::get_group_type(this->type_id)->use_deterministic_contacts(condition_id);
}

bool Group::can_transmit(int condition_id) {
  return Group_Type::get_group_type(this->type_id)->can_transmit(condition_id);
}

int Group::get_contact_count(int condition_id) {
  return Group_Type::get_group_type(this->type_id)->get_contact_count(condition_id);
}

Group_Type* Group::get_group_type() {
  return Group_Type::get_group_type(this->type_id);
}

void Group::create_administrator() {
  // generate a meta_agent
  this->admin = Person::create_admin_agent();
  this->admin->set_admin_group(this);
  // FRED_VERBOSE(0, "CREATE_ADMIN group %s admin person %d admin_group %s\n", this->get_label(), admin->get_id(), admin->get_admin_group()->get_label());

  if(is_a_place()) {
    // add admin id to place list
    string var_name = string(get_group_type()->get_name()) + "List";
    int vid = Person::get_global_list_var_id(var_name);
    int p = Person::get_global_list_size(vid);
    Person::push_back_global_list_var(vid, this->admin->get_id());
    Place* place = admin->get_place_of_type(this->type_id);
    // FRED_VERBOSE(0, "CREATE_ADMIN %s[%d] = %d %lld %f %f\n", var_name.c_str(), p, admin->get_id(), place->get_sp_id(), place->get_latitude(), place->get_longitude()); 
  }

  return;
}


bool Group::is_open() {

  Group_Type* group_type = Group_Type::get_group_type(this->type_id);
  if(group_type->is_open() == false) {
    FRED_STATUS(1, "group %s is closed at hour %d day %d because group_type is closed\n", Global::Simulation_Hour,
        Global::Simulation_Day, this->get_label());
    return false;
  }

  if(has_admin_closure()) {
    FRED_STATUS(1, "group %s is closed due to admin closure\n", this->get_label());
    return false;
  }

  FRED_STATUS(1, "group %s is open\n", this->get_label());
  return true;
}

bool Group::has_admin_closure() {
  // FRED_VERBOSE(0, "has_admin_closure: check group %s\n", this->get_label());
  if(this->admin != NULL) {
    if(this->admin->has_closure()) {
      // FRED_STATUS(0, "group %s is closed on %s due to admin closure\n", this->get_label(), Date::get_date_string().c_str());
      return true;
    } else {
      return false;
    }
  }
  return false;
}

bool Group::is_a_place() {
  return (get_type_id() < Place_Type::get_number_of_place_types());
}

bool Group::is_a_network() {
  return (Place_Type::get_number_of_place_types() <= get_type_id());
}

bool Group::is_a_place(int type_id) {
  return (type_id < Place_Type::get_number_of_place_types());
}

bool Group::is_a_network(int type_id) {
  return (Place_Type::get_number_of_place_types() <= type_id);
}

/**
 * Set the sp_id of this mixing Group. There is a static map that is used to avoid duplications. If the value is a duplicate, a warning message will be
 * created.
 *
 * @param value the sp_id to set
 */
void Group::set_sp_id(long long int value) {
  this->sp_id = value;
  if(Group::sp_id_map.insert(std::make_pair(value, this)).second == false) {
    // Note - we will probably have duplicates when we use multiple counties that border each other, since there are many people who work or go to school
    // across borders. If we use Utils::print_error(), then we set the  Global::Error_found flag to true, which will cause the simulation to abort.
    // To avoid this, we are simply writing a warning instead.
    char msg[FRED_STRING_SIZE];
    sprintf(msg, "Place id %lld is duplicated for two places: %s and %s", value, get_label(), Group::sp_id_map[value]->get_label());
    Utils::print_warning(msg);
    FRED_VERBOSE(0, "%s\n", msg);
  }
}

