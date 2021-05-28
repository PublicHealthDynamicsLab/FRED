/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

#ifndef _FRED_STATE_SPACE_H
#define _FRED_STATE_SPACE_H

#include <string>
#include <vector>
using namespace std;

class State_Space {

public:
  State_Space(char* model_name);

  ~State_Space();

  void get_properties();

  void print();

  char* get_name() {
    return this->name;
  }

  int get_number_of_states() {
    return this->number_of_states;
  }

  string get_state_name(int state) {
    if (state < this->number_of_states) {
      return state_name[state];
    }
    else {
      return "";
    }
  }

  int get_state_from_name(char* sname);
  int get_state_from_name(string sname);

private:
  char name[256];
  char tmp[256];
  int number_of_states;
  std::vector<std::string> state_name;
};

#endif
