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
// File: Transmission.cc
//

#include <math.h>

#include "Transmission.h"
#include "Respiratory_Transmission.h"
#include "Sexual_Transmission.h"
#include "Vector_Transmission.h"
#include "Params.h"
#include "Utils.h"

#define PI 3.14159265359
double Transmission::Seasonal_Reduction = 0.0;
double* Transmission::Seasonality_multiplier = NULL;


Transmission * Transmission::get_new_transmission(char* transmission_mode) {
  
  if (strcmp(transmission_mode, "respiratory") == 0) {
    return new Respiratory_Transmission();
  }
  
  if (strcmp(transmission_mode, "vector") == 0) {
    return new Vector_Transmission();
  }

  if (strcmp(transmission_mode, "sexual") == 0) {
    return new Sexual_Transmission();
  }

  Utils::fred_abort("Unknown transmission_mode (%s).\n", transmission_mode);
  return NULL;
}



void Transmission::get_parameters() {

  // all-disease seasonality reduction
  Params::get_param_from_string("seasonal_reduction", &Transmission::Seasonal_Reduction);
  // setup seasonal multipliers

  if (Transmission::Seasonal_Reduction > 0.0) {
    int seasonal_peak_day_of_year; // e.g. Jan 1
    Params::get_param_from_string("seasonal_peak_day_of_year", &seasonal_peak_day_of_year);

    // setup seasonal multipliers
    Transmission::Seasonality_multiplier = new double [367];
    for(int day = 1; day <= 366; ++day) {
      int days_from_peak_transmissibility = abs(seasonal_peak_day_of_year - day);
      Transmission::Seasonality_multiplier[day] = (1.0 - Transmission::Seasonal_Reduction) +
	Transmission::Seasonal_Reduction * 0.5 * (1.0 + cos(days_from_peak_transmissibility * (2 * PI / 365.0)));
      if(Transmission::Seasonality_multiplier[day] < 0.0) {
	Transmission::Seasonality_multiplier[day] = 0.0;
      }
      // printf("Seasonality_multiplier[%d] = %e %d\n", day, Transmission::Seasonality_multiplier[day], days_from_peak_transmissibility);
    }
  }
}



