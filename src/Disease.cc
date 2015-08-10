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

Disease::Disease() {
  this->id = -1;
  this->transmissibility = -1.0;
  this->immunity_loss_rate = -1.0;
  this->residual_immunity = NULL;
  this->infection_immunity_prob = NULL;
  this->at_risk = NULL;
  this->epidemic = NULL;
  this->natural_history = NULL;
  this->enable_case_fatality = 0;
  this->case_fatality_age_factor = NULL;
  this->case_fatality_prob_by_day = NULL;
  this->min_symptoms_for_seek_healthcare = -1.0;
  this->hospitalization_prob = NULL;
  this->outpatient_healthcare_prob = NULL;
  this->seasonality_Ka = -1.0;
  this->seasonality_Kb = -1.0;
  this->seasonality_min = -1.0;
  this->seasonality_max = -1.0;
  this->symptomaticity_threshold = -1.0;
  this->evol = NULL;
  this->infectivity_threshold = -1.0;
  this->max_days_case_fatality_prob = -1;
  this->min_symptoms_for_case_fatality = -1.0;
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
  delete this->infection_immunity_prob;
  delete this->at_risk;
  delete this->natural_history;

  if(this->evol != NULL) {
    delete this->evol;
  }

  if(this->case_fatality_age_factor != NULL) {
    delete this->case_fatality_age_factor;
  }

  if(this->case_fatality_prob_by_day != NULL) {
    delete this->case_fatality_prob_by_day;
  }

  if(this->hospitalization_prob != NULL) {
    delete this->hospitalization_prob;
  }

  if(this->outpatient_healthcare_prob != NULL) {
    delete this->outpatient_healthcare_prob;
  }
}

void Disease::get_parameters(int disease, string name) {
  char paramstr[256];

  // set disease id
  this->id = disease;
  
  // set disease name
  strcpy(this->disease_name, name.c_str());

  fprintf(Global::Statusfp, "disease %d %s read_parameters entered\n",
	  this->id, this->disease_name);
  fflush(Global::Statusfp);

  // moved from natural_history
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

  // end: moved from natural_history

  Params::get_param_from_string("R0", &this->R0);
  Params::get_param_from_string("R0_a", &this->R0_a);
  Params::get_param_from_string("R0_b", &this->R0_b);

  Params::get_indexed_param(this->disease_name, "trans", &(this->transmissibility));
  Params::get_indexed_param(this->disease_name, "immunity_loss_rate",
			    &(this->immunity_loss_rate));

  if(Global::Enable_Climate) {
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_min",
			      &(this->seasonality_min));
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_max",
			      &(this->seasonality_max));
    Params::get_indexed_param(this->disease_name, "seasonality_multiplier_Ka",
			      &(this->seasonality_Ka)); // Ka = -180 by default
    this->seasonality_Kb = log(this->seasonality_max - this->seasonality_min);
  }

  if(this->R0 > 0) {
    this->transmissibility = this->R0_a * this->R0 * this->R0 + this->R0_b * this->R0;
  }

  //case fatality parameters
  Params::get_indexed_param(this->disease_name, "enable_case_fatality",
			    &(this->enable_case_fatality));
  if(this->enable_case_fatality) {
    Params::get_indexed_param(this->disease_name, "min_symptoms_for_case_fatality",
			      &(this->min_symptoms_for_case_fatality));
    this->case_fatality_age_factor = new Age_Map("Case Fatality Age Factor");
    sprintf(paramstr, "%s_case_fatality_age_factor", this->disease_name);
    this->case_fatality_age_factor->read_from_input(paramstr);
    Params::get_indexed_param(this->disease_name, "case_fatality_prob_by_day",
			      &(this->max_days_case_fatality_prob));
    this->case_fatality_prob_by_day =
      new double[this->max_days_case_fatality_prob];
    Params::get_indexed_param_vector(this->disease_name, "case_fatality_prob_by_day", 
				     this->case_fatality_prob_by_day);
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

  // Probability of developing an immune response by past infections
  this->infection_immunity_prob = new Age_Map("Infection Immunity Probability");
  sprintf(paramstr, "%s_infection_immunity_prob", this->disease_name);
  this->infection_immunity_prob->read_from_input(paramstr);

  // Define at risk people
  this->at_risk = new Age_Map("At Risk Population");
  sprintf(paramstr, "%s_at_risk", this->disease_name);
  this->at_risk->read_from_input(paramstr);

  fprintf(Global::Statusfp, "disease %d %s read_parameters finished\n", this->id, this->disease_name);
  fflush(Global::Statusfp);
}

void Disease::setup() {
  fprintf(Global::Statusfp, "disease %d %s setup entered\n",
	  this->id, this->disease_name);
  fflush(Global::Statusfp);

  this->epidemic = new Epidemic(this);
  this->epidemic->setup();

  // Initialize Natural History Model
  Params::get_indexed_param(this->disease_name, "natural_history_model", natural_history_model);
  this->natural_history = Natural_History::get_natural_history(natural_history_model);
  this->natural_history->setup(this);

  // Initialize Infection Thresholds
  Params::get_indexed_param(this->disease_name, "infectivity_threshold",
			    &(this->infectivity_threshold));
  Params::get_indexed_param(this->disease_name, "symptomaticity_threshold",
			    &(this->symptomaticity_threshold));

  int evolType;
  Params::get_indexed_param(this->disease_name, "evolution", &evolType);
  this->evol = EvolutionFactory::newEvolution(evolType);
  this->evol->setup(this);

  fprintf(Global::Statusfp, "disease %d %s setup finished\n", this->id, this->disease_name);
  fflush(Global::Statusfp);
}

  // called by Infection:

int Disease::get_latent_period(Person* host) {
  return this->natural_history->get_latent_period(host);
}

int Disease::get_duration_of_infectiousness(Person* host) {
  return this->natural_history->get_duration_of_infectiousness(host);
}

int Disease::get_incubation_period(Person* host) {
  return this->natural_history->get_incubation_period(host);
}

int Disease::get_duration_of_symptoms(Person* host) {
  return this->natural_history->get_duration_of_symptoms(host);
}

int Disease::get_duration_of_immunity(Person* host) {
  int days;

  if(this->immunity_loss_rate > 0.0) {
    // draw from exponential distribution
    days = floor(0.5 + Random::draw_exponential(this->immunity_loss_rate));
    // printf("DAYS RECOVERED = %d\n", days);
  } else {
    days = -1;
  }

  return days;
}

double Disease::get_attack_rate() {
  return this->epidemic->get_attack_rate();
}

double Disease::get_symptomatic_attack_rate() {
  return this->epidemic->get_symptomatic_attack_rate();
}

// static
void Disease::get_disease_parameters() {
}

bool Disease::gen_immunity_infection(double real_age) {
  double prob = this->infection_immunity_prob->find_value(real_age);
  return (Random::draw_random() <= prob);
}

double Disease::calculate_climate_multiplier(double seasonality_value) {
  return exp(((this->seasonality_Ka* seasonality_value) + this->seasonality_Kb))
    + this->seasonality_min;
}

void Disease::initialize_evolution_reporting_grid(Regional_Layer* grid) {
  // this->evol->initialize_reporting_grid(grid);
}

void Disease::init_prior_immunity() {
  this->evol->init_prior_immunity(this);
}

bool Disease::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  if(this->enable_case_fatality && symptoms >= this->min_symptoms_for_case_fatality) {
    double age_prob = this->case_fatality_age_factor->find_value(real_age);
    double day_prob = this->case_fatality_prob_by_day[days_symptomatic];
    return (Random::draw_random() < age_prob * day_prob);
  }
  return false;
}

bool Disease::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  if(Global::Enable_Chronic_Condition && this->enable_case_fatality) {
    if(per->has_chronic_condition()) {
      double age_prob = this->case_fatality_age_factor->find_value(per->get_real_age());
      double day_prob = this->case_fatality_prob_by_day[days_symptomatic];
      if(per->is_asthmatic()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::ASTHMA);
      }
      if(per->has_COPD()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::COPD);
      }
      if(per->has_chronic_renal_disease()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::CHRONIC_RENAL_DISEASE);
      }
      if(per->is_diabetic()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::DIABETES);
      }
      if(per->has_heart_disease()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::HEART_DISEASE);
      }
      if(per->has_hypertension()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::HYPERTENSION);
      }
      if(per->has_hypercholestrolemia()) {
        age_prob *= Health::get_chronic_condition_case_fatality_prob_mult(per->get_age(), Chronic_condition_index::HYPERCHOLESTROLEMIA);
      }
      if(per->get_demographics()->is_pregnant()) {
        age_prob *= Health::get_pregnancy_case_fatality_prob_mult(per->get_age());
      }
      return (Random::draw_random() < age_prob * day_prob);
    } else {
      return is_fatal(per->get_age(), symptoms, days_symptomatic);
    }
  } else {
    return is_fatal(per->get_age(), symptoms, days_symptomatic);
  }
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
