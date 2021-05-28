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
// File: Preference.h
//


#ifndef _FRED_PREFERENCE_H
#define _FRED_PREFERENCE_H

#include "Global.h"

class Preference {

public:
  Preference();
  ~Preference();
  void add_preference_expressions(string expr_str);
  Person* select_person(Person* person, person_vector_t &people);

private:
  expression_vector_t expressions;
  double get_value(Person* person, Person* other);
  string get_name();
};

#endif // _FRED_PREFERENCE_H
