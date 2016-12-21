/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_MARKOV_CHAIN_H
#define _FRED_MARKOV_CHAIN_H

#include <string>
using namespace std;

class Age_Map;


class Markov_Chain {

public:
  Markov_Chain();

  ~Markov_Chain();

  void setup(char* condition_name) {
    strcpy(this->name, condition_name);
  }

  void get_parameters();

  void print();

  char* get_name() {
    return this->name;
  }

  int get_number_of_states() {
    return this->number_of_states;
  }

  std::string get_state_name(int i) {
    return this->state_name[i];
  }

  int get_initial_state(double age, int adjustment_state = 0, double adjustment = 1.0);

  void get_next_state(int day, double age, int state, int* next_state, int* transition_day, int adjustment_state = -1, double adjustment = 1.0);

  int get_age_group(double age);

protected:
  char name[256];
  int number_of_states;
  std::vector<std::string> state_name;

private:
  Age_Map* age_map;
  int age_groups;
  double** state_initial_percent;
  double*** transition_matrix;
  double transition_time_period;
};

#endif
