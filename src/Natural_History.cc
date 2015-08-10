/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "Natural_History.h"
#include "HIV_Natural_History.h"
#include "Utils.h"

/**
 * This static factory method is used to get an instance of a specific Natural_History Model.
 * Depending on the model parameter, it will create a specific Natural_History Model and return
 * a pointer to it.
 *
 * @param a string containing the requested Natural_History model type
 * @return a pointer to a specific Natural_History model
 */

Natural_History * Natural_History::get_natural_history(char* natural_history_model) {

  if (strcmp(natural_history_model, "basic") == 0) {
    return new Natural_History;
  }
    
  if (strcmp(natural_history_model, "HIV") == 0) {
    return new HIV_Natural_History;
  }

  FRED_WARNING("Unknown natural_history_model (%s)-- using basic Natural_History.\n", natural_history_model);
  return new Natural_History;
}


void Natural_History::setup(Disease *disease) {
  this->disease = disease;
  prob_symptomatic = -1.0;
  asymp_infectivity = -1.0;
  symp_infectivity = -1.0;
  max_days_latent = -1;
  max_days_asymp = -1;
  max_days_symp = -1;
  days_latent = NULL;
  days_asymp = NULL;
  days_symp = NULL;
}

int Natural_History::get_latent_period(Person* host) {
}

int Natural_History::get_duration_of_infectiousness(Person* host) {
}

int Natural_History::get_incubation_period(Person* host) {
}

int Natural_History::get_duration_of_symptoms(Person* host) {
}



