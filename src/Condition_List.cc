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

//
//
// File: Condition_List.cc
//

#include <string>
#include "Condition_List.h"
#include "Condition.h"
#include "Natural_History.h"
#include "Global.h"
#include "Params.h"

void Condition_List::get_parameters() {

  // read in the list of condition names
  this->conditions.clear();
  char param_name[FRED_STRING_SIZE];
  char param_value[FRED_STRING_SIZE];
  strcpy(param_name, "conditions");
  Params::get_param(param_name, param_value);

  // parse the param value into separate strings
  char *pch;
  double v;
  pch = strtok(param_value," ");
  while (pch != NULL) {
    this->condition_name.push_back(string(pch));
    pch = strtok(NULL, " ");
  }
  this->number_of_conditions = this->condition_name.size();

  // check if behaviors are treated as conditions
  /*
  if (Global::Enable_Conditional_Behaviors) {
    // add behaviors as extra conditions
    this->condition_name.push_back("take_sick_leave");
    this->number_of_conditions++;
  }
  */

  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {

    // create new Condition object
    Condition * condition = new Condition();

    // get its parameters
    condition->get_parameters(condition_id, this->condition_name[condition_id]);
    this->conditions.push_back(condition);
    printf("condition %d = %s\n", condition_id, this->condition_name[condition_id].c_str());
  }
}

void Condition_List::setup() {
  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    this->conditions[condition_id]->setup();
  }
}

void Condition_List::prepare_conditions() {
  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    Condition *condition = this->conditions[condition_id]; 
    condition->prepare();
  }
}


Condition* Condition_List::get_condition(char * condition_name) {
  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    if (strcmp(condition_name, this->conditions[condition_id]->get_condition_name()) == 0) {
      return this->conditions[condition_id]; 
    }
  }
  return NULL;
}


void Condition_List::end_of_run() {
  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    this->conditions[condition_id]->end_of_run(); 
  }
}
