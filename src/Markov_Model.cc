/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Markov_Model.h"
#include "Params.h"
#include "Random.h"


Markov_Model::Markov_Model() {
}

Markov_Model::~Markov_Model() {
}

void Markov_Model::get_parameters() {

  FRED_VERBOSE(0, "Markov_Model(%s)::get_parameters\n", this->name);

  char model_file[256];
  char paramstr[256];
  char state_name[256];

  Params::get_indexed_param(this->name,"model_file", model_file);
  Params::read_parameter_file(model_file);

  Params::get_indexed_param(this->name, "states", &(this->number_of_states));
  this->transition_matrix = new double * [this->number_of_states];
  this->state_name.reserve(this->number_of_states);
    
  double initial_total = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    sprintf(paramstr, "%s[%d].name", this->name, i);
    Params::get_param(paramstr, state_name);
    this->state_name.push_back(state_name);
    double init_pct;
    sprintf(paramstr, "%s[%d].initial_percent", this->name, i);
    printf("read param %s\n", paramstr); fflush(stdout);
    Params::get_param(paramstr, &init_pct);
    this->state_initial_percent.push_back(init_pct);
    if (i > 0) {
      initial_total += init_pct;
    }
  }
  // make sure state percentages add up
  this->state_initial_percent[0] = 100.0 - initial_total;
  assert(this->state_initial_percent[0] >= 0.0);

  // get time period for transition probabilities
  Params::get_indexed_param(this->name, "period_in_transition_probabilities", &(this->period_in_transition_probabilities));

  // initialize transition matrix
  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_matrix[i] = new double [number_of_states];
  }

  // read optional parameters
  Params::disable_abort_on_failure();

  for (int i = 0; i < this->number_of_states; i++) {
    for (int j = 0; j < this->number_of_states; j++) {
      // default value if not in params file:
      double prob = 0.0;
      sprintf(paramstr, "%s_trans[%d][%d]", this->name, i,j);
      Params::get_param(paramstr, &prob);
      this->transition_matrix[i][j] = prob;
    }
  }
  // restore requiring parameters
  Params::set_abort_on_failure();

  // guarantee probability distribution by making same-state transition the default
  for (int i = 0; i < this->number_of_states; i++) {
    double sum = 0;
    for (int j = 0; j < this->number_of_states; j++) {
      if (i != j) {
	sum += this->transition_matrix[i][j];
      }
    }
    assert(sum <= 1.0);
    this->transition_matrix[i][i] = 1.0 - sum;
  }
}


void Markov_Model::print() {
  for (int i = 0; i < this->number_of_states; i++) {
    printf("MARKOV MODEL %s[%d].name = %s\n",
	   this->name, i, this->state_name[i].c_str());
    printf("MARKOV MODEL %s[%d].initial_percent = %f\n",
	   this->name, i, this->state_initial_percent[i]);
  }
  for (int i = 0; i < this->number_of_states; i++) {
    for (int j = 0; j < this->number_of_states; j++) {
      printf("MARKOV MODEL %s_trans[%d][%d] = %f\n",
	     this->name,i,j,this->transition_matrix[i][j]);
    }
  }
}

int Markov_Model::get_initial_state() {
  double r = 100.0 * Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    sum += this->state_initial_percent[i];
    if (r < sum) {
      return i;
    }
  }  
  assert(r < sum);
  return -1;
}


void Markov_Model::get_next_state_and_time(int day, int old_state, int* new_state, int* transition_day) {
  *transition_day = -1;
  *new_state = old_state;
  for (int j = 0; j < this->number_of_states; j++) {
    if (j == old_state) {
      continue;
    }
    double lambda = this->transition_matrix[old_state][j];
    if (lambda == 0.0) {
      continue;
    }
    int t = day + 1 + round(Random::draw_exponential(lambda) * this->period_in_transition_probabilities);
    if (*transition_day < 0 || t < *transition_day) {
      *transition_day = t;
      *new_state = j;
    }
  }
}



