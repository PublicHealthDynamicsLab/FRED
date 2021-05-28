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
// File: Census_Tract.h
//

#ifndef _FRED_CENSUS_TRACT_H
#define _FRED_CENSUS_TRACT_H

#include <assert.h>
#include <vector>
#include <unordered_map>
using namespace std;

#include "Global.h"
#include "Admin_Division.h"

// school attendance maps
typedef std::unordered_map<int,int> attendance_map_t;
typedef attendance_map_t::iterator attendance_map_itr_t;
typedef std::unordered_map<int,Place*> school_id_map_t;


class Census_Tract : public Admin_Division {
public:

  Census_Tract(long long int _admin_code);
  ~Census_Tract();

  void setup();

  void update(int day);

  // for selecting new workplaces
  void set_workplace_probabilities();
  Place* select_new_workplace();
  void report_workplace_sizes();

  // for selecting new schools
  void set_school_probabilities();
  Place* select_new_school(int grade);
  void report_school_sizes();
  bool is_school_attended(int school_id, int grade) {
    return (school_counts[grade].find(school_id) != school_counts[grade].end());
  }

  static int get_number_of_census_tracts() {
    return Census_Tract::census_tracts.size();
  }

  static Census_Tract* get_census_tract_with_index(int n) {
    return Census_Tract::census_tracts[n];
  }

  static Census_Tract* get_census_tract_with_admin_code(long long int census_tract_admin_code);

  static void setup_census_tracts();

private:

  // schools attended by people in this census_tract, with probabilities
  std::vector<Place*> schools_attended[Global::GRADES];
  std::vector<double> school_probabilities[Global::GRADES];

  // list of schools attended by people in this county
  attendance_map_t school_counts[Global::GRADES];
  school_id_map_t school_id_lookup;

  // workplaces attended by people in this census_tract, with probabilities
  std::vector<Place*> workplaces_attended;
  std::vector<double> workplace_probabilities;

  static std::vector<Census_Tract*> census_tracts;
  static std::unordered_map<long long int,Census_Tract*> lookup_map;

};

#endif // _FRED_CENSUS_TRACT_H
