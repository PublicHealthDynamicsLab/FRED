/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
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
#include "Place_List.h"
#include "Place.h"

typedef vector<Person*> person_vec;	     //vector of person pointers
typedef vector<Place*> place_vec;	      //vector of place pointers

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
    // <-------------------------------------------------------------- Mutex
    fred::Scoped_Lock lock(this->mutex);
    this->person.push_back(p);
    if(Global::Enable_Vector_Layer) {
      Household* hh = static_cast<Household*>(p->get_household());
      if(hh == NULL) {
        if(Global::Enable_Hospitals && p->is_hospitalized() && p->get_permanent_household() != NULL) {
          hh = static_cast<Household*>(p->get_permanent_household());
        }
      }
      int h_county = hh->get_county_fips();
      this->counties.insert(h_county);
      if(p->is_student()) {
	      int age_ = 0;
	      age_ = p->get_age();
	      if(age_>100) {
	        age_=100;
	      }
	      if(age_<0) {
	        age_=0;
	      }
	      this->students_by_age[age_].push_back(p);
      }
      if(p->get_workplace() != NULL) {
	      this->workers.push_back(p);
      }
    }
    ++this->demes[p->get_deme_id()];
    ++this->popsize;
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

  void unenroll(Person* pers);
  void add_workplace(Place* place);
  void add_hospital(Place* place);
  place_vec get_hospitals() {
    return this->hospitals;
  }
  Place* get_nearby_workplace(Place* place, int staff);
  Place* get_closest_workplace(double x, double y, int min_size, int max_size, double* min_dist);

  int get_id() {
    return this->id;
  }

  void swap_county_people();

  unsigned char get_deme_id();

protected:
  fred::Mutex mutex;
  Regional_Layer* grid;
  int popsize;
  vector<Person*> person;
  std::set<int> counties;
  int max_popsize;
  double pop_density;
  int id;
  static int next_patch_id;
  place_vec workplaces;
  place_vec hospitals;
  person_vec students_by_age[100];
  person_vec workers;
  std::map<unsigned char, int> demes;
};

#endif // _FRED_REGIONAL_PATCH_H
