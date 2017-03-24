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

//
//
// File: Condition_List.h
//

#ifndef _FRED_CONDITION_LIST_H
#define _FRED_CONDITION_LIST_H

#include <vector>
#include <string>
using namespace std;

class Condition;


class Condition_List {

public:

  Condition_List() {};

  ~Condition_List() {};

  void get_parameters();

  void setup();

  Condition* get_condition(int condition_id) {
    return this->conditions[condition_id];
  }

  Condition* get_condition(char* condition_name);

  string get_condition_name(int condition_id) {
    return this->condition_name[condition_id];
  }

  int get_number_of_conditions() { 
    return this->number_of_conditions;
  }

  void prepare_conditions();

  void end_of_run();

private:
  std::vector <Condition*> conditions;
  std::vector<string> condition_name;
  int number_of_conditions;
};

#endif // _FRED_CONDITION_LIST_H
