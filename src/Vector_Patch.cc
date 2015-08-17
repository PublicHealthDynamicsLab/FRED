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
// File: Vector_Patch.cc
//

#include "Vector_Patch.h"
#include "Vector_Layer.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Params.h"
#include "Utils.h"
#include "Person.h"
#include "Random.h"

#include <vector>
using namespace std;

void Vector_Patch::setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y) {
  row = i;
  col = j;
  min_x = grid_min_x + (col)*patch_size;
  min_y = grid_min_y + (row)*patch_size;
  max_x = grid_min_x + (col+1)*patch_size;
  max_y = grid_min_y + (row+1)*patch_size;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;

  for (int i = 0; i < DISEASE_TYPES; i++) {
    seeds[i] = 0.0;
    day_start_seed[i] = 0;
    day_end_seed[i] = 0;
  }	
  temperature = 0;
}

double Vector_Patch::distance_to_patch(Vector_Patch *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

void Vector_Patch::set_temperature(double patch_temperature){
  //temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
  double temps[8] = {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92}; //temperatures
  double dev_times[8] = {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0}; //development times
  temperature = patch_temperature;
  if (temperature > 32) temperature = 32;
  FRED_VERBOSE(1, "SET TEMP: Patch %d %d temp %f\n", row, col, patch_temperature);

}

double Vector_Patch::get_seeds(int dis, int day) {
  if((day<day_start_seed[dis]) ||( day>day_end_seed[dis])){
    return 0.0;
  }
  else {
    return seeds[dis];
  }
}

void Vector_Patch::set_vector_seeds(int dis,int day_on, int day_off,double seeds_){
  seeds[dis] = seeds_;
  day_start_seed[dis] = day_on;
  day_end_seed[dis] = day_off;
  FRED_VERBOSE(1, "SET_VECTOR_SEEDS: Patch %d %d proportion of susceptible for disease [%d]: %f. start: %d end: %d\n", row, col, dis,seeds[dis],day_on,day_off);
}
void Vector_Patch::print() {
  FRED_VERBOSE(0, "Vector_patch: %d %d\n",
	       row, col);
}


