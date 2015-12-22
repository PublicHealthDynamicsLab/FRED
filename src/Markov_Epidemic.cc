/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Markov_Epidemic.cc

#include "Events.h"
#include "Disease.h"
#include "Global.h"
#include "Health.h"
#include "Markov_Epidemic.h"
#include "Markov_Natural_History.h"
#include "Person.h"
#include "Population.h"
#include "Place.h"
#include "Random.h"
#include "Utils.h"

Markov_Epidemic::Markov_Epidemic(Disease* _disease) :
  Epidemic(_disease) {
}


void Markov_Epidemic::setup() {

  Epidemic::setup();

  FRED_VERBOSE(0, "Markov_Epidemic::setup\n");

  // initialize Markov specific-variables here:

  this->number_of_states = this->disease->get_natural_history()->get_number_of_states();
  FRED_VERBOSE(0, "Markov_Epidemic::setup states = %d\n", this->number_of_states);


  this->people_in_state = new person_vector_t [this->number_of_states];

  for (int i = 0; i < this->number_of_states; i++) {
    this->people_in_state[i].reserve(Global::Pop.get_pop_size());
    this->people_in_state[i].clear();
  }

  this->transition_to_state_event_queue = new Events* [this->number_of_states];
  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_to_state_event_queue[i] = new Events;
  }

  FRED_VERBOSE(0, "Markov_Epidemic::setup finished\n");
}


void Markov_Epidemic::get_next_state_and_time(int day, int old_state, int* new_state, int* transition_day) {
  *transition_day = -1;
  *new_state = old_state;
  for (int j = 0; j < this->number_of_states; j++) {
    if (j == old_state) {
      continue;
    }
    double lambda = this->disease->get_natural_history()->get_transition_probability(old_state, j);
    if (lambda == 0.0) {
      continue;
    }
    int t = day + 1 + round(Random::draw_exponential(lambda) * 7.0);
    if (*transition_day < 0 || t < *transition_day) {
      *transition_day = t;
      *new_state = j;
    }
  }
}

void Markov_Epidemic::process_transitions_to_state(int day, int state) {
  int size = this->transition_to_state_event_queue[state]->get_size(day);
  FRED_VERBOSE(0, "MARKOV_TRANS_TO_STATE %d day %d size %d\n", state, day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->transition_to_state_event_queue[state]->get_event(day, i);

    FRED_VERBOSE(1,"TRANSITION to state %d  day %d person %d\n",
		 state, day, person->get_id());

    int old_state = person->get_health()->get_infection(this->id)->get_state();

    // delete from old list
    for (int j = 0; j < people_in_state[old_state].size(); j++) {
      if (people_in_state[old_state][j] == person) {
	people_in_state[old_state][j] = people_in_state[old_state].back();
	people_in_state[old_state].pop_back();
      }
    }

    // add to active people list
    people_in_state[state].push_back(person);

    // update person's health chart
    person->get_health()->get_infection(this->id)->set_state(state);

    // change person's epidemic status
    // process_transition(day, old_state, state, person);

    // update next event list
    int new_state, transition_day;
    get_next_state_and_time(day, state, &new_state, &transition_day);
    this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);

  }
  this->transition_to_state_event_queue[state]->clear_events(day);
}

void Markov_Epidemic::preliminary_updates(int day) {

  if (day == 0) {
    int popsize = Global::Pop.get_pop_size();
    for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
      Person* person = Global::Pop.get_person_by_index(p);
      if(person == NULL) {
	continue;
      }
      if (person->is_infected(this->id)) {
	int state = person->get_health()->get_infection(this->id)->get_state();
	people_in_state[state].push_back(person);
	int new_state, transition_day;
	get_next_state_and_time(day, state, &new_state, &transition_day);
	printf("PRELIM day %d person %d state %d new_state %d transition_day %d\n",
	       day, person->get_id(), state, new_state, transition_day);
	this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);
      }
    }
  }
  else {
    for (int i = 0; i < this->number_of_states; i++) {
      process_transitions_to_state(day, i);
    }
  }

  int nn = people_in_state[0].size();
  int an = people_in_state[1].size();
  int pn = people_in_state[2].size();
  printf("PRELIM UPDATE day %d non %d asymp %d prob %d\n", day, nn, an, pn);
}

void Markov_Epidemic::update(int day) {
  Epidemic::update(day);
}


void Markov_Epidemic::process_transition(int day, int i, int j, Person * person) {

  char state_i[80];
  char state_j[80];

  strcpy(state_i, this->disease->get_natural_history()->get_state_name(i).c_str());
  strcpy(state_j, this->disease->get_natural_history()->get_state_name(j).c_str());

  FRED_VERBOSE(0, "Markov transition day %d from %s to %s person %d\n",
	       day,state_i,state_j,person->get_id());

  if (i == 0) {
    // infect the person
    person->become_exposed(this->id, NULL, NULL, day);
    // notify the epidemic
    Epidemic::become_exposed(person, day);
    if (j == 1) {
      // person is asymptomatic
    }
    if (j == 2) {
      person->become_symptomatic(this->disease);
    }
  }
  
  if (i == 1) {
    if (j == 0) {
      Epidemic::recover(person, day);
    }
    if (j == 2) {
      person->become_symptomatic(this->disease);
    }
  }
  
  if (i == 2) {
    if (j == 0) {
      Epidemic::recover(person, day);
    }
    if (j == 1) {
      person->resolve_symptoms(this->disease);
    }
  }

  FRED_VERBOSE(0, "Markov transition day %d state %d to state %d person %d is_infected %d is_symptomatic %d \n",
	       day,i,j,person->get_id(), person->is_infected(0)?1:0, person->is_symptomatic(0)?1:0);
}


void Markov_Epidemic::report_disease_specific_stats(int day) {
  FRED_VERBOSE(0, "Markov report day %d \n",day);
  for (int i = 0; i < this->number_of_states; i++) {
    char str[80];
    strcpy(str,this->disease->get_natural_history()->get_state_name(i).c_str());
    track_value(day, str, (int)(people_in_state[i].size()));
  }
}

void Markov_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Markov_Epidemic finished\n");

}


