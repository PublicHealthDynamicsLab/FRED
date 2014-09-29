/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Visualization_Patch.h
//

#ifndef _FRED_VISUALIZATION_PATCH_H
#define _FRED_VISUALIZATION_PATCH_H

#include "Global.h"
#include "Abstract_Patch.h"

class Visualization_Layer;

class Visualization_Patch : public Abstract_Patch {
public:
  Visualization_Patch() {}
  ~Visualization_Patch() {}
  void setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y);
  void quality_control();
  double distance_to_patch(Visualization_Patch *patch2);
  void print();

  void reset_counts() { count = 0; popsize = 0; }
  void update_patch_count(int n, int total) { count += n; popsize += total; } 
  int get_count() { return count; }
  int get_popsize() { return popsize; }

protected:
  int count;
  int popsize;
};

#endif // _FRED_VISUALIZATION_PATCH_H
