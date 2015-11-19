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
#include "Neighborhood_Patch.h"
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
  void get_vector_population(int disease_id);

  std::vector<int> read_vector_control_tracts(char * filename);
  void setup_vector_control_by_census_tract();
  int select_places_for_vector_control(Neighborhood_Patch * patch_n, int day);
  void update_vector_control_by_census_tract(int day);
  void add_infectious_patch(Place * p, int day);
  bool get_vector_control_status(){return this->Enable_Vector_Control;}

protected:
  void get_county_ids();
  void get_immunity_from_file();
  void get_immunity_from_file(int d);
  void get_people_size_by_age();
  void immunize_total_by_age();
  void immunize_by_age(int d);
  void seed_patches_by_distance_in_km(fred::geo lat, fred::geo lon, double radius_in_km, int dis,int day_on, int day_off,double seeds_);

  Vector_Patch ** grid;			 // Rectangular array of patches

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

  std::vector<int>census_tracts_with_vector_control;

  int vector_pop;
  int total_infected_vectors;
  int school_infected_vectors;
  int workplace_infected_vectors;
  int household_infected_vectors;
  int neighborhood_infected_vectors;
  int total_susceptible_vectors;
  int total_infected_hosts;
  int total_infectious_hosts;
  int school_vectors;
  int workplace_vectors;
  int household_vectors;
  int neighborhood_vectors;

  // vector control parameters
  int total_places_in_vector_control;
  int schools_in_vector_control;
  int households_in_vector_control;
  int workplaces_in_vector_control;
  int neighborhoods_in_vector_control;
  int vector_control_day_on;
  int vector_control_day_off;
  int vector_control_max_places;
  int vector_control_places_enrolled;
  int vector_control_random;
  double vector_control_threshold;
  double vector_control_coverage;
  double vector_control_efficacy;
  double vector_control_neighborhoods_rate;


  static bool Enable_Vector_Control;
  static bool School_Vector_Control;
  static bool Workplace_Vector_Control;
  static bool Household_Vector_Control;
  static bool Limit_Vector_Control;
  static bool Neighborhood_Vector_Control;
};

#endif // _FRED_VECTOR_LAYER_H
