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
// File: AgeMap.cpp
//
#include <iostream>
#include <iomanip>

#include "Age_Map.h"
#include "Params.h"
#include "Utils.h"

using namespace std;

Age_Map::Age_Map() {
}

Age_Map::Age_Map(string name) {
  this->name = name + " Age Map";
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

// will leave this interface in, as it will automatically determine whether the 
// input variable is indexed or not.
void Age_Map::read_from_input(string input) {
  this->name = input + " Age Map";

  char ages_string[255];
  char values_string[255];

  if(input.find("[") != string::npos) {
    // Need Special parsing if this is an array from input
    // Allows Disease specific values
    string input_tmp;
    string number;
    size_t found = input.find_first_of("[");
    size_t found2 = input.find_last_of("]");
    input_tmp.assign(input.begin(), input.begin() + found);
    number.assign(input.begin() + found + 1, input.begin() + found2);
    sprintf(ages_string, "%s_ages[%s]", input_tmp.c_str(), number.c_str());
    sprintf(values_string, "%s_values[%s]", input_tmp.c_str(), number.c_str());
  } else {
    sprintf(ages_string, "%s_ages", input.c_str());
    sprintf(values_string, "%s_values", input.c_str());
  }

  vector<int> ages_tmp;
  int na = Params::get_param_vector(ages_string, ages_tmp);

  if(na % 2) {
    Utils::fred_abort("Error parsing Age_Map: %s: Must be an even number of age entries\n",
        input.c_str());
  }

  for(int i = 0; i < na; i += 2) {
    vector<int> ages_tmp2(2);
    ages_tmp2[0] = ages_tmp[i];
    ages_tmp2[1] = ages_tmp[i + 1];
    this->ages.push_back(ages_tmp2);
  }

  Params::get_param_vector(values_string, this->values);

  if(quality_control() != true) {
    Utils::fred_abort("");
  }
  return;
}

void Age_Map::read_table(string input) {
  this->name = input + " Age Map";

  char ages_string[255];
  char values_string[255];

  sprintf(ages_string, "%s_ages", input.c_str());

  vector<int> ages_tmp;
  int na = Params::get_param_vector(ages_string, ages_tmp);

  if(na % 2) {
    Utils::fred_abort("Error parsing Age_Map: %s: Must be an even number of age entries\n",
        input.c_str());
  }

  for(int i = 0; i < na; i += 2) {
    vector<int> ages_tmp2(2);
    ages_tmp2[0] = ages_tmp[i];
    ages_tmp2[1] = ages_tmp[i + 1];
    this->ages.push_back(ages_tmp2);
  }

  int rows = this->ages.size();
  int cols = rows;
  for(int i = 0; i < rows; i++) {
    double * row_tmp = new double[cols];
    sprintf(values_string, "%s_values[%d]", input.c_str(), i);
    Params::get_param_vector(values_string, row_tmp);
    this->table.push_back(row_tmp);
  }

  if(quality_control() != true) {
    Utils::fred_abort("");
  }
  return;
}

int Age_Map::get_minimum_age(void) const {
  int min = 9999999;
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if(this->ages[i][0] < min) {
      min = this->ages[i][0];
    }
  }
  return min;
}

int Age_Map::get_maximum_age(void) const {
  int max = -1;
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if(this->ages[i][1] > max) {
      max = this->ages[i][1];
    }
  }
  return max;
}

void Age_Map::add_value(int lower_age, int upper_age, double val) {
  vector<int> ages_tmp(2);
  ages_tmp[0] = lower_age;
  ages_tmp[1] = upper_age;
  this->ages.push_back(ages_tmp);
  this->values.push_back(val);
}

void Age_Map::set_value(int age, double val) {
  for(unsigned int i = 0; i < this->values.size(); i++) {
    if(age >= this->ages[i][0] && age <= this->ages[i][1]) {
      this->values[i] = val;
    }
  }
}

void Age_Map::set_all_values(double val) {
  for(unsigned int i = 0; i < this->values.size(); i++) {
    this->values[i] = val;
  }
}

double Age_Map::find_value(int age) const {

  //  if(age >=0 && age < ages[0]) return values[0];
  for(unsigned int i = 0; i < this->values.size(); i++) {
    if(age >= this->ages[i][0] && age <= this->ages[i][1]) {
      return this->values[i];
    }
  }

  return 0.0;
}

double Age_Map::find_value(int age1, int age2) const {

  unsigned int i, j;

  for(i = 0; i < this->values.size(); i++) {
    if(age1 >= this->ages[i][0] && age1 <= this->ages[i][1]) {
      break;
    }
  }

  for(j = 0; i < this->values.size(); j++) {
    if(age2 >= this->ages[j][0] && age2 <= this->ages[j][1]) {
      break;
    }
  }

  if(i < this->values.size() && j < this->values.size()) {
    return this->table[i][j];
  } else {
    return 0.0;
  }
}

void Age_Map::print() const {
  cout << "\n" << this->name << "\n";
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    cout << "age " << setw(2) << this->ages[i][0] << " to " << setw(3) << this->ages[i][1] << ": " << setw(7)
        << setprecision(4) << fixed << this->values[i] << "\n";
  }
  cout << "\n";
}

bool Age_Map::quality_control() const {
  // First check to see there are a proper number of values for each age
  if(this->ages.size() != this->values.size()) {
    cout << "Help! Age_Map: " << this->name << ": Must have the same number of age groups and values\n";
    cout << "Number of Age Groups = " << this->ages.size() << "  Number of Values = " << this->values.size()
        << "\n";
    return false;
  }

  // Next check that the ages groups are correct, the low and high ages are right
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    if(this->ages[i][0] > this->ages[i][1]) {
      cout << "Help! Age_Map: " << this->name << ": Age Group " << i
          << " invalid, low age higher than high\n";
      cout << " Low Age = " << this->ages[i][0] << "  High Age = " << this->ages[i][1] << "\n";
      return false;
    }
  }

  // Make sure the age groups are mutually exclusive
  for(unsigned int i = 0; i < this->ages.size(); i++) {
    int lowage = this->ages[i][0];
    int highage = this->ages[i][1];

    for(unsigned int j = 0; j < this->ages.size(); j++) {
      if(j != i) {
        if((lowage >= this->ages[j][0] && lowage <= this->ages[j][1])
            || (highage >= this->ages[j][0] && highage <= this->ages[j][1])) {
          cout << "Help! Age_Map: age group " << i << " not mutually exclusive with age group " << j
              << "\n";
          cout << lowage << " " << highage << " " << this->ages[j][0] << " " << this->ages[j][1] << "\n";
          return false;
        }
      }
    }
  }

  return true;
}

