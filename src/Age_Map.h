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
// File: Age_Map.h
//
// Age_Map is a class that holds a set of age-specific ranged values
// The age ranges must be mutually exclusive.
//
#ifndef _FRED_AGEMAP_H
#define _FRED_AGEMAP_H

#include "Global.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <sstream>

#include "Params.h"

using namespace std;

/**
 * Class used to map a range of ages to a given value.  These often come from the parameters file and are structured
 * like in the following example:<br />
 *
 * vaccine_dose_efficacy_upper_ages = 2 4 120<br />
 * vaccine_dose_efficacy_values = 2 0.70 0.83<br />
 *
 * In this example, the ages [0,4) would map to value 0.70 and ages [4,120) would map to 0.83
 *
 */
class Age_Map {
public:
  // Creation operations
  /**
   * Default constructor
   */
  Age_Map();

  /**
   * Constructor that sets the Age_Map's name attribute
   *
   * @param Name the name of the Age_Map
   */
  Age_Map(string name);

  /**
   * @return whether or not the age group vector is empty
   */
  bool is_empty() const {
    return this->ages.empty();
  }

  // Additional creation operations for building an Age_Map
  /**
   * @param Input a string that will be parsed to use for a parameter lookup
   */
  void read_from_input(string input);

  /**
   * Will concatenate an index onto the input string and then pass to <code>Age_Map::read_from_input(string Input)</code>
   *
   * @param Input a string that will be parsed to use for a parameter lookup
   * @param i an index that will be appended
   */
  void read_from_input(string input, int i);

  /**
   * Will concatenate two indices onto the input string and then pass to <code>Age_Map::read_from_input(string Input)</code>
   *
   * @param Input a string that will be parsed to use for a parameter lookup
   * @param i an index that will be appended
   * @param j an index that will be appended
   */
  void read_from_input(string input, int i, int j);

  void set_all_values(double val);
  
  vector<double> get_ages(){
    return this->ages;		
  }

  vector<double> get_values(){
    return this->values;		
  }
  
  void set_ages(vector<double> input_ages){
    ages = input_ages;
  }
  
  void set_values(vector<double> input_values){
    values = input_values;
  }
  
  void read_from_string(string ages_string, string values_string);
  
  // Operations
  /**
   * Find a value given an age. Will return 0.0 if no matching range is found.
   *
   * @param (double) age the age to find
   * @return the found value
   */
  double find_value(double age);


  /**
   * Find the group associated with a given age. Will abort if no matching range is found.
   *
   * @param (double) age the age to find
   * @return the corresponding age group index
   */
  int find_group(double age);

  // Utility functions
  /**
   * Print out information about this object
   */
  void print() const;

  int get_groups() {
    return this->ages.size();
  }

  /**
   * Perform validation on the Age_Map, making sure the age
   * groups are mutually exclusive.
   */
  bool quality_control() const;

private:
  string name;
  vector<double> ages; // vector to hold the upper age for each age group
  vector<double> values; // vector to hold the values for each age range
};

#endif
