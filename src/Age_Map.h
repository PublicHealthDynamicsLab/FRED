/*
 This file is part of the FRED system.

 Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
 Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
 Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

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
 * vaccine_dose_efficacy_ages[0][0] = 4 0 4 5 100<br />
 * vaccine_dose_efficacy_values[0][0] = 2 0.70 0.83<br />
 *
 * In this example, the ages 0 - 4 would map to value 0.70 and ages 5 - 100 would map to 0.83
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
   * @return the size of the age range vector
   */
  int get_num_ages() const {
    return this->ages.size();
  }

  /**
   * @return the minimum age in the age range vector
   */
  int get_minimum_age() const;

  /**
   * @return the maximum age in the age range vector
   */
  int get_maximum_age() const;

  /**
   * @return whether or not the age range vector is empty
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
   * Will concatenate tow indices onto the input string and then pass to <code>Age_Map::read_from_input(string Input)</code>
   *
   * @param Input a string that will be parsed to use for a parameter lookup
   * @param i an index that will be appended
   * @param j an index that will be appended
   */
  void read_from_input(string input, int i, int j);

  void read_table(string input);

  /**
   * Add a value to the Age_Map
   *
   * @param lower_age the lower bound for the key
   * @param upper_age the upper bound for the key
   * @param val what value should be returned
   */
  void add_value(int lower_age, int upper_age, double val);

  /**
   * Set a value in the Age_Map
   *
   * @param age the age to use for the key
   * @param val what value should be returned
   */
  void set_value(int age, double val);

  void set_all_values(double val);

  // Operations
  /**
   * Tries to find a value given an age.  If the age falls within an lower and upper bound for a given age range, then
   * the associated value is returned.  Will return 0.0 if no matching range is found.
   *
   * @param age the age to find
   * @return the found value
   */
  double find_value(int age) const;

  // Operations
  /**
   * Tries to find a value in a table indexed by age ranges.
   * If the age falls within an lower and upper bound for a given age range, then
   * the associated value is returned.  Will return 0.0 if no matching range is found.
   *
   * @param age the age to find
   * @return the found value
   */
  double find_value(int age1, int age2) const;

  // Utility functions
  /**
   * Print out information about this object
   */
  void print() const;

  /**
   * Perform validation on the Age_Map. First check to see there are a proper number of values for each age.
   * Next checks that the ages groups are correct, the low and high ages are right.  Last, makes sure the age
   * groups are mutually exclusive.
   */
  bool quality_control() const;

private:
  string name;
  vector<vector<int> > ages;  // vector to hold the age ranges
  vector<double> values;       // vector to hold the values for each age range
  vector<double *> table;	// table to hold the values indexed by age range
};

#endif
