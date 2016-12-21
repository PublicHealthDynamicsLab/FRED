/*
  This file is part of the FRED system.

  Copyright (c) 2010-2014, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Manager.cpp
//

#include <iostream>
#include <list>
#include <vector>

#include "Manager.h"
#include "Policy.h"
#include "Person.h"

using namespace std;

Manager::Manager() {
  this->pop = NULL;
  this->current_policy = -1;
}

Manager::~Manager() {
  for(unsigned int i=0; i < this->policies.size(); ++i) {
    delete this->policies[i];
  }
  this->policies.clear();
  this->results.clear();
}

Manager::Manager(Population *P) {
  this->pop = P;
  this->current_policy = 0;
}

int Manager::poll_manager(Person* p, int condition, int day) {
  int result = this->policies[this->current_policy]->choose(p, condition,day);
  return result;
}
