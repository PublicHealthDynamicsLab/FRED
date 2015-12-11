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

#include "Natural_History.h"

#include <vector>
using namespace std;

class Markov_Natural_History : public Natural_History {

public:
  Markov_Natural_History();

  ~Markov_Natural_History();

  void setup(Disease *disease);

  void get_parameters();

  void update_infection(int day, Person* host, Infection *infection);

  double get_probability_of_symptoms(int age);

  int get_latent_period(Person* host);

  int get_duration_of_infectiousness(Person* host);

  int get_duration_of_immunity(Person* host);

  int get_incubation_period(Person* host);

  int get_duration_of_symptoms(Person* host);

  bool is_fatal(double real_age, double symptoms, int days_symptomatic);

  bool is_fatal(Person* per, double symptoms, int days_symptomatic);

  void init_prior_immunity() {}

  bool gen_immunity_infection(double real_age) { return true; }

  void print();

  int get_number_of_states() {
    return this->number_of_states;
  }

  std::string get_state_name(int i) {
    return this->state_name[i];
  }

  double get_transition_probability(int i, int j) {
    return this->transition_matrix[i][j];
  }

private:
  int number_of_states;
  std::vector<std::string>state_name;
  double ** transition_matrix;

};

#endif
