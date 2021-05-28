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

//
//
// File: State.h
//


#ifndef _FRED_STATE_H
#define _FRED_STATE_H

#include <vector>
#include <unordered_map>
using namespace std;

#include "Admin_Division.h"

class State : public Admin_Division {
public:

  State(long long int _admin_code);

  ~State();

  static int get_number_of_states() {
    return State::states.size();
  }

  static State* get_state_with_index(int i) {
    return State::states[i];
  }

  static State* get_state_with_admin_code(long long int state_code);

private:

  // static variables
  static std::vector<State*> states;
  static std::unordered_map<long long int,State*> lookup_map;

};

#endif // _FRED_STATE_H
