/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_MARKOV_NATURAL_HISTORY_H
#define _FRED_MARKOV_NATURAL_HISTORY_H

#include <string>
#include <vector>
using namespace std;

#include "Natural_History.h"
class Markov_Model;


class Markov_Natural_History : public Natural_History {

public:

  Markov_Natural_History();

  ~Markov_Natural_History();

  void setup(Disease *disease);

  void get_parameters();

  void print();

  double get_infectivity(int s) {
    return state_infectivity[s];
  }

  double get_symptoms(int s) {
    return state_symptoms[s];
  }

  bool is_fatal(int s) {
    return (state_fatality[s] == 1);
  }

  Markov_Model* get_markov_model() {
    return markov_model;
  }

  char* get_name();

  int get_number_of_states();

  std::string get_state_name(int i);

  // the following are unused in this model:
  double get_probability_of_symptoms(int age) {
    return 0.0;
  }
  int get_latent_period(Person* host) {
    return -1;
  }
  int get_duration_of_infectiousness(Person* host) {
    return -1;
  }
  int get_duration_of_immunity(Person* host) {
    return -1;
  }
  int get_incubation_period(Person* host) {
    return -1;
  }
  int get_duration_of_symptoms(Person* host) {
    return -1;
  }
  bool is_fatal(double real_age, double symptoms, int days_symptomatic) {
    return false;
  }
  bool is_fatal(Person* per, double symptoms, int days_symptomatic) {
    return false;
  }
  void init_prior_immunity() {}
  bool gen_immunity_infection(double real_age) {
    return true;
  }

private:
  Markov_Model* markov_model;
  std::vector<double>state_infectivity;
  std::vector<double>state_symptoms;
  std::vector<int>state_fatality;
  std::vector<int>state_visualization;
};

#endif
