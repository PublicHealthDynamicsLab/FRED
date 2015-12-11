/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Markov_Epidemic.cc

#include "Markov_Epidemic.h"
#include "Markov_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Utils.h"
#include "Population.h"
#include "Random.h"

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

  FRED_VERBOSE(0, "Markov_Epidemic::setup finished\n");
}


void Markov_Epidemic::update(int day) {

  FRED_VERBOSE(0, "Markov_Epidemic::update day %d\n", day);

  if (day == 0) {
    double pct_init_nonusers = 32675.0 / 34653.0;
    double pct_init_asymp = 861.0 / 34653.0;
    double pct_init_symp = 1117.0 / 34653.0;

    // set initial users
    for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
      Person* person = Global::Pop.get_person_by_index(p);
      if(person == NULL) {
	continue;
      }
      double r = Random::draw_random();
      if (r < pct_init_asymp + pct_init_symp) {

	// infect the person
	person->become_exposed(this->id, NULL, NULL, day);
	// notify the epidemic
	Epidemic::become_exposed(person, day);

	if (r < pct_init_symp) {
	  // person is problem user
	  // person->become_symptomatic(this->disease);
	  people_in_state[2].push_back(person);
	}
	else {
	  // person is asymptomatic user
	  people_in_state[1].push_back(person);
	}
      }
      else {
	people_in_state[0].push_back(person);
      }
    }
  }

  if (day > 0 && day % 7 == 0) {

    // create a matrix to count people for transition
    int ** transitions;
    transitions = new int * [this->number_of_states];
    for (int i = 0; i < this->number_of_states; i++) {
      transitions[i] = new int [this->number_of_states];
    }
    
    // create temporary lists of people in transition
    person_vector_t * in_transition;
    in_transition = new person_vector_t [this->number_of_states];

    // compute expected numbers of Markov transitions
    for (int i = 0; i < this->number_of_states; i++) {
      int size = people_in_state[i].size();
      transitions[i][i] = 0;
      int sum = 0;
      for (int j = 0; j < this->number_of_states; j++) {
	if (i != j) {
	  // find number of people to transition from state i to j
	  double pij = this->disease->get_natural_history()->get_transition_probability(i,j);
	  transitions[i][j] = round(pij * size);
	  sum += transitions[i][j];
	}
      }
      in_transition[i].reserve(sum);

      // shuffle the list
      FYShuffle<Person *>(people_in_state[i]);
      for (int t = 0; t < sum; t++) {
	// move people from the back of the shuffled list to the temp list
	Person * person = this->people_in_state[i].back();
	this->people_in_state[i].pop_back();
	in_transition[i].push_back(person);
      }
    }

    // make the transitions
    for (int i = 0; i < this->number_of_states; i++) {
      for (int j = 0; j < this->number_of_states; j++) {
	for (int t = 0 ; t < transitions[i][j]; t++) {
	  assert(in_transition[i].size() > 0);
	  Person * person = in_transition[i].back();
	  in_transition[i].pop_back();
	  people_in_state[j].push_back(person);
	  process_transition(day, i, j, person);
	}
      }
    }
  }

  Epidemic::update(day);
}


void Markov_Epidemic::process_transition(int day, int i, int j, Person * person) {

  FRED_VERBOSE(0, "Markov transition day %d state %d to state %d person %d\n",
	       day,i,j,person->get_id());

  if (i == 0) {
    // infect the person
    person->become_exposed(this->id, NULL, NULL, day);
    // notify the epidemic
    Epidemic::become_exposed(person, day);
    if (j == 1) {
      // person is asymptomatic
    }
    if (j == 2) {
      // person->become_symptomatic(this->disease);
    }
  }
  
  if (i == 1) {
    if (j == 0) {
      Epidemic::recover(person, day);
    }
    if (j == 2) {
      // person->become_symptomatic(this->disease);
    }
  }
  
  if (i == 2) {
    if (j == 0) {
      Epidemic::recover(person, day);
    }
    if (j == 1) {
      // person->resolve_symptoms(this->disease);
    }
  }

  FRED_VERBOSE(0, "Markov transition day %d state %d to state %d person %d is_infected %d is_symptomatic %d \n",
	       day,i,j,person->get_id(), person->is_infected(0)?1:0, person->is_symptomatic(0)?1:0);
}


void Markov_Epidemic::report_disease_specific_stats(int day) {

  // put values that should appear in outfile here:

  for (int i = 0; i < this->number_of_states; i++) {
    char str[80];
    strcpy(str,this->disease->get_natural_history()->get_state_name(i).c_str());
    track_value(day, str, (int)(people_in_state[i].size()));
  }

  // print additional daily output here:

}

void Markov_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Markov_Epidemic finished\n");

}


