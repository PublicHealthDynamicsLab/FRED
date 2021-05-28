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
// File: Admin_Division.h
//

#ifndef _FRED_ADMIN_DIVISION_H
#define _FRED_ADMIN_DIVISION_H

#include <vector>
#include <unordered_map>
using namespace std;

class Place;

class Admin_Division {
public:

  Admin_Division(long long int _admin_code);
  virtual ~Admin_Division();

  virtual void setup();

  virtual void add_household(Place* h);
  
  void set_higher_division(Admin_Division* high) {
    this->higher = high;
  }

  Admin_Division* get_higher_division() {
    return this->higher;
  }

  virtual void add_subdivision(Admin_Division* sub) {
    this->subdivisions.push_back(sub);
  }

  virtual int get_number_of_subdivisions() {
    return this->subdivisions.size();;
  }

  Admin_Division* get_subdivision(int i) {
    return this->subdivisions[i];
  }

  long long int get_admin_division_code() {
    return this->admin_code;
  }

  int get_number_of_households() {
    return (int) this->households.size();
  }

  Place* get_household(int i);

  int get_population_size();

protected:
  long long int admin_code;

  // pointers to households
  std::vector<Place*> households;

  // pointer to higher level division
  Admin_Division* higher;

  // vector of subdivisions
  std::vector<Admin_Division*> subdivisions;

};

#endif // _FRED_ADMIN_DIVISION_H
