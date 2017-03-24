/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
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

  virtual void get_parameters();

  virtual void print();

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
