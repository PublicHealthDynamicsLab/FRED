/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Age_Map.h"
#include "Markov_Chain.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"

Markov_Chain::Markov_Chain() {
}

Markov_Chain::~Markov_Chain() {
}

void Markov_Chain::get_parameters() {

  FRED_VERBOSE(0, "Markov_Chain(%s)::get_parameters\n", this->name);

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
      sprintf(paramstr, "%s.group[%d].initial_percent[%d]", this->name, group, i);
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

  // initialize transition matrices, one for each age group
  this->transition_matrix = new double** [this->age_groups];
  for (int group = 0; group < this->age_groups; group++) {
    this->transition_matrix[group] = new double* [this->number_of_states];
    for (int i = 0; i < this->number_of_states; i++) {
      this->transition_matrix[group][i] = new double [this->number_of_states];
    }

    // identity matrix
    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	this->transition_matrix[group][i][j] = 0.0;
      }
      this->transition_matrix[group][i][i] = 1.0;
    }

    // read optional parameters
    Params::disable_abort_on_failure();

    // get time period for transition probabilities (default: daily)
    this->transition_time_period = 1;
    Params::get_indexed_param(this->name, "transition_time_period", &(this->transition_time_period));

    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	// default value if not in params file:
	double prob = 0.0;
	sprintf(paramstr, "%s.group[%d].trans[%d][%d]", this->name, group,i,j);
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
  print();
}


void Markov_Chain::print() {
  for (int i = 0; i < this->number_of_states; i++) {
    printf("MARKOV MODEL %s[%d].name = %s\n",
	   this->name, i, this->state_name[i].c_str());
  }

  this->age_map->print();

  for (int g = 0; g < this->age_groups; g++) {
    for (int i = 0; i < this->number_of_states; i++) {
      printf("MARKOV MODEL %s.group[%d].initial_percent[%d] = %f\n",
	     this->name, g, i, this->state_initial_percent[g][i]);
    }

    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	printf("MARKOV MODEL %s.group[%d].trans[%d][%d] = %f\n",
	       this->name,g,i,j,this->transition_matrix[g][i][j]);
      }
    }
  }
}


int Markov_Chain::get_initial_state(double age, int adjustment_state, double adjustment) {
  int group = this->age_map->find_value(age);
  double total = 100.0 - (1.0-adjustment)*this->state_initial_percent[group][adjustment_state];
  double r = total * Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    double next_pct = this->state_initial_percent[group][i];
    if (i==adjustment_state) {
      next_pct *= adjustment;
    }
    sum += next_pct;
    if (r < sum) {
      return i;
    }
  }  
  assert(r < sum);
  return -1;
}


void Markov_Chain::get_next_state(int day, double age, int state, int* next_state, int* transition_day, int adjustment_state, double adjustment) {
  *transition_day = -1;
  *next_state = state;
  int group = this->age_map->find_value(age);

  double stay = this->transition_matrix[group][state][state];
  if (adjustment_state == state) {
    stay *= adjustment;
    if (stay > 1.0) {
      stay = 1.0;
    }
  }

  if (stay == 1.0) {
    // current state is absorbing
    return;
  }

  // lambda is the rate at which we leave current state
  double lambda = -log(stay);

  // find time of next transition
  *transition_day = day + round(Random::draw_exponential(lambda) * this->transition_time_period);
  // transition must be in the future
  if (*transition_day == day) {
    *transition_day = day+1;
  }

  // find the next state

  // form a prob dist over outgoing states
  double p[this->number_of_states];
  double total = 0.0;
  for (int j = 0; j < this->number_of_states; j++) {
    if (j == state) {
      p[j] = 0.0;
    }
    else {
      p[j] = this->transition_matrix[group][state][j];
      if (adjustment_state == j) {
	p[j] *= adjustment;
      }
    }
    total += p[j];
  }


  // draw a random number from 0 to the sum of all outgoing state transition probabilites
  double r = Random::draw_random(0, total);

  // find transition corresponding to this draw
  double sum = 0.0;
  for (int j = 0; j < this->number_of_states; j++) {
    sum += p[j];
    if (r < sum) {
      *next_state = j;
      return;
    }
  }

  // should never reach this point
  Utils::fred_abort("Markov_Chain::get_next_state: Help! Bad result: state = %d next_state = %d\n",
		  state, *next_state);
  return;
}


int Markov_Chain::get_age_group(double age) {
  return (int) this->age_map->find_value(age);
}
