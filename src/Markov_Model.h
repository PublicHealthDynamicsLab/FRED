/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_MARKOV_MODEL_H
#define _FRED_MARKOV_MODEL_H

#include <string>
#include <vector>
using namespace std;

typedef std::vector<Person*> person_vector_t;
class Events;



class Markov_Model {

public:
  Markov_Model();

  ~Markov_Model();

  void setup(char* _name) {
    strcpy(this->name, _name);
  }

  void get_parameters();

  void print();

  void prepare();

  int get_number_of_states() {
    return this->number_of_states;
  }

  std::string get_state_name(int i) {
    return this->state_name[i];
  }

  int get_initial_state();

  void setup();

  void get_next_state_and_time(int day, int old_state, int* new_state, int* transition_day);

  void process_transitions_to_state(int day, int state);

  void update(int day);

  void report(int day);

  void end_of_run();

protected:
  char name[256];
  int number_of_states;
  std::vector<std::string>state_name;

private:
  std::vector<double>state_initial_percent;
  double ** transition_matrix;
  int period_in_transition_probabilities;
  person_vector_t * people_in_state;

  // event queues
  Events** transition_to_state_event_queue;
};

#endif
