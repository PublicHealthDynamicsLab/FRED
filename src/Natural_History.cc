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
#include "Evolution.h"
#include "EvolutionFactory.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "Utils.h"


Natural_History::~Natural_History() {

  if (this->age_specific_prob_infection_immunity != NULL) {
    delete this->age_specific_prob_infection_immunity;
  }
  
  if(this->evol != NULL) {
    delete this->evol;
  }
  
  if(this->age_specific_prob_case_fatality != NULL) {
    delete this->age_specific_prob_case_fatality;
  }
  
  if(this->case_fatality_prob_by_day != NULL) {
    delete this->case_fatality_prob_by_day;
  }
  
};

/**
 * This static factory method is used to get an instance of a specific Natural_History Model.
 * Depending on the model parameter, it will create a specific Natural_History Model and return
 * a pointer to it.
 *
 * @param a string containing the requested Natural_History model type
 * @return a pointer to a specific Natural_History model
 */

Natural_History * Natural_History::get_new_natural_history(char* natural_history_model) {
  
  if (strcmp(natural_history_model, "basic") == 0) {
    return new Natural_History;
  }
  
  if (strcmp(natural_history_model, "hiv") == 0) {
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
  FRED_VERBOSE(0, "Natural_History::setup\n");
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
  this->age_specific_prob_symptoms = NULL;
  this->immunity_loss_rate = -1.0;

  this->symptomaticity_threshold = -1.0;
  this->infectivity_threshold = -1.0;

  this->enable_case_fatality = 0;
  this->age_specific_prob_case_fatality = NULL;
  this->case_fatality_prob_by_day = NULL;
  this->min_symptoms_for_case_fatality = -1.0;
  this->max_days_case_fatality_prob = -1;

  this->age_specific_prob_infection_immunity = NULL;
  this->evol = NULL;

  FRED_VERBOSE(0, "Natural_History::setup finished\n");
}

void Natural_History::get_parameters() {

  FRED_VERBOSE(0, "Natural_History::get_parameters\n");
  // read in the disease-specific parameters
  char paramstr[256];
  char disease_name[20];
  int n;

  strcpy(disease_name, disease->get_disease_name());

  Params::get_indexed_param(disease_name,"probability_of_symptoms",&(this->probability_of_symptoms));
  Params::get_indexed_param(disease_name,"symp_infectivity",&(this->symptomatic_infectivity));
  Params::get_indexed_param(disease_name,"asymp_infectivity",&(this->asymptomatic_infectivity));
  Params::get_indexed_param(disease_name, "immunity_loss_rate",&(this->immunity_loss_rate));
  
  // age specific probablility of symptoms
  this->age_specific_prob_symptoms = new Age_Map("Symptoms");
  sprintf(paramstr, "%s_prob_symptoms", disease_name);
  this->age_specific_prob_symptoms->read_from_input(paramstr);

  Params::get_indexed_param(disease_name,"symptoms_distributions", (this->symptoms_distributions));
  if (strcmp(this->symptoms_distributions, "lognormal")==0) {
    Params::get_indexed_param(disease_name, "incubation_period_median", &(this->incubation_period_median));
    Params::get_indexed_param(disease_name, "incubation_period_dispersion", &(this->incubation_period_dispersion));
    Params::get_indexed_param(disease_name, "symptoms_duration_median", &(this->symptoms_duration_median));
    Params::get_indexed_param(disease_name, "symptoms_duration_dispersion", &(this->symptoms_duration_dispersion));
    this->symptoms_distribution_type = LOGNORMAL;
  }
  else {
    Params::get_indexed_param(disease_name,"days_incubating",&n);
    this->days_incubating = new double [n];
    this->max_days_incubating = Params::get_indexed_param_vector(disease_name, "days_incubating", this->days_incubating) -1;
    
    Params::get_indexed_param(disease_name,"days_symptomatic",&n);
    this->days_symptomatic = new double [n];
    this->max_days_symptomatic = Params::get_indexed_param_vector(disease_name, "days_symptomatic", this->days_symptomatic) -1;
    this->symptoms_distribution_type = CDF;
  }

  Params::get_indexed_param(disease_name,"infectious_distributions", (this->infectious_distributions));
  if (strcmp(this->infectious_distributions, "offset_from_symptoms")==0) {
    Params::get_indexed_param(disease_name, "infectious_start_offset", &(this->infectious_start_offset));
    Params::get_indexed_param(disease_name, "infectious_end_offset", &(this->infectious_end_offset));
    this->infectious_distribution_type = OFFSET_FROM_SYMPTOMS;
  }
  else if (strcmp(this->infectious_distributions, "offset_from_start_of_symptoms")==0) {
    Params::get_indexed_param(disease_name, "infectious_start_offset", &(this->infectious_start_offset));
    Params::get_indexed_param(disease_name, "infectious_end_offset", &(this->infectious_end_offset));
    this->infectious_distribution_type = OFFSET_FROM_START_OF_SYMPTOMS;
  }
  else if (strcmp(this->infectious_distributions, "lognormal")==0) {
    Params::get_indexed_param(disease_name, "latent_period_median", &(this->latent_period_median));
    Params::get_indexed_param(disease_name, "latent_period_dispersion", &(this->latent_period_dispersion));
    Params::get_indexed_param(disease_name, "infectious_duration_median", &(this->infectious_duration_median));
    Params::get_indexed_param(disease_name, "infectious_duration_dispersion", &(this->infectious_duration_dispersion));
    this->infectious_distribution_type = LOGNORMAL;
  }
  else if (strcmp(this->infectious_distributions, "cdf")==0) {
    Params::get_indexed_param(disease_name,"days_latent",&n);
    this->days_latent = new double [n];
    this->max_days_latent = Params::get_indexed_param_vector(disease_name, "days_latent", this->days_latent) -1;
    
    Params::get_indexed_param(disease_name,"days_infectious",&n);
    this->days_infectious = new double [n];
    this->max_days_infectious = Params::get_indexed_param_vector(disease_name, "days_infectious", this->days_infectious) -1;
    this->infectious_distribution_type = CDF;
  }
  else {
    Utils::fred_abort("Natural_History: unrecognized infectious_distributions type: %s\n", this->infectious_distributions);
  }

  // Initialize Infection Thresholds
  Params::get_indexed_param(disease_name, "infectivity_threshold", &(this->infectivity_threshold));
  Params::get_indexed_param(disease_name, "symptomaticity_threshold", &(this->symptomaticity_threshold));

  //case fatality parameters
  Params::get_indexed_param(disease_name, "enable_case_fatality",
			    &(this->enable_case_fatality));
  if(this->enable_case_fatality) {
    Params::get_indexed_param(disease_name, "min_symptoms_for_case_fatality",
			      &(this->min_symptoms_for_case_fatality));
    this->age_specific_prob_case_fatality = new Age_Map("Case Fatality");
    sprintf(paramstr, "%s_case_fatality", disease_name);
    this->age_specific_prob_case_fatality->read_from_input(paramstr);
    Params::get_indexed_param(disease_name, "case_fatality_prob_by_day",
			      &(this->max_days_case_fatality_prob));
    this->case_fatality_prob_by_day =
      new double[this->max_days_case_fatality_prob];
    Params::get_indexed_param_vector(disease_name, "case_fatality_prob_by_day", 
				     this->case_fatality_prob_by_day);
  }

  // Probability of developing an immune response by past infections
  this->age_specific_prob_infection_immunity = new Age_Map("Infection Immunity");
  sprintf(paramstr, "%s_infection_immunity", disease_name);
  this->age_specific_prob_infection_immunity->read_from_input(paramstr);

  if (Global::Enable_Viral_Evolution) {
    int evolType;
    Params::get_indexed_param(disease_name, "evolution", &evolType);
    this->evol = EvolutionFactory::newEvolution(evolType);
    this->evol->setup(this->disease);
  }

  FRED_VERBOSE(0, "Natural_History::get_parameters finished\n");
}


double Natural_History::get_probability_of_symptoms(int age) {
  if (this->age_specific_prob_symptoms->is_empty()) {
    return this->probability_of_symptoms;
  }
  else {
    return this->age_specific_prob_symptoms->find_value(age);
  }
}

int Natural_History::get_latent_period(Person* host) {
  return Random::draw_from_distribution(max_days_latent, days_latent);
}

int Natural_History::get_incubation_period(Person* host) {
  return Random::draw_from_distribution(max_days_incubating, days_incubating);
}

int Natural_History::get_duration_of_infectiousness(Person* host) {
  return Random::draw_from_distribution(max_days_infectious, days_infectious);
}

int Natural_History::get_duration_of_symptoms(Person* host) {
  return Random::draw_from_distribution(max_days_symptomatic, days_symptomatic);
}

int Natural_History::get_duration_of_immunity(Person* host) {
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


bool Natural_History::gen_immunity_infection(double real_age) {
  double prob = this->age_specific_prob_infection_immunity->find_value(real_age);
  return (Random::draw_random() <= prob);
}


void Natural_History::initialize_evolution_reporting_grid(Regional_Layer* grid) {
  this->evol->initialize_reporting_grid(grid);
}

void Natural_History::init_prior_immunity() {
  this->evol->init_prior_immunity(this->disease);
}

bool Natural_History::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  if(this->enable_case_fatality && symptoms >= this->min_symptoms_for_case_fatality) {
    double age_prob = this->age_specific_prob_case_fatality->find_value(real_age);
    double day_prob = this->case_fatality_prob_by_day[days_symptomatic];
    return (Random::draw_random() < age_prob * day_prob);
  }
  return false;
}

bool Natural_History::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  if(Global::Enable_Chronic_Condition && this->enable_case_fatality) {
    if(per->has_chronic_condition()) {
      double age_prob = this->age_specific_prob_case_fatality->find_value(per->get_real_age());
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

double Natural_History::get_infectious_start_offset(Person* host) {
  return this->infectious_start_offset;
}

double Natural_History::get_infectious_end_offset(Person* host) {
  return this->infectious_end_offset;
}

double Natural_History::get_real_incubation_period(Person* host) {
  double location = log(this->incubation_period_median);
  double scale = 0.5*log(this->incubation_period_dispersion);
  double incubation_period = Random::draw_lognormal(location, scale);
  return incubation_period;
}

double Natural_History::get_symptoms_duration(Person* host) {
  double location = log(this->symptoms_duration_median);
  double scale = log(this->symptoms_duration_dispersion);
  double symptoms_duration = Random::draw_lognormal(location, scale);
  return symptoms_duration;
}

double Natural_History::get_real_latent_period(Person* host) {
  double location = log(this->latent_period_median);
  double scale = 0.5*log(this->latent_period_dispersion);
  double latent_period = Random::draw_lognormal(location, scale);
  return latent_period;
}

double Natural_History::get_infectious_duration(Person* host) {
  double location = log(this->infectious_duration_median);
  double scale = log(this->infectious_duration_dispersion);
  double infectious_duration = Random::draw_lognormal(location, scale);
  return infectious_duration;
}

