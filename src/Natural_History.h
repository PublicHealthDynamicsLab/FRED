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
class Person;
class Infection;
class Trajectory;

class Natural_History {
public:
  
  virtual ~Natural_History() {};

  /**
   * This static factory method is used to get an instance of a specific Natural_History Model.
   * Depending on the model parameter, it will create a specific Natural_History Model and return
   * a pointer to it.
   *
   * @param a string containing the requested Natural_History model type
   * @return a pointer to a specific Natural_History model
   */
  static Natural_History * get_new_natural_history(char* natural_history_model);

  /**
   * Set the attributes for the Natural_History
   *
   * @param dis the disease to which this Natural_History model is associated
   */
  virtual void setup(Disease *dis);
  virtual void get_parameters(Disease * disease);

  // called from Infection
  virtual void update_infection(int day, Person* host, Infection *infection) {}
  int get_latent_period(Person* host);
  int get_duration_of_infectiousness(Person* host);
  int get_incubation_period(Person* host);
  int get_duration_of_symptoms(Person* host);

  Trajectory * get_trajectory(int age);
  int get_days_latent();
  int get_days_asymp();
  int get_days_symp();
  int get_days_susceptible();
  int will_have_symptoms(int age);
  double get_prob_symptomatic(int age);

  double get_asymptomatic_infectivity() {
    return asymptomatic_infectivity;
  }

  double get_symptomatic_infectivity() {
    return symptomatic_infectivity;
  }

  double get_prob_symptomatic() {
    return prob_symptomatic;
  }

protected:
  Disease *disease;
  double asymptomatic_infectivity;
  double symptomatic_infectivity;
  int max_days_latent;
  int max_days_asymp;
  int max_days_symp;
  double *days_latent;
  double *days_asymp;
  double *days_symp;
  double prob_symptomatic;
  Age_Map *age_specific_prob_symptomatic;
};

#endif
