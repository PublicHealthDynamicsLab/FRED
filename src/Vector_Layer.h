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
// File: Vector_Layer.h
//

#ifndef _FRED_VECTOR_LAYER_H
#define _FRED_VECTOR_LAYER_H
#define  DISEASE_TYPES 4
#include "Global.h"
#include "Abstract_Grid.h"
#include <fstream>
class Vector_Patch;
class Place;
class Person;


class Vector_Layer : public Abstract_Grid {

public:
  Vector_Layer();
  ~Vector_Layer() {}
  Vector_Patch ** get_neighbors(int row, int col);
  Vector_Patch * get_patch(int row, int col);
  Vector_Patch * get_patch(fred::geo lat, fred::geo lon);
  Vector_Patch * select_random_patch();
  void quality_control();
  void initialize();
  void update(int day);
  void update_visualization_data(int disease_id, int day);
  void add_hosts(Place * p);
  double get_temperature(Place * p);
  double get_seeds(Place * place, int dis, int day);
  void add_host(Person * person, Place * place);
  void read_temperature();
  void read_vector_seeds();
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
  double infection_efficiency;
  double transmission_efficiency;
  double place_seeding_probability;
  double mosquito_seeds;
};

#endif // _FRED_VECTOR_LAYER_H
