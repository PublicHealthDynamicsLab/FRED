/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Events.h"
#include "Global.h"
#include "Markov_Model.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Population.h"
#include "Utils.h"


Markov_Model::Markov_Model(char* _name) {
  strcpy(this->name, _name);
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
  Params::get_indexed_param(this->name, "period_in_transition_probabilitiess", &(this->period_in_transition_probabilities));

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

  // gaurantee probability distribution by making same-state transition the default
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
  print();
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





////////////////

void Markov_Model::setup() {

  FRED_VERBOSE(0, "Markov_Model(%s)::setup\n", this->name);

  this->people_in_state = new person_vector_t [this->number_of_states];

  for (int i = 0; i < this->number_of_states; i++) {
    this->people_in_state[i].reserve(Global::Pop.get_pop_size());
    this->people_in_state[i].clear();
  }

  this->transition_to_state_event_queue = new Events* [this->number_of_states];
  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_to_state_event_queue[i] = new Events;
  }

  FRED_VERBOSE(0, "Markov_Model(%s)::setup finished\n", this->name);
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

void Markov_Model::process_transitions_to_state(int day, int state) {
  int size = this->transition_to_state_event_queue[state]->get_size(day);
  FRED_VERBOSE(0, "MARKOV_TRANS_TO_STATE %d %s day %d size %d\n",
	       state, this->state_name[state].c_str(), day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->transition_to_state_event_queue[state]->get_event(day, i);

    FRED_VERBOSE(1,"TRANSITION to state %d  day %d person %d\n",
		 state, day, person->get_id());

    int old_state = person->get_health_state(this);

    // delete from old list
    for (int j = 0; j < people_in_state[old_state].size(); j++) {
      if (people_in_state[old_state][j] == person) {
	people_in_state[old_state][j] = people_in_state[old_state].back();
	people_in_state[old_state].pop_back();
      }
    }

    // add to state list
    people_in_state[state].push_back(person);

    // update person's state
    person->set_health_state(this, state);

    // update next event list
    int new_state, transition_day;
    get_next_state_and_time(day, state, &new_state, &transition_day);
    this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);

  }
  this->transition_to_state_event_queue[state]->clear_events(day);
}

void Markov_Model::update(int day) {

  if (day == 0) {
    int popsize = Global::Pop.get_pop_size();
    for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
      Person* person = Global::Pop.get_person_by_index(p);
      if(person == NULL) {
	continue;
      }
      int state = get_initial_state();

      // add to state list
      people_in_state[state].push_back(person);

      // update person's state
      person->set_health_state(this, state);

      // update next event list
      int new_state, transition_day;
      get_next_state_and_time(day, state, &new_state, &transition_day);
      this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);

      printf("INITIALIZE MARKOV MODEL %s day %d person %d state %d new_state %d transition_day %d\n",
	     this->name, day, person->get_id(), state, new_state, transition_day);
    }
  }
  else {
    for (int i = 0; i < this->number_of_states; i++) {
      process_transitions_to_state(day, i);
    }
  }
}

void Markov_Model::report(int day) {
  FRED_VERBOSE(0, "Markov Model %s report day %d \n",this->name,day);
  for (int i = 0; i < this->number_of_states; i++) {
    char str[80];
    strcpy(str,this->state_name[i].c_str());
    Utils::track_value(day, str, (int)(people_in_state[i].size()));
  }
}

void Markov_Model::end_of_run() {

  // print end-of-run statistics here:
  printf("Markov_Model finished\n");

}


