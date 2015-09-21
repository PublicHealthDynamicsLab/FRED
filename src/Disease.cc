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
// File: Disease.cc
//
#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

#include "Age_Map.h"
#include "Disease.h"
#include "Epidemic.h"
#include "Evolution.h"
#include "EvolutionFactory.h"
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

Disease::Disease() {
  this->id = -1;
  this->transmissibility = -1.0;
  this->residual_immunity = NULL;
  this->at_risk = NULL;
  this->epidemic = NULL;
  this->natural_history = NULL;
  this->min_symptoms_for_seek_healthcare = -1.0;
  this->hospitalization_prob = NULL;
  this->outpatient_healthcare_prob = NULL;
  this->seasonality_Ka = -1.0;
  this->seasonality_Kb = -1.0;
  this->seasonality_min = -1.0;
  this->seasonality_max = -1.0;
  this->R0 = -1.0;
  this->R0_a = -1.0;
  this->R0_b = -1.0;
  strcpy(this->natural_history_model,"");
  this->enable_face_mask_usage = 0;
  this->face_mask_transmission_efficacy = -1.0;
  this->face_mask_susceptibility_efficacy = -1.0;
  this->enable_hand_washing = 0;
  this->hand_washing_transmission_efficacy = -1.0;
  this->hand_washing_susceptibility_efficacy = -1.0;
  this->face_mask_plus_hand_washing_transmission_efficacy = -1.0;
  this->face_mask_plus_hand_washing_susceptibility_efficacy = -1.0;
}

Disease::~Disease() {
  delete this->epidemic;
  delete this->residual_immunity;
  delete this->at_risk;
  delete this->natural_history;

  if(this->hospitalization_prob != NULL) {
    delete this->hospitalization_prob;
  }

  if(this->outpatient_healthcare_prob != NULL) {
    delete this->outpatient_healthcare_prob;
  }
}

void Disease::get_parameters(int disease_id, string name) {
  char paramstr[256];

  // set disease id
  this->id = disease_id;
  
  // set disease name
  strcpy(this->disease_name, name.c_str());

  FRED_VERBOSE(0, "disease %d %s read_parameters entered\n", this->id, this->disease_name);

  // type of natural history and transmission mode
  Params::get_indexed_param(this->disease_name, "natural_history_model", this->natural_history_model);
  Params::get_indexed_param(this->disease_name, "transmission_mode", this->transmission_mode);
  
  // contagiousness
  // Note: the following tries first to find "trans" but falls back to "transmissibility":
  Params::disable_abort_on_failure();
  int found = Params::get_indexed_param(this->disease_name, "trans", &(this->transmissibility));
  Params::set_abort_on_failure();
  if (found == 0) {
    Params::get_indexed_param(this->disease_name, "transmissibility", &(this->transmissibility));
  }


  // convenience parameters (for single disease simulations only)
  if (this->id == 0) {
    Params::get_param_from_string("R0", &this->R0);
    Params::get_param_from_string("R0_a", &this->R0_a);
    Params::get_param_from_string("R0_b", &this->R0_b);
    if(this->R0 > 0) {
      this->transmissibility = this->R0_a * this->R0 * this->R0 + this->R0_b * this->R0;
    }
  }

  // variation over time of year
  if(Global::Enable_Climate) {
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_min",
			      &(this->seasonality_min));
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_max",
			      &(this->seasonality_max));
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_Ka",
			      &(this->seasonality_Ka)); // Ka = -180 by default
    this->seasonality_Kb = log(this->seasonality_max - this->seasonality_min);
  }

  //Hospitalization and Healthcare parameters
  if(Global::Enable_Hospitals) {
    Params::get_indexed_param(this->disease_name, "min_symptoms_for_seek_healthcare",
			      &(this->min_symptoms_for_seek_healthcare));
    this->hospitalization_prob = new Age_Map("Hospitalization Probability");
    sprintf(paramstr, "%s_hospitalization_prob", this->disease_name);
    this->hospitalization_prob->read_from_input(paramstr);
    this->outpatient_healthcare_prob = new Age_Map("Outpatient Healthcare Probability");
    sprintf(paramstr, "%s_outpatient_healthcare_prob", this->disease_name);
    this->outpatient_healthcare_prob->read_from_input(paramstr);
  }

  // protective behavior efficacy parameters
  Params::get_param_from_string("enable_face_mask_usage", &(this->enable_face_mask_usage));
  if(this->enable_face_mask_usage) {
    Params::get_indexed_param(this->disease_name, "face_mask_transmission_efficacy",
			      &(this->face_mask_transmission_efficacy));
    Params::get_indexed_param(this->disease_name, "face_mask_susceptibility_efficacy",
			      &(this->face_mask_susceptibility_efficacy));
  }
  Params::get_param_from_string("enable_hand_washing", &(this->enable_hand_washing));
  if(this->enable_hand_washing) {
    Params::get_indexed_param(this->disease_name, "hand_washing_transmission_efficacy",
			      &(this->hand_washing_transmission_efficacy));
    Params::get_indexed_param(this->disease_name, "hand_washing_susceptibility_efficacy",
			      &(this->hand_washing_susceptibility_efficacy));
  }
  if(this->enable_face_mask_usage && this->enable_hand_washing) {
    Params::get_indexed_param(this->disease_name, "face_mask_plus_hand_washing_transmission_efficacy",
			      &(this->face_mask_plus_hand_washing_transmission_efficacy));
    Params::get_indexed_param(this->disease_name, "face_mask_plus_hand_washing_susceptibility_efficacy",
			      &(this->face_mask_plus_hand_washing_susceptibility_efficacy));
  }

  // Define residual immunity
  this->residual_immunity = new Age_Map("Residual Immunity");
  sprintf(paramstr, "%s_residual_immunity", this->disease_name);
  this->residual_immunity->read_from_input(paramstr);

  // use params "Immunization" to override the residual immunity for
  // the group starting with age 0.
  // This allows easy specification for a prior imunization rate, e.g.,
  // to specify 25% prior immunization rate, use the parameters:
  //
  // residual_immunity_ages[0] = 2 0 100
  // residual_immunity_values[0] = 1 0.0
  // Immunization = 0.25
  //

  double immunization_rate;
  Params::get_param_from_string("Immunization", &immunization_rate);
  if(immunization_rate >= 0.0) {
    this->residual_immunity->set_all_values(immunization_rate);
  }

  if(this->residual_immunity->is_empty() == false) {
    this->residual_immunity->print();
  }
  
  if(Global::Residual_Immunity_by_FIPS) {
    this->read_residual_immunity_by_FIPS();
    fprintf(Global::Statusfp, "Residual Immunity by FIPS enabled \n");
  }

  // Define at risk people
  this->at_risk = new Age_Map("At Risk Population");
  sprintf(paramstr, "%s_at_risk", this->disease_name);
  this->at_risk->read_from_input(paramstr);

  FRED_VERBOSE(0, "disease %d %s read_parameters finished\n", this->id, this->disease_name);
}

void Disease::setup() {

  FRED_VERBOSE(0, "disease %d %s setup entered\n", this->id, this->disease_name);

  this->epidemic = new Epidemic(this);
  this->epidemic->setup();

  // Initialize Natural History Model
  this->natural_history = Natural_History::get_new_natural_history(this->natural_history_model);

  // read in parameters and files associated with this natural history model: 
  this->natural_history->setup(this);
  this->natural_history->get_parameters();

  // Initialize Transmission Model
  this->transmission = Transmission::get_new_transmission(this->transmission_mode);

  // read in parameters and files associated with this transmission mode: 
  this->transmission->setup(this);

  fprintf(Global::Statusfp, "disease %d %s setup finished\n", this->id, this->disease_name);
  fflush(Global::Statusfp);
}

double Disease::get_attack_rate() {
  return this->epidemic->get_attack_rate();
}

double Disease::get_symptomatic_attack_rate() {
  return this->epidemic->get_symptomatic_attack_rate();
}

double Disease::calculate_climate_multiplier(double seasonality_value) {
  return exp(((this->seasonality_Ka* seasonality_value) + this->seasonality_Kb))
    + this->seasonality_min;
}

double Disease::get_hospitalization_prob(Person* per) {
  return this->hospitalization_prob->find_value(per->get_real_age());
}

double Disease::get_outpatient_healthcare_prob(Person* per) {
  return this->outpatient_healthcare_prob->find_value(per->get_real_age());
}

void Disease::read_residual_immunity_by_FIPS() {  
  //Params::get_param_from_string("residual_immunity_by_FIPS_file", Global::Residual_Immunity_File);
  char fips_string[FRED_STRING_SIZE];
  int fips_int;
  //char ages_string[FRED_STRING_SIZE];
  char values_string[FRED_STRING_SIZE];
  FILE *fp = NULL;
  fp = Utils::fred_open_file(Global::Residual_Immunity_File);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Residual Immunity by FIPS enabled but residual_immunity_by_FIPS_file %s not found\n", Global::Residual_Immunity_File);
    exit(1);
  }
  while(fgets(fips_string, FRED_STRING_SIZE - 1, fp) != NULL) {  //fips 2 lines fips first residual immunity second
    if ( ! (istringstream(fips_string) >> fips_int) ) fips_int = 0;
    if (fgets(values_string, FRED_STRING_SIZE - 1, fp) == NULL) {  //values
      fprintf(Global::Statusfp, "Residual Immunity by FIPS file %s not properly formed\n", Global::Residual_Immunity_File);
      exit(1);
    }
    std::vector<double> temp_vector;
    Params::get_param_vector_from_string(values_string, temp_vector);
    this->residual_immunity_by_FIPS.insert(std::pair<int, vector<double> > (fips_int,temp_vector) );
  }
}

vector<double> Disease::get_residual_immunity_values_by_FIPS(int FIPS_int) {
  return residual_immunity_by_FIPS[FIPS_int];
}

void Disease::print_stats(int day) {
  this->epidemic->print_stats(day);
}


void Disease::increment_cohort_infectee_count(int day) {
  this->epidemic->increment_cohort_infectee_count(day);
}
 
void Disease::update(int day) {
  this->epidemic->update(day);
}

void Disease::become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic) {
  this->epidemic->become_immune(person, susceptible, infectious, symptomatic);
}

