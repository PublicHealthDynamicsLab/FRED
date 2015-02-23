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
// File: Visualization_Patch.cc
//

#include "Visualization_Patch.h"
#include "Visualization_Layer.h"
#include "Utils.h"

void Visualization_Patch::setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y) {
  this->row = i;
  this->col = j;
  this->min_x = grid_min_x + (this->col) * patch_size;
  this->min_y = grid_min_y + (this->row) * patch_size;
  this->max_x = grid_min_x + (this->col + 1)* patch_size;
  this->max_y = grid_min_y + (this->row + 1) * patch_size;
  this->center_y = (this->min_y + this->max_y) / 2.0;
  this->center_x = (this->min_x + this->max_x) / 2.0;
  reset_counts();
}

void Visualization_Patch::quality_control() {
  return;
}

double Visualization_Patch::distance_to_patch(Visualization_Patch* p2) {
  double x1 = this->center_x;
  double y1 = this->center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void Visualization_Patch::print() {
  FRED_VERBOSE(0, "visualization_patch: %d %d %d %d\n", row, col, count, popsize);
}


