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
// File: Disease_List.cc
//

#include <string>
#include "Disease_List.h"
#include "Disease.h"
#include "Params.h"

void Disease_List::get_parameters() {
  this->diseases.clear();
  Params::get_param_from_string("diseases", &this->number_of_diseases);

  // sanity check
  if(this->number_of_diseases > Global::MAX_NUM_DISEASES) {
    Utils::fred_abort("Disease_List::number_of_diseases (= %d) > Global::MAX_NUM_DISEASES (= %d)!",
		      this->number_of_diseases, Global::MAX_NUM_DISEASES);
  }

  // get disease names
  this->disease_name = new string[this->number_of_diseases];
  Params::get_param_vector((char *)"disease_names", this->disease_name);
  for(int disease_id = 0; disease_id < this->number_of_diseases; ++disease_id) {
    // create new Disease object
    Disease * disease = new Disease();

    // get its parameters
    disease->get_parameters(disease_id, this->disease_name[disease_id]);
    this->diseases.push_back(disease);
    printf("disease %d = %s\n", disease_id, this->disease_name[disease_id].c_str());
  }
}

void Disease_List::setup() {
  for(int disease_id = 0; disease_id < this->number_of_diseases; ++disease_id) {
    this->diseases[disease_id]->setup();
  }
}

void Disease_List::prepare_diseases() {
  for(int disease_id = 0; disease_id < this->number_of_diseases; ++disease_id) {
    Disease *disease = this->diseases[disease_id]; 
    disease->initialize_evolution_reporting_grid(Global::Simulation_Region);
    if(!Global::Enable_Vector_Layer) {
      disease->init_prior_immunity();
    }
  }
}


