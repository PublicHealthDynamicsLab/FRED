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

#include "Disease_List.h"
#include "Global.h"
#include "Disease.h"


void Disease_List::setup(int num_diseases) {
  this->diseases.clear();
  for(int disease_id = 0; disease_id < num_diseases; ++disease_id) {
    Disease * disease = new Disease;
    this->diseases.push_back(disease);
  }
}

