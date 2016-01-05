/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_NATURAL_HISTORY_H
#define _FRED_NATURAL_HISTORY_H

#include <string>
using namespace std;

class Age_Map;
class Disease;
class Evolution;
class Person;
class Regional_Layer;
class Infection;

#define NEVER (-1)

#define LOGNORMAL 0
#define OFFSET_FROM_START_OF_SYMPTOMS 1
#define OFFSET_FROM_SYMPTOMS 2
#define CDF 3

class Natural_History {
public:
  
  virtual ~Natural_History();

  /**
   * This static factory method is used to get an instance of a specific Natural_History Model.
   * Depending on the model parameter, it will create a specific Natural_History Model and return
   * a pointer to it.
   *
   * @param a string containing the requested Natural_History model type
   * @return a pointer to a specific Natural_History model
   */

  static Natural_History * get_new_natural_history(char* natural_history_model);

  /*
   * The Natural History base class implements an SEIR(S) model.
   * For other models, define a derived class and redefine the following
   * virtual methods as needed.
   */

  virtual void setup(Disease *disease);

  virtual void get_parameters();

  virtual void prepare() {}

  // called from Infection

  virtual void update_infection(int day, Person* host, Infection *infection) {}

  virtual bool do_symptoms_coincide_with_infectiousness() {
    return true;
  }

  virtual double get_probability_of_symptoms(int age);

  virtual int get_latent_period(Person* host);

  virtual int get_duration_of_infectiousness(Person* host);

  virtual int get_duration_of_immunity(Person* host);

  virtual double get_real_incubation_period(Person* host);

  virtual double get_symptoms_duration(Person* host);

  virtual double get_real_latent_period(Person* host);

  virtual double get_infectious_duration(Person* host);

  virtual double get_infectious_start_offset(Person* host);

  virtual double get_infectious_end_offset(Person* host);

  virtual int get_incubation_period(Person* host);

  virtual int get_duration_of_symptoms(Person* host);

  virtual double get_asymptomatic_infectivity() {
    return asymptomatic_infectivity;
  }

  virtual Evolution* get_evolution() {
    return this->evol;
  }

  virtual double get_infectivity_threshold() {
    return this->infectivity_threshold;
  }
  
  virtual double get_symptomaticity_threshold() {
    return this->symptomaticity_threshold;
  }

  virtual void init_prior_immunity();

  // case fatality

  virtual bool is_case_fatality_enabled() {
    return this->enable_case_fatality;
  }

  virtual bool is_fatal(double real_age, double symptoms, int days_symptomatic);

  virtual bool is_fatal(Person* per, double symptoms, int days_symptomatic);

  // immunity after infection

  virtual bool gen_immunity_infection(double real_age);

  // support for viral evolution

  virtual void initialize_evolution_reporting_grid(Regional_Layer* grid);

  virtual void end_of_run(){}

  /*
  virtual int get_use_incubation_offset() {
    return 0;
  }
  */

  virtual int get_number_of_states() {
    return 1;
  }

  virtual double get_transition_probability(int i, int j) {
    return 0.0;
  }

  virtual std::string get_state_name(int i) {
    return "";
  }

  virtual int get_initial_state() {
    return 0;
  }

  virtual double get_infectivity(int state) {
    return 0.0;
  }

  virtual double get_symptoms(int state) {
    return 0.0;
  }

  virtual bool is_fatal(int state) {
    return false;
  }

  int get_symptoms_distribution_type() {
    return symptoms_distribution_type;
  }

  int get_infectious_distribution_type() {
    return infectious_distribution_type;
  }

  double get_full_symptoms_start() {
    return full_symptoms_start;
  }

  double get_full_symptoms_end() {
    return full_symptoms_end;
  }

  double get_full_infectivity_start() {
    return full_infectivity_start;
  }

  double get_full_infectivity_end() {
    return full_infectivity_end;
  }


protected:
  Disease *disease;
  double probability_of_symptoms;
  double asymptomatic_infectivity;

  char symptoms_distributions[32];
  char infectious_distributions[32];
  int symptoms_distribution_type;
  int infectious_distribution_type;

  // CDFs
  int max_days_incubating;
  int max_days_symptomatic;
  double *days_incubating;
  double *days_symptomatic;

  int max_days_latent;
  int max_days_infectious;
  double *days_latent;
  double *days_infectious;

  Age_Map *age_specific_prob_symptoms;
  double immunity_loss_rate;

  // parameters for incubation and infectious periods and offsets
  double incubation_period_median;
  double incubation_period_dispersion;
  double incubation_period_upper_bound;

  double symptoms_duration_median;
  double symptoms_duration_dispersion;
  double symptoms_duration_upper_bound;

  double latent_period_median;
  double latent_period_dispersion;
  double latent_period_upper_bound;

  double infectious_duration_median;
  double infectious_duration_dispersion;
  double infectious_duration_upper_bound;

  double infectious_start_offset;
  double infectious_end_offset;

  // thresholds used in Infection class to determine if an agent is
  // infectious/symptomatic at a given time point
  double infectivity_threshold;
  double symptomaticity_threshold;

  // fraction of periods with full symptoms or infectivity
  double full_symptoms_start;
  double full_symptoms_end;
  double full_infectivity_start;
  double full_infectivity_end;
  
  // case fatality parameters
  int enable_case_fatality;
  double min_symptoms_for_case_fatality;
  Age_Map* age_specific_prob_case_fatality;
  double* case_fatality_prob_by_day;
  int max_days_case_fatality_prob;

  Age_Map* age_specific_prob_infection_immunity;
  Evolution* evol;

};

#endif
