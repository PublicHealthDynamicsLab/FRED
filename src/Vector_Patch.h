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
// File: Vector_Patch.h
//

#ifndef _FRED_VECTOR_PATCH_H
#define _FRED_VECTOR_PATCH_H

#include "Global.h"
#include "Abstract_Patch.h"
#include <vector>
using namespace std;

class Vector_Layer;
class Place;
class Person;

#define DISEASE_TYPES 4

class Vector_Patch : public Abstract_Patch {
public:
  Vector_Patch() {}
  ~Vector_Patch() {}
  void setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y);
  void quality_control() { return; }
  double distance_to_patch(Vector_Patch *patch2);
  void print();
  void set_temperature(double patch_temperature);	
  void set_vector_seeds(int dis,int day_on, int day_off,double seeds_);
  double get_temperature(){return temperature;}
  double get_seeds(int dis) { return seeds[dis];}
  int get_day_start_seed(int dis) { return day_start_seed[dis];}
  int get_day_end_seed(int dis) {return day_end_seed[dis];}
  double get_seeds(int dis, int day);

protected:
  double suitability;

  // Vectors per host
  double temperature;
  double vectors_per_host;
  double pupae_per_host;
  double life_span;
  double sucess_rate;
  double female_ratio;
  double development_time;

  // proportion of imported or born infectious
  double seeds[DISEASE_TYPES];

  // day on and day off of seeding mosquitoes in the patch
  int day_start_seed[DISEASE_TYPES];
  int day_end_seed[DISEASE_TYPES];

};

#endif // _FRED_VECTOR_PATCH_H
