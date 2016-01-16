/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

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
    return conditions[condition_id];
  }

  Condition* get_condition(char* condition_name);

  int get_number_of_conditions() { 
    return number_of_conditions;
  }

  void prepare_conditions();

  void end_of_run();

private:
  std::vector <Condition*> conditions;
  int number_of_conditions;
  string * condition_name;
};

#endif // _FRED_CONDITION_LIST_H
