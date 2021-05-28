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
// File: Regional_Patch.h
//
#ifndef _FRED_REGIONAL_PATCH_H
#define _FRED_REGIONAL_PATCH_H

#include <vector>
#include <set>

#include "Person.h"
#include "Abstract_Patch.h"
#include "Global.h"
#include "Utils.h"
#include "Household.h"
#include "Place.h"

class Regional_Layer;

class Regional_Patch : public Abstract_Patch {
public:
  Regional_Patch(Regional_Layer* grd, int i, int j);
  Regional_Patch();
  ~Regional_Patch() {
  }
  void setup(Regional_Layer* grd, int i, int j);
  void quality_control();
  double distance_to_patch(Regional_Patch* patch2);
  void add_person_to_patch(Person* p) {
    this->person.push_back(p);
  }

  int get_popsize() {
    return this->popsize;
  }

  Person* select_random_person();
  Person* select_random_student(int age_);
  Person* select_random_worker();
  void set_max_popsize(int n);
  int get_max_popsize() {
    return this->max_popsize;
  }

  double get_pop_density() {
    return this->pop_density;
  }

  void end_membership(Person* pers);
  void add_workplace(Place* place);
  void add_hospital(Place* place);
  place_vector_t get_hospitals() {
    return this->hospitals;
  }
  Place* get_nearby_workplace(Place* place, int staff);
  Place* get_closest_workplace(double x, double y, int min_size, int max_size, double* min_dist);

  int get_id() {
    return this->id;
  }

  void swap_county_people();

protected:
  Regional_Layer* grid;
  int popsize;
  person_vector_t person;
  std::set<int> counties;
  int max_popsize;
  double pop_density;
  int id;
  static int next_patch_id;
  place_vector_t workplaces;
  place_vector_t hospitals;
  person_vector_t students_by_age[100];
  person_vector_t workers;
};

#endif // _FRED_REGIONAL_PATCH_H
