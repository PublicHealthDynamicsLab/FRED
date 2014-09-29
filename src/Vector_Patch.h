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
  void setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y,double trans_efficiency, double infec_efficiency);
  void quality_control() { return; }
  double distance_to_patch(Vector_Patch *patch2);
  void print();
  void update(int day);
  void set_temperature(double patch_temperature);	
  void set_vector_seeds(int dis,int day_on, int day_off,double seeds_);
  void add_hosts(int n) { N_vectors += n * vectors_per_host; }
  void add_host(Person * person);
  int get_vector_population_size() { return N_vectors; }
  int get_number_of_infected_hosts(int disease_id) { return E_hosts[disease_id]; }
  double get_temperature(){return temperature;}
  double get_seeds(int dis) { return seeds[dis];}
  int get_day_start_seed(int dis) { return day_start_seed[dis];}
  int get_day_end_seed(int dis) {return day_end_seed[dis];}
  double get_seeds(int dis, int day);
  int get_infected_vectors() {
    int n = 0;
    for (int i = 0; i < DISEASE_TYPES; i++)
      n += E_vectors[i] + I_vectors[i];
    return n;
  }
  int get_infectious_vectors() {
    int n = 0;
    for (int i = 0; i < DISEASE_TYPES; i++)
      n += I_vectors[i];
    return n;
  }
  int get_newly_infected_vectors() {
    int n = 0;
    for (int i = 0; i < DISEASE_TYPES; i++)
      n += E_vectors[i];
    return n;
  }
  int get_infected_hosts() {
    int n = 0;
    for (int i = 0; i < DISEASE_TYPES; i++)
      n += E_hosts[i];
    return n;
  }

protected:
  void infect_vectors(int day);
  void transmit_to_hosts(int day);
  void update_vector_population(int day);
  double death_rate;
  double birth_rate;
  double bite_rate;
  double incubation_rate;
  double suitability;
  double transmission_efficiency;
  double infection_efficiency;

  // Vectors per host
  double temperature;
  double vectors_per_host;
  double pupae_per_host;
  double life_span;
  double sucess_rate;
  double female_ratio;
  double development_time;
  // counts for vectors
  int N_vectors;
  int S_vectors;
  int E_vectors[DISEASE_TYPES];
  int I_vectors[DISEASE_TYPES];
  // proportion of imported or born infectious
  double seeds[DISEASE_TYPES];
  // day on and day off of seeding mosquitoes in the patch
  int day_start_seed[DISEASE_TYPES];
  int day_end_seed[DISEASE_TYPES];

  // counts for hosts
  int N_hosts;
  std::vector<Person *> S_hosts[DISEASE_TYPES];
  int E_hosts[DISEASE_TYPES];
  int I_hosts[DISEASE_TYPES];
  int R_hosts[DISEASE_TYPES];

};

#endif // _FRED_VECTOR_PATCH_H
