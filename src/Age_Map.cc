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
// File: Age_Map.cpp
//

#include "Age_Map.h"
#include "Property.h"
#include "Utils.h"


Age_Map::Age_Map() {
  ages.clear();
  values.clear();
}


void Age_Map::read_properties(string prefix) {
  // FRED_VERBOSE(0,"read_properties for prefix = |%s|\n", prefix.c_str());
  this->name = prefix;
  this->ages.clear();
  this->values.clear();

  // make the following properties optional
  Property::disable_abort_on_failure();

  char property_string[FRED_STRING_SIZE];

  sprintf(property_string, "%s.age_groups", prefix.c_str());
  Property::get_property_vector(property_string, this->ages);

  sprintf(property_string, "%s.age_values", prefix.c_str());
  Property::get_property_vector(property_string, this->values);

  // restore requiring properties
  Property::set_abort_on_failure();

  if(quality_control() != true) {
    Utils::fred_abort("Bad input on age map %s", this->name.c_str());
  }
  return;
}

double Age_Map::find_value(double age) {
  // printf("find_value: age = %.1f  groups %d \n", age, (int) this->ages.size());

  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if (age < this->ages[i]) {
      return this->values[i];
    }
  }
  return 0.0;
}

bool Age_Map::quality_control() const {

  // printf("AGE_MAP quality_control for %s\n", this->name.c_str());
  // printf("age groups = %d  values = %d\n", (int) this->ages.size(), (int) this->values.size());

  // number of groups and number of values must agree
  if(this->ages.size() != this->values.size()) {
    printf("Help! Age_Map %s number of groups = %lu but number of values = %lu\n",
	   this->name.c_str(), this->ages.size(), this->values.size());
    return false;
  }

  if (this->ages.size() > 0) {
    // age groups must have increasing upper bounds
    for(unsigned int i = 0; i < this->ages.size()-1; i++) {
      if(this->ages[i] > this->ages[i+1]) {
	printf("Help! Age_Map %s upper bound %d %f > upper bound %d %f\n",
	       this->name.c_str(), i, this->ages[i], i+1, this->ages[i+1]);
	return false;
      }
    }
  }
  return true;
}

