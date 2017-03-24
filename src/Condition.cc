/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Condition.cc
//
#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

#include "Age_Map.h"
#include "Condition.h"
#include "Epidemic.h"
#include "Global.h"
#include "Health.h"
#include "Household.h"
#include "Natural_History.h"
#include "Params.h"
#include "Person.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "Seasonality.h"
#include "Timestep_Map.h"
#include "Transmission.h"

Condition::Condition() {
  this->id = -1;
  this->transmissibility = -1.0;
  this->epidemic = NULL;
  this->natural_history = NULL;
  this->min_symptoms_for_seek_healthcare = -1.0;
  this->hospitalization_prob = NULL;
  this->outpatient_healthcare_prob = NULL;
  this->seasonality_Ka = -1.0;
  this->seasonality_Kb = -1.0;
  this->seasonality_min = -1.0;
  this->seasonality_max = -1.0;
  strcpy(this->natural_history_model,"markov");
  strcpy(this->transmission_mode,"respiratory");
  this->is_hiv_infection = false;
}

Condition::~Condition() {

  delete this->natural_history;
  delete this->epidemic;

  if(this->hospitalization_prob != NULL) {
    delete this->hospitalization_prob;
  }

  if(this->outpatient_healthcare_prob != NULL) {
    delete this->outpatient_healthcare_prob;
  }
}

void Condition::get_parameters(int condition_id, string name) {
  char param_name[256];

  // set condition id
  this->id = condition_id;
  
  // set condition name
  strcpy(this->condition_name, name.c_str());

  FRED_VERBOSE(0, "condition %d %s read_parameters entered\n", this->id, this->condition_name);
  
  // optional parameters:
  Params::disable_abort_on_failure();

  // type of natural history and transmission mode
  Params::get_param(this->condition_name, "natural_history_model", this->natural_history_model);
  Params::get_param(this->condition_name, "transmission_mode", this->transmission_mode);

  Params::set_abort_on_failure();

  // variation over time of year
  if(Global::Enable_Climate) {
    Params::get_param(this->condition_name, "seasonality_multiplier_min",
			      &(this->seasonality_min));
    Params::get_param(this->condition_name, "seasonality_multiplier_max",
			      &(this->seasonality_max));
    Params::get_param(this->condition_name, "seasonality_multiplier_Ka",
			      &(this->seasonality_Ka)); // Ka = -180 by default
    this->seasonality_Kb = log(this->seasonality_max - this->seasonality_min);
  }

  //Hospitalization and Healthcare parameters
  if(Global::Enable_Hospitals) {
    Params::get_param(this->condition_name, "min_symptoms_for_seek_healthcare",
			      &(this->min_symptoms_for_seek_healthcare));
    this->hospitalization_prob = new Age_Map("Hospitalization Probability");
    sprintf(param_name, "%s_hospitalization_prob", this->condition_name);
    this->hospitalization_prob->read_from_input(param_name);
    this->outpatient_healthcare_prob = new Age_Map("Outpatient Healthcare Probability");
    sprintf(param_name, "%s_outpatient_healthcare_prob", this->condition_name);
    this->outpatient_healthcare_prob->read_from_input(param_name);
  }

  FRED_VERBOSE(0, "condition %d %s read_parameters finished\n", this->id, this->condition_name);
}

void Condition::setup() {

  FRED_VERBOSE(0, "condition %d %s setup entered\n", this->id, this->condition_name);

  // is this condition HIV?
  is_hiv_infection = (strcmp(this->condition_name, "HIV") == 0) || (strcmp(this->condition_name,"hiv")==0);

  // Initialize Natural History Model
  this->natural_history = Natural_History::get_new_natural_history(this->natural_history_model);

  // read in parameters and files associated with this natural history model: 
  this->natural_history->setup(this);
  this->natural_history->get_parameters();

  // contagiousness
  this->transmissibility = this->natural_history->get_transmissibility();
  
  if(this->transmissibility > 0.0) {
    // Initialize Transmission Model
    this->transmission = Transmission::get_new_transmission(this->transmission_mode);

    // read in parameters and files associated with this transmission mode: 
    this->transmission->setup(this);
  }

  // Initialize Epidemic Model
  this->epidemic = Epidemic::get_epidemic(this);
  this->epidemic->setup();

  FRED_VERBOSE(0, "condition %d %s setup finished\n", this->id, this->condition_name);
}

void Condition::prepare() {

  FRED_VERBOSE(0, "condition %d %s prepare entered\n", this->id, this->condition_name);

  // final prep for natural history model
  this->natural_history->prepare();

  // final prep for epidemic
  this->epidemic->prepare();

  FRED_VERBOSE(0, "condition %d %s prepare finished\n", this->id, this->condition_name);
}

double Condition::get_attack_rate() {
  return this->epidemic->get_attack_rate();
}

double Condition::get_symptomatic_attack_rate() {
  return this->epidemic->get_symptomatic_attack_rate();
}

double Condition::calculate_climate_multiplier(double seasonality_value) {
  return exp(((this->seasonality_Ka* seasonality_value) + this->seasonality_Kb))
    + this->seasonality_min;
}

double Condition::get_hospitalization_prob(Person* per) {
  return this->hospitalization_prob->find_value(per->get_real_age());
}

double Condition::get_outpatient_healthcare_prob(Person* per) {
  return this->outpatient_healthcare_prob->find_value(per->get_real_age());
}

void Condition::report(int day) {
  this->epidemic->report(day);
}


void Condition::increment_cohort_infectee_count(int day) {
  this->epidemic->increment_cohort_infectee_count(day);
}
 
void Condition::update(int day) {
  this->epidemic->update(day);
}

void Condition::terminate_person(Person* person, int day) {
  this->epidemic->terminate_person(person, day);
}

void Condition::become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic) {
  this->epidemic->become_immune(person, susceptible, infectious, symptomatic);
}

bool Condition::is_case_fatality_enabled() {
  return this->natural_history->is_case_fatality_enabled();
}

bool Condition::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  return this->natural_history->is_fatal(real_age, symptoms, days_symptomatic);
}

bool Condition::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  return this->natural_history->is_fatal(per, symptoms, days_symptomatic);
}

void Condition::end_of_run() {
  this->epidemic->end_of_run();
  this->natural_history->end_of_run();
}

int Condition::get_number_of_states() {
  if (strcmp(this->natural_history_model, "markov")==0) {
    return this->natural_history->get_number_of_states();
  }
  else {
    return 0;
  }
}

string Condition::get_state_name(int i) {
  if (i < get_number_of_states()) {
    return this->natural_history->get_state_name(i);
  }
  else {
    return "UNDEFINED";
  }
}

int Condition::get_state_from_name(string state_name) {
  for (int i = 0; i < get_number_of_states(); ++i) {
    if (get_state_name(i) == state_name) {
      return i;
    }
  }
  assert(0);
  return -1;
}

int Condition::get_total_symptomatic_cases() {
  return this->epidemic->get_total_symptomatic_cases();
}

int Condition::get_total_severe_symptomatic_cases() {
  return this->epidemic->get_total_severe_symptomatic_cases();
}


bool Condition::is_household_confinement_enabled(Person *person) {
  return this->natural_history->is_household_confinement_enabled(person);
}

bool Condition::is_confined_to_household(Person *person) {
  return this->natural_history->is_confined_to_household(person);
}

bool Condition::get_daily_household_confinement(Person *person) {
  return this->natural_history->get_daily_household_confinement(person);
}

/*
double Condition::get_transmission_modifier(int condition_id) {
  return this->natural_history->get_transmission_modifier(condition_id);
}

double Condition::get_susceptibility_modifier(int condition_id) {
  return this->natural_history->get_susceptibility_modifier(condition_id);
}
*/

