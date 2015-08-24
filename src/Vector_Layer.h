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
// File: Vector_Layer.h
//

#ifndef _FRED_VECTOR_LAYER_H
#define _FRED_VECTOR_LAYER_H

#include "Global.h"
#include "Abstract_Grid.h"
#include <fstream>

class Epidemic;
class Person;
class Place;
class Vector_Patch;

class Vector_Layer : public Abstract_Grid {

public:
  Vector_Layer();
  ~Vector_Layer() {}
  void setup();
  Vector_Patch ** get_neighbors(int row, int col);
  Vector_Patch * get_patch(int row, int col);
  Vector_Patch * get_patch(fred::geo lat, fred::geo lon);
  Vector_Patch * select_random_patch();
  void quality_control();
  void update(int day);
  void update_visualization_data(int disease_id, int day);
  void add_hosts(Place * p);
  double get_vectors_per_host(Place * p);
  double get_seeds(Place * place, int dis, int day);
  void add_host(Person * person, Place * place);
  void read_temperature();
  void read_vector_seeds();
  void swap_county_people();
  int get_total_infected_vectors() { return total_infected_vectors; }
  int get_total_infected_hosts() { return total_infected_hosts; }
  int get_total_infectious_hosts() { return total_infectious_hosts; }
  void init_prior_immunity_by_county();
  void init_prior_immunity_by_county(int d);
  double get_infection_efficiency() { return infection_efficiency; }
  double get_transmission_efficiency() { return transmission_efficiency; }
  double get_seeds(Place * p, int dis);
  double get_day_start_seed(Place * p, int dis);
  double get_day_end_seed(Place * p, int dis);
  void report(int day, Epidemic * epidemic);
  vector_disease_data_t update_vector_population(int day, Place * place);
  double get_bite_rate() { return this->bite_rate; }

protected:
  void get_county_ids();
  void get_immunity_from_file();
  void get_immunity_from_file(int d);
  void get_people_size_by_age();
  void immunize_total_by_age();
  void immunize_by_age(int d);
  void seed_patches_by_distance_in_km(fred::geo lat, fred::geo lon, double radius_in_km, int dis,int day_on, int day_off,double seeds_);
  Vector_Patch ** grid;			 // Rectangular array of patches
  int total_infected_vectors;
  int total_infected_hosts;
  int total_infectious_hosts;

  // fixed parameters for this disease vector
  double infection_efficiency;
  double transmission_efficiency;
  double place_seeding_probability;
  double mosquito_seeds;
  double death_rate;
  double birth_rate;
  double bite_rate;
  double incubation_rate;
  double suitability;
  double pupae_per_host;
  double life_span;
  double sucess_rate;
  double female_ratio;

  // vector control parameters
  static bool Enable_Vector_Control;
  static bool School_Vector_Control;
  static bool Workplace_Vector_Control;
  static bool Household_Vector_Control;
  static bool Limit_Vector_Control;
  static bool Neighborhood_Vector_Control;
};

#endif // _FRED_VECTOR_LAYER_H
