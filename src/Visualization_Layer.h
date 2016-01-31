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
// File: Visualization_Layer.h
//

#ifndef _FRED_VISUALIZATION_GRID_H
#define _FRED_VISUALIZATION_GRID_H

#include <vector>

#include "Abstract_Grid.h"
#include "Global.h"
#include "Place.h"


typedef std::pair<double, double> point;

class Visualization_Patch;

class Visualization_Layer : public Abstract_Grid {
public:
  Visualization_Layer();
  ~Visualization_Layer() {}
  void setup();
  Visualization_Patch** get_neighbors(int row, int col);
  Visualization_Patch* get_patch(int row, int col);
  Visualization_Patch* get_patch(fred::geo lat, fred::geo lon);
  Visualization_Patch* get_patch(double x, double y);
  Visualization_Patch* select_random_patch();
  void quality_control();
  void add_census_tract(unsigned long long tract);
  void print_visualization_data(int day);
  void print_vector_data(char* dir, int condition_id, int day);
  void add_household(Place* h) {
    this->households.push_back(h);
  }

  void print_hazel_household_data(int condition_id, int day);
  void print_census_tract_data(int condition_id, int output_code, char* output_str, int day);
  void initialize_household_data(fred::geo latitude, fred::geo longitude, int count);
  void update_data(fred::geo latitude, fred::geo longitude, int count, int popsize);
  void update_data(double x, double y, int count, int popsize);
  void update_data(unsigned long long tract, int count, int popsize);

protected:
  Visualization_Patch** grid;            // Rectangular array of patches
  int max_grid_size;		    // maximum number of rows or columns
  int gaia_mode;			// if set, collect data for GAIA
  int household_mode;		  // if set, collect data for households
  int census_tract_mode;	// if set, collect data for census_tract
  vector<Place*> households;

};

#endif // _FRED_VISUALIZATION_GRID_H
