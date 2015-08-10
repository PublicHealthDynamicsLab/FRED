/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "Natural_History.h"

#include "Age_Map.h"
#include "HIV_Natural_History.h"
#include "Disease.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "Trajectory.h"
#include "Utils.h"


/**
 * This static factory method is used to get an instance of a specific Natural_History Model.
 * Depending on the model parameter, it will create a specific Natural_History Model and return
 * a pointer to it.
 *
 * @param a string containing the requested Natural_History model type
 * @return a pointer to a specific Natural_History model
 */

static Natural_History * Natural_History::get_new_natural_history(char* natural_history_model) {
  
  if (strcmp(natural_history_model, "basic") == 0) {
    return new Natural_History;
  }
  
  if (strcmp(natural_history_model, "HIV") == 0) {
    return new HIV_Natural_History;
  }
  
  FRED_WARNING("Unknown natural_history_model (%s)-- using basic Natural_History.\n", natural_history_model);
  return new Natural_History;
}

/*
 * The Natural History base class implements an SEIR(S) model.
 * For other models, define a derived class.
 */

void Natural_History::setup(Disease * _disease) {
  this->disease = _disease;
  this->probability_of_symptoms = 0;
  this->symptomatic_infectivity = 0;
  this->asymptomatic_infectivity = 0;
  this->max_days_latent = 0;
  this->max_days_infectious = 0;
  this->max_days_incubating = 0;
  this->max_days_symptomatic = 0;
  this->days_latent = NULL;
  this->days_infectious = NULL;
  this->days_incubating = NULL;
  this->days_symptomatic = NULL;
  this->age_specific_prob_symptomatic = NULL;

  // read in the disease-specific parameters
  char paramstr[256];
  char disease_name[20];

  strcpy(disease_name, disease->get_disease_name());

  Params::get_indexed_param(disease_name,"probability_of_symptoms",&probability_of_symptoms);
  Params::get_indexed_param(disease_name,"symp_infectivity",&symptomatic_infectivity);
  Params::get_indexed_param(disease_name,"asymp_infectivity",&asymptomatic_infectivity);
  
  // age specific probablility of symptoms
  this->age_specific_prob_symptomatic = new Age_Map("Prob Symptomatic");
  sprintf(paramstr, "%s_prob_symp", disease_name);
  this->age_specific_prob_symptomatic->read_from_input(paramstr);

  int n;
  Params::get_indexed_param(disease_name,"days_latent",&n);
  days_latent = new double [n];
  max_days_latent = Params::get_indexed_param_vector(disease_name, "days_latent", days_latent) -1;

  Params::get_indexed_param(disease_name,"days_infectious",&n);
  days_infectious = new double [n];
  max_days_infectious = Params::get_indexed_param_vector(disease_name, "days_infectious", days_infectious) -1;

  Params::get_indexed_param(disease_name,"days_incubating",&n);
  days_incubating = new double [n];
  max_days_incubating = Params::get_indexed_param_vector(disease_name, "days_incubating", days_incubating) -1;

  Params::get_indexed_param(disease_name,"days_symptomatc",&n);
  days_symptomatic = new double [n];
  max_days_symptomatic = Params::get_indexed_param_vector(disease_name, "days_symptimatic", days_symptomatic) -1;

}

int Natural_History::get_latent_period(Person* host) {
  int days = Random::draw_from_distribution(max_days_latent, days_latent);
  return days;
}

int Natural_History::get_duration_of_infectiousness(Person* host) {
  int days = Random::draw_from_distribution(max_days_infectious, days_infectious);
  return days;
}

Trajectory* Natural_History::get_trajectory(int age) {
  int sequential = 0; // get_infection_model();
  int will_be_symptomatic = will_have_symptoms(age);
  int days_latent = get_days_latent();
  int days_incubating;
  int days_asymptomatic = 0;
  int days_symptomatic = 0;
  double symptomaticity = 1.0;

  if (sequential) { // SEiIR model
    days_asymptomatic = get_days_asymp();

    if (will_be_symptomatic) {
      days_symptomatic = get_days_symp();
    }
  } else { // SEIR/SEiR model
    if (will_be_symptomatic) {
      days_symptomatic = get_days_symp();
    } else {
      days_asymptomatic = get_days_asymp();
    }
  }

  days_incubating = days_latent + days_asymptomatic;

  Loads* loads;
  loads = new Loads;
  loads->clear();
  loads->insert( pair<int, double> (1, 1.0) );

  Trajectory * trajectory = new Trajectory();

  map<int, double> :: iterator it;
  for(it = loads->begin(); it != loads->end(); it++) {
    vector<double> infectivity_trajectory(days_latent, 0.0);
    infectivity_trajectory.insert(infectivity_trajectory.end(), days_asymptomatic, asymptomatic_infectivity);
    infectivity_trajectory.insert(infectivity_trajectory.end(), days_symptomatic, symptomatic_infectivity);
    trajectory->set_infectivity_trajectory(it->first, infectivity_trajectory);
  }

  vector<double> symptomaticity_trajectory(days_incubating, 0.0);
  symptomaticity_trajectory.insert(symptomaticity_trajectory.end(), days_symptomatic, symptomaticity);
  trajectory->set_symptomaticity_trajectory(symptomaticity_trajectory);

  return trajectory;
}

double Natural_History::get_probability_of_symptoma(int age) {
  if (this->age_specific_prob_symptomatic->is_empty()) {
    return this->probability_of_symptoms;
  }
  else {
    return this->age_specific_prob_symptomatic->find_value(age);
  }
}

