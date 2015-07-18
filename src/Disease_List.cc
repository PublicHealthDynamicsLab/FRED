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

#include <stdio.h>
#include <string>
#include "Disease_List.h"
#include "Disease.h"


void Disease_List::get_parameters(int num_of_diseases) {
  this->diseases.clear();
  this->number_of_diseases = num_of_diseases;
  int num_diseases = Global::Dis.get_number_of_diseases();

  this->disease_name = new string[num_of_diseases];
  Params::get_param_vector((char *)"disease_names", this->disease_name);
  for(int d = 0; d < num_of_diseases; ++d) {
    printf("disease %d = %s\n", d, this->disease_name[d].c_str());
  }

  for(int disease_id = 0; disease_id < num_of_diseases; ++disease_id) {
    Disease * disease = new Disease();
    disease->get_parameters(disease_id, this->disease_name[disease_id]);
    this->diseases.push_back(disease);
  }
}

void Disease_List::setup() {
  for(int disease_id = 0; disease_id < this->number_of_diseases; ++disease_id) {
    this->diseases[disease_id]->setup();
  }
}


