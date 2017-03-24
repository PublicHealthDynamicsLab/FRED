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
// File: Age_Map.cpp
//
#include <iostream>
#include <iomanip>

#include "Age_Map.h"
#include "Params.h"
#include "Utils.h"
#include "Demographics.h"

using namespace std;

Age_Map::Age_Map() {
}

Age_Map::Age_Map(string name) {
  this->name = name + " Age Map";
  ages.clear();
  values.clear();
}

void Age_Map::read_from_input(string input, int i) {
  stringstream input_string;
  input_string << input << "[" << i << "]";
  this->read_from_input(input_string.str());
}

void Age_Map::read_from_input(string input, int i, int j) {
  stringstream input_string;
  input_string << input << "[" << i << "][" << j << "]";
  this->read_from_input(input_string.str());
}

void Age_Map::read_from_input(string input) {
  // FRED_VERBOSE(0,"read_from_input with string = |%s|\n", input.c_str());
  this->name = input + " Age Map";
  this->ages.clear();
  this->values.clear();

  char ages_string[FRED_STRING_SIZE];
  char values_string[FRED_STRING_SIZE];

  if(input.find("[") != string::npos) {
    // Need Special parsing if this is an array from input
    // Allows Condition specific values
    string input_tmp;
    string number;
    size_t found = input.find_first_of("[");
    size_t found2 = input.find_last_of("]");
    input_tmp.assign(input.begin(), input.begin() + found);
    number.assign(input.begin() + found + 1, input.begin() + found2);
    sprintf(ages_string, "%s.age_groups[%s]", input_tmp.c_str(), number.c_str());
    sprintf(values_string, "%s.age_values[%s]", input_tmp.c_str(), number.c_str());
  } else {
    sprintf(ages_string, "%s.age_groups", input.c_str());
    sprintf(values_string, "%s.age_values", input.c_str());
  }

  // Age map will be empty if not found.
  // make the following parameters optional
  Params::disable_abort_on_failure();

  Params::get_param_vector(ages_string, this->ages);
  Params::get_param_vector(values_string, this->values);

  // restore requiring parameters
  Params::set_abort_on_failure();

  if(quality_control() != true) {
    Utils::fred_abort("Bad input on age map %s", this->name.c_str());
  }
  return;
}

//overload to set ages and values from input string
void Age_Map::read_from_string(string ages_string, string values_string) {

  char * astr = new char [ages_string.length()+1];
  std::strcpy (astr, ages_string.c_str());
  char * vstr = new char [values_string.length()+1];
  std::strcpy (vstr, values_string.c_str());
  Params::get_param_vector_from_string(astr, this->ages);
  Params::get_param_vector_from_string(vstr, this->values);

  return;
}

void Age_Map::set_all_values(double val) {
  if (this->ages.size()) {
    this->ages.clear();
  }
  if (this->values.size()) {
    this->values.clear();
  }
  this->ages.push_back(Demographics::MAX_AGE);
  this->values.push_back(val);
}

int Age_Map::find_group(double age) {
  // printf("find_group: age = %.1f  groups %d \n", age, (int) this->ages.size());
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if (age < this->ages[i]) {
      return i;
    }
  }
  Utils::fred_abort("No age group found in %s for age %f\n", this->name.c_str(), age);
  return -1;
}

double Age_Map::find_value(double age) {
  // printf("find_value: age = %.1f  groups %d \n", age, (int) this->ages.size());
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if (age < this->ages[i]) {
      return this->values[i];
    }
  }
  return 0.0;
}

void Age_Map::print() const {
  cout << "\n" << this->name << "\n";
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    cout << "age less than " << setw(7) << setprecision(2) << fixed << this->ages[i] << ": "
	 << setw(7) << setprecision(4) << fixed << this->values[i] << "\n";
  }
  cout << "\n";
}

bool Age_Map::quality_control() const {
  // printf("AGE_MAP quality_control for %s\n", this->name.c_str());
  // printf("age groups = %d  values = %d\n", (int) this->ages.size(), (int) this->values.size());

  // First check to see there are a proper number of values for each age
  if(this->ages.size() != this->values.size()) {
    cout << "Help! Age_Map: " << this->name << ": Must have the same number of age groups and values\n";
    cout << "Number of Age Groups = " << this->ages.size() << "  Number of Values = " << this->values.size()
	 << "\n";
    return false;
  }

  if (this->ages.size() > 0) {
    // Next check that the ages groups are correct, the low and high ages are right
    for(unsigned int i = 0; i < this->ages.size()-1; i++) {
      if(this->ages[i] > this->ages[i+1]) {
	cout << "Help! Age_Map: " << this->name << ": Age Group " << i
	     << " invalid, low age higher than high\n";
	cout << " Low Age = " << this->ages[i] << "  High Age = " << this->ages[i+1] << "\n";
	return false;
      }
    }
  }
  return true;
}

