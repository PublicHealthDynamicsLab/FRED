/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Age_Map.h"
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

  Params::get_indexed_param(this->name, "states", &(this->number_of_states));
  this->state_name.reserve(this->number_of_states);
    
  // get state names
  for (int i = 0; i < this->number_of_states; i++) {
    sprintf(paramstr, "%s[%d].name", this->name, i);
    Params::get_param(paramstr, state_name);
    this->state_name.push_back(state_name);
  }

  // age map
  this->age_map = new Age_Map(this->name);
  sprintf(paramstr, "%s", this->name);
  this->age_map->read_from_input(paramstr);
  this->age_groups = this->age_map->get_groups();

  // create initial distribution for each age group
  state_initial_percent = new double* [this->age_groups];
  for (int group = 0; group < this->age_groups; group++) {
    state_initial_percent[group] = new double [this->number_of_states];
    double initial_total = 0.0;
    for (int i = 0; i < this->number_of_states; i++) {
      double init_pct;
      sprintf(paramstr, "%s[%d][%d].initial_percent", this->name, group, i);
      printf("read param %s\n", paramstr); fflush(stdout);
      Params::get_param(paramstr, &init_pct);
      this->state_initial_percent[group][i] = init_pct;
      if (i > 0) {
	initial_total += init_pct;
      }
    }
    // make sure state percentages add up
    this->state_initial_percent[group][0] = 100.0 - initial_total;
    assert(this->state_initial_percent[group][0] >= 0.0);
  }

  // get time period for transition probabilities
  Params::get_indexed_param(this->name, "period_in_transition_probabilities", &(this->period_in_transition_probabilities));

  // initialize transition matrices, one for each age group
  this->transition_matrix = new double** [this->age_groups];
  for (int group = 0; group < this->age_groups; group++) {
    this->transition_matrix[group] = new double* [this->number_of_states];
    for (int i = 0; i < this->number_of_states; i++) {
      this->transition_matrix[group][i] = new double [this->number_of_states];
    }

    // read optional parameters
    Params::disable_abort_on_failure();

    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	// default value if not in params file:
	double prob = 0.0;
	sprintf(paramstr, "%s_trans[%d][%d][%d]", this->name, group,i,j);
	Params::get_param(paramstr, &prob);
	this->transition_matrix[group][i][j] = prob;
      }
    }
    // restore requiring parameters
    Params::set_abort_on_failure();

    // guarantee probability distribution by making same-state transition the default
    for (int i = 0; i < this->number_of_states; i++) {
      double sum = 0;
      for (int j = 0; j < this->number_of_states; j++) {
	if (i != j) {
	  sum += this->transition_matrix[group][i][j];
	}
      }
      assert(sum <= 1.0);
      this->transition_matrix[group][i][i] = 1.0 - sum;
    }
  }
}


void Markov_Model::print() {
  for (int i = 0; i < this->number_of_states; i++) {
    printf("MARKOV MODEL %s[%d].name = %s\n",
	   this->name, i, this->state_name[i].c_str());
  }

  for (int g = 0; g < this->age_groups; g++) {
    for (int i = 0; i < this->number_of_states; i++) {
      printf("MARKOV MODEL %s[%d][%d].initial_percent = %f\n",
	     this->name, g, i, this->state_initial_percent[g][i]);
    }

    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	printf("MARKOV MODEL %s_trans[%d][%d][%d] = %f\n",
	       this->name,g,i,j,this->transition_matrix[g][i][j]);
      }
    }
  }
}


int Markov_Model::get_initial_state(double age) {
  int group = this->age_map->find_value(age);
  double r = 100.0 * Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    sum += this->state_initial_percent[group][i];
    if (r < sum) {
      return i;
    }
  }  
  assert(r < sum);
  return -1;
}


void Markov_Model::get_next_state_and_time(int day, double age, int old_state, int* new_state, int* transition_day) {
  *transition_day = -1;
  *new_state = old_state;
  int group = this->age_map->find_value(age);
  for (int j = 0; j < this->number_of_states; j++) {
    if (j == old_state) {
      continue;
    }
    double lambda = this->transition_matrix[group][old_state][j];
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



