/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <vector>
#include <map>

#include "Age_Map.h"
#include "Binary_Natural_History.h"
#include "Disease.h"
#include "Natural_History.h"
#include "Infection.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Trajectory.h"

Binary_Natural_History::Binary_Natural_History() {
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

Binary_Natural_History::~Binary_Natural_History() {
  delete [] days_latent;
  delete [] days_asymp;
  delete [] days_symp;
}

void Binary_Natural_History::setup(Disease *disease) {
  char disease_name[80];
  char paramstr[80];
  Natural_History::setup(disease);

  strcpy(disease_name, disease->get_disease_name());
  Params::get_indexed_param(disease_name,"symp",&prob_symptomatic);
  Params::get_indexed_param(disease_name,"symp_infectivity",&symp_infectivity);
  Params::get_indexed_param(disease_name,"asymp_infectivity",&asymp_infectivity);

  // age specific probablility of symptoms
  this->age_specific_prob_symptomatic = new Age_Map("Prob Symptomatic");
  sprintf(paramstr, "%s_prob_symp", disease_name);
  this->age_specific_prob_symptomatic->read_from_input(paramstr);

  int n;
  Params::get_indexed_param(disease_name,"days_latent",&n);
  days_latent = new double [n];
  max_days_latent = Params::get_indexed_param_vector(disease_name, "days_latent", days_latent) -1;

  Params::get_indexed_param(disease_name,"days_asymp",&n);
  days_asymp = new double [n];
  max_days_asymp = Params::get_indexed_param_vector(disease_name, "days_asymp", days_asymp) -1;

  Params::get_indexed_param(disease_name,"days_symp",&n);
  days_symp = new double [n];
  max_days_symp = Params::get_indexed_param_vector(disease_name, "days_symp", days_symp) -1;

  Params::get_indexed_param(disease_name,"infection_model",  &infection_model);
}

Trajectory* Binary_Natural_History::get_trajectory(int age) {
  int sequential = get_infection_model();
  int will_be_symptomatic = will_have_symptoms(age);
  int days_latent = get_days_latent();
  int days_incubating;
  int days_asymptomatic = 0;
  int days_symptomatic = 0;
  double symptomatic_infectivity = get_symp_infectivity();
  double asymptomatic_infectivity = get_asymp_infectivity();
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

int Binary_Natural_History::get_days_latent() {
  int days = 0;
  days = Random::draw_from_distribution(max_days_latent, days_latent);
  return days;
}

int Binary_Natural_History::get_days_asymp() {
  int days = 0;
  days = Random::draw_from_distribution(max_days_asymp, days_asymp);
  return days;
}

int Binary_Natural_History::get_days_symp() {
  int days = 0;
  days = Random::draw_from_distribution(max_days_symp, days_symp);
  return days;
}

int Binary_Natural_History::get_days_susceptible() {
  return 0;
}

int Binary_Natural_History::will_have_symptoms(int age) {
  double prob = get_prob_symptomatic(age);
  return (Random::draw_random() < prob);
}

