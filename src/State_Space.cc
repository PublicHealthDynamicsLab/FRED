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

#include "State_Space.h"
#include "Property.h"
#include "Random.h"
#include "Utils.h"

State_Space::State_Space(char* model_name) {
  strcpy(this->name, model_name);
  this->number_of_states = -1;
  this->state_name.clear();
}

State_Space::~State_Space() {
}

void State_Space::get_properties() {

  FRED_VERBOSE(0, "State_Space(%s)::get_properties\n", this->name);

  // read optional properties
  Property::disable_abort_on_failure();
  
  // read in the list of state names
  this->state_name.clear();
  char property_name[FRED_STRING_SIZE];
  char property_value[FRED_STRING_SIZE];
  strcpy(property_value, "");
  sprintf(property_name, "%s.%s", this->name, "states");
  Property::get_property(property_name, property_value);

  this->number_of_states = 0;
  if (strcmp(property_value, "")!=0) {
    // parse the property value into separate strings
    char *pch;
    double v;
    pch = strtok(property_value," \t\n\r");
    while (pch != NULL) {
      this->state_name.push_back(string(pch));
      pch = strtok(NULL, " \t\n\r");
    }
    this->number_of_states = this->state_name.size();
  }

  if (this->number_of_states == 0) {
    this->state_name.push_back("Start");
    this->number_of_states = 1;
  }

  for (int i = 0; i < this->number_of_states-1; i++) {
    for (int j = i+1; j < this->number_of_states; j++) {
      if (this->state_name[i] == this->state_name[j]) {
	char msg[FRED_STRING_SIZE];
	sprintf(msg,"Duplicate state %s found in Condition %s\n", this->state_name[i].c_str(), this->name);
	Utils::print_error(msg);
      }
    }
  }

  FRED_VERBOSE(0, "state space %s number of states = %d\n",
	       this->name, this->number_of_states);

  // restore requiring properties
  Property::set_abort_on_failure();

}


void State_Space::print() {
  for(int i = 0; i < this->number_of_states; ++i) {
    printf("STATE SPACE %s[%d].name = %s\n",
	   this->name, i, this->state_name[i].c_str());
  }
}


int State_Space::get_state_from_name(char* sname) {
  for (int i = 0; i < this->number_of_states; ++i) {
    string str = sname;
    if (str == this->state_name[i]) {
      return i;
    }
  }
  return -1;
}

int State_Space::get_state_from_name(string sname) {
  for (int i = 0; i < this->number_of_states; ++i) {
    if (sname == this->state_name[i]) {
      return i;
    }
  }
  return -1;
}



