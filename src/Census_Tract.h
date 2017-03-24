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
// File: Census_Tract.h
//

#ifndef _FRED_CENSUS_TRACT_H
#define _FRED_CENSUS_TRACT_H

#include <assert.h>
#include <vector>
#include <unordered_map>
using namespace std;

#include "Global.h"
class Household;
class Person;
class School;
class Workplace;

// 2-d array of lists
typedef std::vector<int> HouselistT;

// school attendance maps
typedef std::unordered_map<int,int> attendance_map_t;
typedef attendance_map_t::iterator attendance_map_itr_t;
typedef std::unordered_map<int,School*> sid_map_t;


class Census_Tract {
public:

  Census_Tract(long int _fips);
  ~Census_Tract();

  void setup();

  long int get_fips() {
    return this->fips;
  }

  int get_number_of_households() {
    return (int) this->households.size();
  }

  int get_current_popsize() {
    return this->tot_current_popsize;
  }

  bool increment_popsize(Person* person);
  bool decrement_popsize(Person* person);

  void add_household(Household* h) {
    this->households.push_back(h);
  }
  
  void update(int day);
  int find_fips_code(long int n);
  void report_census_tract_population();

  // for selecting new workplaces
  void set_workplace_probabilities();
  Workplace* select_new_workplace();
  void report_workplace_sizes();

  // for selecting new schools
  void set_school_probabilities();
  School* select_new_school(int grade);
  void report_school_sizes();
  bool is_school_attended(int sid, int grade) {
    return (school_counts[grade].find(sid) != school_counts[grade].end());
  }

  Household* get_household(int i) {
    assert(0 <= i && i < this->households.size());
    return this->households[i];
  }
  void report();

private:
  long int fips;
  int tot_current_popsize;

  // pointers to households
  std::vector<Household*> households;

  // schools attended by people in this census_tract, with probabilities
  std::vector<School*> schools_attended[Global::GRADES];
  std::vector<double> school_probabilities[Global::GRADES];

  // list of schools attended by people in this county
  attendance_map_t school_counts[Global::GRADES];
  sid_map_t sid_to_school;

  // workplaces attended by people in this census_tract, with probabilities
  std::vector<Workplace*> workplaces_attended;
  std::vector<double> workplace_probabilities;
};

#endif // _FRED_CENSUS_TRACT_H
