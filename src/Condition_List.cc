/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

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
  this->conditions.clear();
  Params::get_param_from_string("conditions", &this->number_of_conditions);

  // sanity check
  if(this->number_of_conditions > Global::MAX_NUM_CONDITIONS) {
    Utils::fred_abort("Condition_List::number_of_conditions (= %d) > Global::MAX_NUM_CONDITIONS (= %d)!",
		      this->number_of_conditions, Global::MAX_NUM_CONDITIONS);
  }

  // get condition names
  this->condition_name = new string[this->number_of_conditions];
  Params::get_param_vector((char *)"condition_names", this->condition_name);
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
    if (Global::Enable_Viral_Evolution) {
      condition->get_natural_history()->initialize_evolution_reporting_grid(Global::Simulation_Region);
      condition->get_natural_history()->init_prior_immunity();
    }
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
