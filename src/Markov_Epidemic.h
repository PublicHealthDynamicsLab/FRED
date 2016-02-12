/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_MARKOV_EPIDEMIC_H
#define _FRED_MARKOV_EPIDEMIC_H

#include <string>
#include <vector>
using namespace std;

#include "Epidemic.h"
class Markov_Chain;

typedef std::vector<Person*> person_vector_t;


class Markov_Epidemic : public Epidemic {

public:
  Markov_Epidemic(Condition * condition);

  ~Markov_Epidemic() {}

  void setup();

  void prepare();

  void get_imported_infections(int day) {}

  void update(int day);

  void markov_updates(int day);

  void transition_person(Person* person, int day, int state);

  void report_condition_specific_stats(int day);

  void end_of_run();

  void terminate_person(Person* person, int day);

  int get_age_code(int age);
  int get_sex_code(char sex);
  int get_race_code(int race);
  int get_county_code(int fips);

private:
  Markov_Chain* markov_chain;
  int number_of_states;
  int* count;
  // person_vector_t* people_in_state;

  // event queues
  Events** transition_to_state_event_queue;

  // optional target locations
  int target_locations;
  double * coordinates;

  // bayesian initialization
  int use_bayesian_init;
  double pnon[13][3][4][68];
  double pasymp[13][3][4][68];
  double pproblem[13][3][4][68];
  double dasymp[13][3][4][68];
  double dproblem[13][3][4][68];

};

#endif // _FRED_MARKOV_EPIDEMIC_H
