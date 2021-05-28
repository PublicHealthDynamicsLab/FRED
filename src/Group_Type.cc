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
// File: Group_Type.cc
//

#include "Condition.h"
#include "Date.h"
#include "Group.h"
#include "Group_Type.h"
#include "Person.h"
#include "Property.h"
#include "Utils.h"

//////////////////////////
//
// STATIC VARIABLES
//
//////////////////////////

std::vector <Group_Type*> Group_Type::group_types;
std::vector<std::string> Group_Type::names;
std::unordered_map<std::string, int> Group_Type::group_name_map;
std::unordered_map<Person*, Group*> Group_Type::host_group_map;


Group_Type::Group_Type(string _name) {
  this->name = _name;
  this->proximity_contact_rate = 0.0;
  this->proximity_same_age_bias = 0.0;
  this->can_transmit_cond = NULL;
  this->contact_count_for_cond = NULL;
  this->contact_rate_for_cond = NULL;
  this->deterministic_contacts_for_cond = NULL;
  this->has_admin = false;
  Group_Type::group_name_map[this->name] = Group_Type::get_number_of_group_types();
  Group_Type::names.push_back(this->name);
}

Group_Type::~Group_Type() {
}

void Group_Type::get_properties() {
  FRED_STATUS(0, "group_type %s get_properties entered\n", this->name.c_str());
  
  char property_name[FRED_STRING_SIZE];
  char property_value[FRED_STRING_SIZE];

  // optional properties:
  Property::disable_abort_on_failure();

  this->file_available = 0;
  sprintf(property_name, "%s.file_available", this->name.c_str());
  Property::get_property(property_name, &this->file_available);

  this->proximity_contact_rate = 0.0;
  sprintf(property_name, "%s.contacts", this->name.c_str());
  Property::get_property(property_name, &this->proximity_contact_rate);

  this->proximity_same_age_bias = 0.0;
  sprintf(property_name, "%s.same_age_bias", this->name.c_str());
  Property::get_property(property_name, &this->proximity_same_age_bias);

  printf("%s.contacts = %f\n", this->name.c_str(), this->proximity_contact_rate);

  int number_of_conditions = Condition::get_number_of_conditions();
  this->can_transmit_cond = new int [number_of_conditions];
  this->contact_count_for_cond = new int [number_of_conditions];
  this->contact_rate_for_cond = new double [number_of_conditions];
  this->deterministic_contacts_for_cond = new bool [number_of_conditions];

  for (int cond_id = 0; cond_id < number_of_conditions; cond_id++) {
    string cond_name = Condition::get_name(cond_id);

    this->can_transmit_cond[cond_id] = 0;
    sprintf(property_name, "%s.can_transmit_%s", this->name.c_str(), cond_name.c_str());
    Property::get_property(property_name, &this->can_transmit_cond[cond_id]);
    FRED_STATUS(0, "%s = %d\n", property_name, this->can_transmit_cond[cond_id]);

    this->contact_count_for_cond[cond_id] = 0;
    sprintf(property_name, "%s.contact_count_for_%s", this->name.c_str(), cond_name.c_str());
    Property::get_property(property_name, &this->contact_count_for_cond[cond_id]);
    FRED_STATUS(0, "%s = %d\n", property_name, this->contact_count_for_cond[cond_id]);

    this->contact_rate_for_cond[cond_id] = 0.0;
    sprintf(property_name, "%s.contact_rate_for_%s", this->name.c_str(), cond_name.c_str());
    Property::get_property(property_name, &this->contact_rate_for_cond[cond_id]);
    FRED_STATUS(0, "%s = %f\n", property_name, this->contact_rate_for_cond[cond_id]);

    this->deterministic_contacts_for_cond[cond_id] = true;
    sprintf(property_name, "%s.deterministic_contacts_for_%s", this->name.c_str(), cond_name.c_str());
    Property::get_property(property_name, &this->deterministic_contacts_for_cond[cond_id]);
    FRED_STATUS(0, "%s = %d\n", property_name, this->deterministic_contacts_for_cond[cond_id]);

  }

  // group type schedule
  for (int day = 0; day < 7; day++) {
    for (int hour = 0; hour < 24; hour++) {
      this->open_at_hour[day][hour] = 0;
      this->starts_at_hour[day][hour] = 0;
      std::string dayname;
      switch (day) {
      case 0: dayname = "Sun"; break;
      case 1: dayname = "Mon"; break;
      case 2: dayname = "Tue"; break;
      case 3: dayname = "Wed"; break;
      case 4: dayname = "Thu"; break;
      case 5: dayname = "Fri"; break;
      case 6: dayname = "Sat"; break;
      }
      sprintf(property_name, "%s.starts_at_hour_%d_on_%s", this->name.c_str(), hour, dayname.c_str());
      if (Property::does_property_exist(property_name)) {
	Property::get_property(property_name, &this->starts_at_hour[day][hour]);
	printf("%s = %d\n", property_name, this->starts_at_hour[day][hour]);
      }
    }
  }

  // shortcuts
  for (int hour = 0; hour < 24; hour++) {
    sprintf(property_name, "%s.starts_at_hour_%d_on_weekdays", this->name.c_str(), hour);
    if (Property::does_property_exist(property_name)) {
      Property::get_property(property_name, &this->starts_at_hour[1][hour]);
      Property::get_property(property_name, &this->starts_at_hour[2][hour]);
      Property::get_property(property_name, &this->starts_at_hour[3][hour]);
      Property::get_property(property_name, &this->starts_at_hour[4][hour]);
      Property::get_property(property_name, &this->starts_at_hour[5][hour]);
    }
    sprintf(property_name, "%s.starts_at_hour_%d_on_weekends", this->name.c_str(), hour);
    if (Property::does_property_exist(property_name)) {
      Property::get_property(property_name, &this->starts_at_hour[0][hour]);
      Property::get_property(property_name, &this->starts_at_hour[6][hour]);
      printf("%s = %d\n", property_name, this->starts_at_hour[0][hour]);
    }
  }

  for (int day = 0; day < 7; day++) {
    for (int hour = 0; hour < 24; hour++) {
      int hr = hour;
      int d = day;
      for (int i = 0; i < this->starts_at_hour[day][hour]; i++) {
	this->open_at_hour[d][hr] = 1;
	hr++;
	if (hr == 24) {
	  hr = 0;
	  d++;
	  if (d == 7) {
	    d = 0;
	  }
	}
      }
    }
  }

  for (int day = 0; day < 7; day++) {
    for (int hour = 0; hour < 24; hour++) {
      if (this->starts_at_hour[day][hour]) {
	FRED_VERBOSE(0, "%s hour %d day %d time_block %d\n",
		     this->name.c_str(), hour, day, this->starts_at_hour[day][hour]);
      }
    }
  }

  // does this group type have adminstrators?
  int n = 0;
  sprintf(property_name, "%s.has_administrator", this->name.c_str());
  Property::get_property(property_name, &n);
  this->has_admin = n;
  FRED_STATUS(0, "group_type %s read_properties %s = %d\n", this->name.c_str(), property_name, n);

  // if this group type has an admin, then create a global list variable for the type
  if (this->has_admin) {
    Person::include_global_list_variable(this->name + "List");
  }

  Property::set_abort_on_failure();

  FRED_STATUS(0, "group_type %s read_properties finished\n", this->name.c_str());
}

//////////////////////////
//
// STATIC METHODS
//
//////////////////////////

int Group_Type::get_type_id(string type_name) {
  std::unordered_map<std::string,int>::const_iterator found = Group_Type::group_name_map.find(type_name);
  if ( found == Group_Type::group_name_map.end() ) {
    FRED_VERBOSE(1, "Help: GROUP_TYPE can't find a group type named %s\n", type_name.c_str());
    return -1;
  }
  else {
    return found->second;
  }
}


bool Group_Type::is_open() {
  return this->open_at_hour[Date::get_day_of_week(Global::Simulation_Day)][Global::Simulation_Hour];
}

int Group_Type::get_time_block(int day, int hour) {
  int weekday = Date::get_day_of_week(day);
  int value = this->starts_at_hour[weekday][hour];
  FRED_VERBOSE(1, "get_time_block %s day %d day_of_week %d hour %d value %d\n",
	       this->name.c_str(), day, weekday, hour, value);
  return value;
}


