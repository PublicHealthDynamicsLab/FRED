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
// File: Disease.cc
//
#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

#include "Disease.h"
#include "Global.h"
#include "Params.h"
#include "Population.h"
#include "Random.h"
#include "Age_Map.h"
#include "Epidemic.h"
#include "Timestep_Map.h"
#include "StrainTable.h"
#include "IntraHost.h"
#include "Evolution.h"
#include "EvolutionFactory.h"
#include "Transmission.h"
#include "Multistrain_Timestep_Map.h"
#include "Seasonality.h"
#include "Strain.h"
#include "Person.h"
#include "Household.h"
#include "Health.h"
#include "Place_List.h"

std::string * Disease::Disease_name = NULL;

Disease::Disease() {
  // note that the code that establishes the latent/asymptomatic/symptomatic
  // periods has been moved to the IntraHost class (or classes derived from it).
  this->id = -1;
  this->transmissibility = -1.0;
  this->immunity_loss_rate = -1.0;
  this->residual_immunity = NULL;
  this->infection_immunity_prob = NULL;
  this->at_risk = NULL;
  this->epidemic = NULL;
  this->population = NULL;
  this->strain_table = NULL;
  this->ihm = NULL;
  this->enable_case_fatality = 0;
  this->case_fatality_age_factor = NULL;
  this->case_fatality_prob_by_day = NULL;
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

}

Disease::~Disease() {
  delete this->epidemic;
  delete this->residual_immunity;
  delete this->infection_immunity_prob;
  delete this->at_risk;
  delete this->strain_table;
  delete this->ihm;
  if(this->evol != NULL) {
    delete this->evol;
  }
}

void Disease::get_parameters(int disease) {
  char paramstr[256];
  this->id = disease;

  if (disease == 0) {
    Disease::Disease_name = new string[Global::Diseases];
    Params::get_param_vector((char *)"disease_names", Disease::Disease_name);
    for (int i = 0; i < Global::Diseases; i++) {
      printf("disease %d = %s\n", i, Disease::Disease_name[i].c_str());
    }
  }
  
  // get name of disease
  strcpy(this->disease_name, Disease::Disease_name[this->id].c_str());

  fprintf(Global::Statusfp, "disease %d %s read_parameters entered\n",
	  this->id, this->disease_name);
  fflush(Global::Statusfp);

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
    this->transmissibility = this->R0_a*this->R0*this->R0 + this->R0_b*this->R0;
  }

  //case fatality parameters
  Params::get_indexed_param(this->disease_name, "enable_case_fatality",
      &(this->enable_case_fatality));
  if (this->enable_case_fatality) {
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

  // protective behavior efficacy parameters
  Params::get_param_from_string("enable_face_mask_usage", &(this->enable_face_mask_usage));
  if (this->enable_face_mask_usage) {
    Params::get_indexed_param(this->disease_name, "face_mask_transmission_efficacy",
			      &(this->face_mask_transmission_efficacy));
    Params::get_indexed_param(this->disease_name, "face_mask_susceptibility_efficacy",
			      &(this->face_mask_susceptibility_efficacy));
  }
  Params::get_param_from_string("enable_hand_washing", &(this->enable_hand_washing));
  if (this->enable_hand_washing) {
    Params::get_indexed_param(this->disease_name, "hand_washing_transmission_efficacy",
			      &(this->hand_washing_transmission_efficacy));
    Params::get_indexed_param(this->disease_name, "hand_washing_susceptibility_efficacy",
			      &(this->hand_washing_susceptibility_efficacy));
  }
  if (this->enable_face_mask_usage && this->enable_hand_washing) {
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

void Disease::setup(Population *pop) {
  char paramstr[256];
  fprintf(Global::Statusfp, "disease %d %s setup entered\n",
	  this->id, this->disease_name);
  fflush(Global::Statusfp);

  this->population = pop;

  // Read primary_cases file that indicates the number of external infections
  // that occur each day.
  // Note: infectees are chosen at random, and previously infected individuals
  // are not affected, so the number of new cases may be less than specified in
  // the file.
  sprintf(paramstr, "primary_cases");
  string param_name_str(paramstr);
  Multistrain_Timestep_Map * msts = new Multistrain_Timestep_Map(
      param_name_str);
  msts->read_map();
  this->epidemic = new Epidemic(this, msts);
  this->epidemic->setup();

  // Initialize StrainTable
  this->strain_table = new StrainTable;
  this->strain_table->setup(this);
  this->strain_table->reset();

  // Initialize IntraHost
  int ihmType;
  Params::get_indexed_param(this->disease_name, "intra_host_model", &ihmType);
  this->ihm = IntraHost::newIntraHost(ihmType);
  this->ihm->setup(this);

  // Initialize Infection Trajectory Thresholds
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

//void Disease::print() {
//  // Since most of Disease has been moved to IntraHost (or classes derived from it)
//  // the old print() function no longer worked.
//  // TODO: write new print() statement or remove
//}

int Disease::get_days_recovered() {
  int days;

  if(this->immunity_loss_rate > 0.0) {
    // draw from exponential distribution
    days = floor(0.5 + draw_exponential(this->immunity_loss_rate));
    // printf("DAYS RECOVERED = %d\n", days);
  } else {
    days = -1;
  }

  return days;
}

int Disease::get_days_symp() {
  return this->ihm->get_days_symp();
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

double Disease::get_transmissibility(int strain) {
  return this->strain_table->get_transmissibility(strain);
}

Trajectory * Disease::get_trajectory(Infection *infection, Transmission::Loads * loads) {
  return this->ihm->get_trajectory(infection, loads);
}

Transmission::Loads * Disease::get_primary_loads(int day) {
  return this->evol->get_primary_loads(day);
}
/// @brief Overloaded to allow specification of a single strain to be used for the initial loads.
Transmission::Loads * Disease::get_primary_loads(int day, int strain) {
  return this->evol->get_primary_loads(day, strain);
}

bool Disease::gen_immunity_infection(double real_age) {
  double prob = this->infection_immunity_prob->find_value(real_age);
  return (RANDOM() <= prob);
}

int Disease::get_num_strain_data_elements(int strain) {
  return this->strain_table->get_num_strain_data_elements(strain);
}

int Disease::get_strain_data_element(int strain, int i) {
  return this->strain_table->get_strain_data_element(strain, i);
}

void Disease::add_root_strain(int num_elements) {
  this->strain_table->add_root_strain(num_elements);
}

int Disease::add_strain(Strain * child_strain, double transmissibility,
    int parent_strain_id) {
  return this->strain_table->add(child_strain, transmissibility, parent_strain_id);
}

int Disease::add_strain(Strain * child_strain, double transmissibility) {
  return this->strain_table->add(child_strain, transmissibility);
}

double Disease::calculate_climate_multiplier(double seasonality_value) {
  return exp(((this->seasonality_Ka * seasonality_value) + this->seasonality_Kb))
      + this->seasonality_min;
}

int Disease::get_num_strains() {
  return this->strain_table->get_num_strains();
}

void Disease::print_strain(int strain_id, stringstream & out) {
  this->strain_table->print_strain(strain_id, out);
}

std::string Disease::get_strain_data_string(int strain_id) {
  return this->strain_table->get_strain_data_string(strain_id);
}

const Strain_Data & Disease::get_strain_data(int strain) {
  return this->strain_table->get_strain_data(strain);
}

const Strain & Disease::get_strain(int strain_id) {
  return this->strain_table->get_strain(strain_id);
}

void Disease::initialize_evolution_reporting_grid(Regional_Layer * grid) {
  this->evol->initialize_reporting_grid(grid);
}

void Disease::init_prior_immunity() {
  this->evol->init_prior_immunity(this);
}
bool Disease::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  if(this->enable_case_fatality && symptoms >= this->min_symptoms_for_case_fatality) {
      double age_prob = this->case_fatality_age_factor->find_value(real_age);
      double day_prob = this->case_fatality_prob_by_day[days_symptomatic];
      return (RANDOM() < age_prob * day_prob);
  }
  return false;
}

bool Disease::is_fatal(Person * per, double symptoms, int days_symptomatic) {
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
      if(per->get_demographics()->is_pregnant()) {
        age_prob *= Health::get_pregnancy_case_fatality_prob_mult(per->get_age());
      }
      return (RANDOM() < age_prob * day_prob);
    } else {
      return is_fatal(per->get_age(), symptoms, days_symptomatic);
    }
  } else {
    return is_fatal(per->get_age(), symptoms, days_symptomatic);
  }
}
