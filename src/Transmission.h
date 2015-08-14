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
// File: Place.h
//

#ifndef _FRED_TRANSMISSION_H
#define _FRED_TRANSMISSION_H

#include <vector>
using namespace std;

#include "Global.h"
#define DISEASE_TYPES 4

class Place;
class Person;

class Transmission {

public:
  
  // place-specific transmission mode parameters
  static bool Enable_Neighborhood_Density_Transmission;
  static bool Enable_Density_Transmission_Maximum_Infectees;
  static int Density_Transmission_Maximum_Infectees;

  // static seasonal transmission parameters
  static double Seasonal_Reduction;
  static double* Seasonality_multiplier;

  // place type codes
  static char HOUSEHOLD;
  static char NEIGHBORHOOD;
  static char SCHOOL;
  static char CLASSROOM;
  static char WORKTRANSMISSION;
  static char OFFICE;
  static char HOSPITAL;
  static char COMMUNITY;

  virtual ~Transmission() {}
  void get_parameters();
  void setup();

  void spread_infection(int day, int disease_id, Place *place);
  void default_transmission_model(int day, int disease_id, Place *place);
  void age_based_transmission_model(int day, int disease_id, Place *place);
  void pairwise_transmission_model(int day, int disease_id, Place *place);
  void density_transmission_model(int day, int disease_id, Place *place);

  bool attempt_transmission(double transmission_prob, Person * infector, Person * infectee, int disease_id, int day, Place* place);

  // vector transmission model
  void vector_transmission(int day, int disease_id, Place * place);
  void infect_vectors(int day, Place * place);
  void vectors_transmit_to_hosts(int day, int disease_id, Place * place);
  void update_vector_population(int day, Place * place);

protected:
  // lists of people
  std::vector<Person*> * infectious_people;
  std::vector<Person*> enrollees;

  // data for vector transmission model
  double death_rate;
  double birth_rate;
  double bite_rate;
  double incubation_rate;
  double suitability;
  double transmission_efficiency;
  double infection_efficiency;

  // vectors per host
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
  double place_seeds[DISEASE_TYPES];

  // day on and day off of seeding mosquitoes in the patch
  int day_start_seed[DISEASE_TYPES];
  int day_end_seed[DISEASE_TYPES];

  // counts for hosts
  int N_hosts;
  bool vectors_not_infected_yet;

};


#endif // _FRED_TRANSMISSION_H
