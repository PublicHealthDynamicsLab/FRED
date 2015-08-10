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

  virtual void setup(Disease *disease);

  // called from Infection
  virtual void update_infection(int day, Person* host, Infection *infection) {}

  int get_latent_period(Person* host);

  int get_duration_of_infectiousness(Person* host);

  int get_incubation_period(Person* host);

  int get_duration_of_symptoms(Person* host);

  double get_probability_of_symptoms(int age);

  double get_asymptomatic_infectivity() {
    return asymptomatic_infectivity;
  }

  double get_symptomatic_infectivity() {
    return symptomatic_infectivity;
  }

  int get_duration_of_immunity(Person* host);

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
  Age_Map *age_specific_prob_symptomatic;
};

#endif
