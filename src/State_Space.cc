/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "State_Space.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"

State_Space::State_Space(char* model_name) {
  strcpy(this->name, model_name);
  this->number_of_states = -1;
  this->state_name.clear();
}

State_Space::~State_Space() {
}

void State_Space::get_parameters() {

  FRED_VERBOSE(0, "State_Space(%s)::get_parameters\n", this->name);

  // read optional parameters
  Params::disable_abort_on_failure();
  
  // read in the list of state names
  this->state_name.clear();
  char param_name[FRED_STRING_SIZE];
  char param_value[FRED_STRING_SIZE];
  sprintf(param_name, "%s.%s", this->name, "states");
  Params::get_param(param_name, param_value);

  // parse the param value into separate strings
  char *pch;
  double v;
  pch = strtok(param_value," ");
  while (pch != NULL) {
    this->state_name.push_back(string(pch));
    pch = strtok(NULL, " ");
  }
  this->number_of_states = this->state_name.size();

  FRED_VERBOSE(0, "state space %s number of states = %d\n",
	       this->name, this->number_of_states);

  // restore requiring parameters
  Params::set_abort_on_failure();

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



