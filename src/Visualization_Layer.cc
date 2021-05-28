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
// File: Visualization_Layer.cc
//

#include <list>
#include <string>
#include <utility>
using namespace std;

#include "Global.h"
#include "Condition.h"
#include "Property.h"
#include "Place.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Visualization_Layer.h"
#include "Visualization_Patch.h"
#include "Utils.h"


Visualization_Layer::Visualization_Layer() {
  // create visualization data directories
  sprintf(Global::Visualization_directory, "%s/RUN%d/VIS",
	  Global::Simulation_directory,
	  Global::Simulation_run_number);
  Utils::fred_make_directory(Global::Visualization_directory);

  // get optional properties:
  Property::disable_abort_on_failure();
  this->period = 1;
  Property::get_property("visualization_period", &this->period);
  Property::set_abort_on_failure();

  this->rows = 0;
  this->cols = 0;
}

Visualization_Patch* Visualization_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &(this->grid[row][col]);
  } else {
    return NULL;
  }
}

Visualization_Patch* Visualization_Layer::get_patch(double x, double y) {
  int row = get_row(y);
  int col = get_col(x);
  return get_patch(row,col);
}


void Visualization_Layer::update_data(double x, double y, int count, int popsize) {
  Visualization_Patch* patch = get_patch(x, y);
  if(patch != NULL) {
    patch->update_patch_count(count, popsize);
  }
}




