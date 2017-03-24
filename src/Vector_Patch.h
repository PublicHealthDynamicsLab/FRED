/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

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
  void set_mosquito_index(double index_);	
  void set_vector_seeds(int dis,int day_on, int day_off,double seeds_);
  double get_temperature(){return temperature;}
  double get_mosquito_index(){return house_index;}
  double get_seeds(int dis) { return seeds[dis];}
  int get_day_start_seed(int dis) { return day_start_seed[dis];}
  int get_day_end_seed(int dis) {return day_end_seed[dis];}
  double get_seeds(int dis, int day);

  int select_places_for_vector_control(double coverage_, double efficacy_, int day_on, int day_off) { return 0; }
  int get_vector_population() { return 0; }
  int get_school_vector_population() { return 0; }
  int get_workplace_vector_population() { return 0; }
  int get_household_vector_population() { return 0; }
  int get_neighborhood_vector_population() { return 0; }
  int get_school_infected_vectors() { return 0; }
  int get_workplace_infected_vectors() { return 0; }
  int get_household_infected_vectors() { return 0; }
  int get_neighborhood_infected_vectors() { return 0; }
  int get_susceptible_vectors() { return 0; }
  int get_infectious_hosts() { return 0; }
  int get_infected_vectors() { return 0; }
  int get_schools_in_vector_control() { return 0; }
  int get_households_in_vector_control() { return 0; }
  int get_workplaces_in_vector_control() { return 0; }
  int get_neighborhoods_in_vector_control() { return 0; }

  /**
   *  Vector control functions
   */
  void set_census_tract_index(int in_){census_tract_index = in_;}

  void make_eligible_for_vector_control(){eligible_for_vector_control = true;}

  void register_for_vector_control(){registered_for_vector_control = true;}

  bool is_eligible_for_vector_control(){return this->eligible_for_vector_control;}

  bool is_registered_for_vector_control(){return this->registered_for_vector_control;}

  int get_census_tract_index(){return this->census_tract_index;}


protected:
  double suitability;

  // Vectors per host
  double temperature;
  double house_index;
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

  int census_tract_index;
  bool eligible_for_vector_control;
  bool registered_for_vector_control;
};

#endif // _FRED_VECTOR_PATCH_H
