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
// File: Age_Map.h
//
// Age_Map is a class that holds a set of age-specific values
//

#ifndef _FRED_AGE_MAP_H
#define _FRED_AGE_MAP_H

#include <vector>
#include <string>
using namespace std;

#include "Global.h"
#include "Property.h"


class Age_Map {
public:

  /**
   * Default constructor
   */
  Age_Map();

  /**
   * @property Input a string that will used for property lookups
   */
  void read_properties(string prefix);

  /**
   * Find a value given an age. Will return 0.0 if no matching range is found.
   *
   * @property (double) age the age to find
   * @return the found value
   */
  double find_value(double age);


private:
  string name;
  vector<double> ages; // vector to hold the upper age for each age group
  vector<double> values; // vector to hold the values for each age range

  /**
   * Perform validation on the Age_Map, making sure the age groups are
   * mutually exclusive.
   */
  bool quality_control() const;
};

#endif
