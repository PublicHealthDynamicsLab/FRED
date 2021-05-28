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

#ifndef _FRED_CLAUSE_H
#define _FRED_CLAUSE_H

#include "Global.h"

class Person;
class Predicate;


class Clause {
public:

  Clause();
  Clause(string s);
  string get_name();
  bool parse();
  bool get_value(Person* person, Person* other = NULL);
  bool is_warning() {
    return this->warning;
  }

private:
  std::string name;
  std::vector<Predicate*>predicates;
  bool warning;

};

#endif


