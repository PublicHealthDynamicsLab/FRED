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
  static double** prob_contact;

  // static seasonal transmission parameters
  static double Seasonal_Reduction;
  static double* Seasonality_multiplier;

  ~Transmission() {}
  static void get_parameters();
  static void spread_infection(int day, int disease_id, Place *place);
  static void default_transmission_model(int day, int disease_id, Place *place);
  static void age_based_transmission_model(int day, int disease_id, Place *place);
  static void pairwise_transmission_model(int day, int disease_id, Place *place);
  static void density_transmission_model(int day, int disease_id, Place *place);

  static bool attempt_transmission(double transmission_prob, Person * infector, Person * infectee, int disease_id, int day, Place* place);

  // vector transmission model
  static void vector_transmission(int day, int disease_id, Place * place);
  static void infect_vectors(int day, Place * place);
  static void infect_hosts(int day, int disease_id, Place * place);
  static void update_vector_population(int day, Place * place);

protected:
};


#endif // _FRED_TRANSMISSION_H
