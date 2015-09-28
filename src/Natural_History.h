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

class Age_Map;
class Disease;
class Evolution;
class Person;
class Regional_Layer;
class Infection;

#define NEVER (-1)

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

  // called from Infection

  virtual void update_infection(int day, Person* host, Infection *infection) {}

  virtual bool do_symptoms_coincide_with_infectiousness() { return true; }

  virtual double get_probability_of_symptoms(int age);

  virtual int get_latent_period(Person* host);

  virtual int get_duration_of_infectiousness(Person* host);

  virtual int get_duration_of_immunity(Person* host);

  // not used in default model: symptoms (if any) coincide with infectiousness
  virtual int get_incubation_period(Person* host) {
    return NEVER;
  }

  // not used in default model: symptoms (if any) coincide with infectiousness
  virtual int get_duration_of_symptoms(Person* host) {
    return NEVER;
  }

  // called from Transmission

  virtual double get_asymptomatic_infectivity() {
    return asymptomatic_infectivity;
  }

  virtual double get_symptomatic_infectivity() {
    return symptomatic_infectivity;
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

protected:
  Disease *disease;
  double probability_of_symptoms;
  double symptomatic_infectivity;
  double asymptomatic_infectivity;
  int max_days_latent;
  int max_days_infectious;
  int max_days_incubating;
  int max_days_symptomatic;
  double *days_latent;
  double *days_infectious;
  double *days_incubating;
  double *days_symptomatic;
  Age_Map *age_specific_prob_symptoms;
  double immunity_loss_rate;

  // thresholds used in Infection class to determine if an agent is
  // infectious/symptomatic at a given time point
  double infectivity_threshold;
  double symptomaticity_threshold;

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
