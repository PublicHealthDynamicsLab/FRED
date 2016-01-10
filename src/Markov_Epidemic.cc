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
#include "Markov_Model.h"
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

  // initialize Markov specific-variables here:

  this->markov_model = static_cast<Markov_Natural_History*>(this->disease->get_natural_history())->get_markov_model();
  FRED_VERBOSE(0, "Markov_Epidemic(%s)::setup\n", this->disease->get_disease_name());
  this->number_of_states = this->markov_model->get_number_of_states();
  FRED_VERBOSE(0, "Markov_Epidemic::setup states = %d\n", this->number_of_states);

  this->people_in_state = new person_vector_t [this->number_of_states];

  this->transition_to_state_event_queue = new Events* [this->number_of_states];
  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_to_state_event_queue[i] = new Events;
  }

  FRED_VERBOSE(0, "Markov_Epidemic(%s)::setup finished\n", this->disease->get_disease_name());
}


void Markov_Epidemic::prepare() {

  FRED_VERBOSE(0, "Markov_Epidemic(%s)::prepare\n", this->disease->get_disease_name());

  for (int i = 0; i < this->number_of_states; i++) {
    this->people_in_state[i].reserve(Global::Pop.get_pop_size());
    this->people_in_state[i].clear();
  }

  // initialize the population

  int popsize = Global::Pop.get_pop_size();
  for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
    Person* person = Global::Pop.get_person_by_index(p);
    if(person == NULL) {
      continue;
    }
    int state = this->markov_model->get_initial_state();
    
    // add to state list
    people_in_state[state].push_back(person);
    
    // update person's state
    person->set_health_state(this->id, state, 0);

    // update next event list
    int new_state, transition_day;
    this->markov_model->get_next_state_and_time(0, state, &new_state, &transition_day);
    this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);
    
    // update person's next state
    person->set_next_health_state(this->id, new_state, transition_day);
    
    FRED_STATUS(1,"INITIALIZE MARKOV Epidemic %s day %d person %d state %d new_state %d transition_day %d\n",
	   this->disease->get_disease_name(), 0, person->get_id(), state, new_state, transition_day);
  }

  FRED_STATUS(0, "Markov_Epidemic(%s)::prepare: state/size: \n", this->disease->get_disease_name());
  for (int i = 0; i < this->number_of_states; i++) {
    FRED_STATUS(0, " | %d %s = %d", i, this->markov_model->get_state_name(i).c_str(), this->people_in_state[i].size());
  }
  FRED_STATUS(0, "\n");

  FRED_VERBOSE(0, "Markov_Epidemic(%s)::prepare finished\n", this->disease->get_disease_name());
}


void Markov_Epidemic::process_transitions_to_state(int day, int state) {
  int size = this->transition_to_state_event_queue[state]->get_size(day);
  FRED_VERBOSE(0, "MARKOV_TRANS_TO_STATE %d day %d size %d\n", state, day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->transition_to_state_event_queue[state]->get_event(day, i);

    FRED_VERBOSE(0,"TRANSITION to state %d  day %d person %d\n",
		 state, day, person->get_id());

    int old_state = person->get_health_state(this->markov_model);

    FRED_VERBOSE(0,"TRANSITION to state %d  day %d person %d old_state %d\n",
		 state, day, person->get_id(), old_state);

    // delete from old list
    for (int j = 0; j < people_in_state[old_state].size(); j++) {
      if (people_in_state[old_state][j] == person) {
	people_in_state[old_state][j] = people_in_state[old_state].back();
	people_in_state[old_state].pop_back();
      }
    }

    // add to active people list
    people_in_state[state].push_back(person);

    // update person's state
    person->set_health_state(this->id, state, day);

    // update next event list
    int new_state, transition_day;
    this->markov_model->get_next_state_and_time(day, state, &new_state, &transition_day);
    this->transition_to_state_event_queue[new_state]->add_event(transition_day, person);
    
    // update person's next state
    person->set_next_health_state(this->id, new_state, transition_day);

    // handle disease-related transition
  }
  this->transition_to_state_event_queue[state]->clear_events(day);
}

void Markov_Epidemic::update(int day) {
  Epidemic::update(day);
}


void Markov_Epidemic::markov_updates(int day) {
  FRED_VERBOSE(0, "Markov_Epidemic(%s)::update for day %d\n", this->disease->get_disease_name(), day);
  for (int i = 0; i < this->number_of_states; i++) {
    process_transitions_to_state(day, i);
  }
  FRED_VERBOSE(0, "Markov_Epidemic(%s)::update finished for day %d\n", this->disease->get_disease_name(), day);
  return;
}

void Markov_Epidemic::report_disease_specific_stats(int day) {
  FRED_VERBOSE(0, "Markov Epidemic %s report day %d \n",this->disease->get_disease_name(),day);
  for (int i = 0; i < this->number_of_states; i++) {
    char str[80];
    strcpy(str,this->markov_model->get_state_name(i).c_str());
    Utils::track_value(day, str, (int)(people_in_state[i].size()));
  }
}

void Markov_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Markov_Epidemic finished\n");

}



/// move to Health.cc

void Markov_Epidemic::process_transition(int day, int i, int j, Person * person) {

  char state_i[80];
  char state_j[80];

  strcpy(state_i, this->markov_model->get_state_name(i).c_str());
  strcpy(state_j, this->markov_model->get_state_name(j).c_str());

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

