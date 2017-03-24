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
// File: Neighborhood_Patch.h
//

#ifndef _FRED_NEIGHBORHOOD_PATCH_H
#define _FRED_NEIGHBORHOOD_PATCH_H

#include <vector>
#include <string.h>

using namespace std;

typedef std::vector<Place *> place_vector;

#include "Abstract_Patch.h"
#include "Global.h"
#include "Place.h"

class Person;
class Neighborhood_Layer;
class Neighborhood;
class Household;


class Neighborhood_Patch : public Abstract_Patch {
public:

  /**
   * Default constructor
   */
  Neighborhood_Patch();
  ~Neighborhood_Patch() {}

  /**
   * Set all of the attributes for the Neighborhood_Patch
   *
   * @param grd the Neighborhood_Layer to which this patch belongs
   * @param i the row of this Neighborhood_Patch in the Neighborhood_Layer
   * @param j the column of this Neighborhood_Patch in the Neighborhood_Layer
   *
   */
  void setup(Neighborhood_Layer* grd, int i, int j);

  /**
   * Used during debugging to verify that code is functioning properly.
   *
   */
  void quality_control();

  /**
   * Determines distance from this Neighborhood_Patch to another.  Note, that it is distance from the
   * <strong>center</strong> of this Neighborhood_Patch to the <strong>center</strong> of the Neighborhood_Patch in question.
   *
   * @param the patch to check against
   * @return the distance from this patch to the one in question
   */
  double distance_to_patch(Neighborhood_Patch* patch2);

  /**
   * Setup the neighborhood in this Neighborhood_Patch
   */
  void make_neighborhood();

  /**
   * Add household to this Neighborhood_Patch's household vector
   */
  void add_household(Household* p);

  /**
   * Create lists of persons, workplaces, schools (by age)
   */
  void record_daily_activity_locations();

  /**
   * @return a pointer to a random Person in this Neighborhood_Patch
   */
  Person* select_random_person();

  /**
   * @return a pointer to a random Household in this Neighborhood_Patch
   */
  Place* select_random_household();

  /**
   * @return a count of houses in this Neighborhood_Patch
   */
  int get_houses() { 
    return this->households.size();
  }

  /**
   * @return a pointer to this Neighborhood_Patch's Neighborhood
   */
  Place* get_neighborhood() {
    return this->neighborhood;
  }

  int enroll(Person* per) {
    return this->neighborhood->enroll(per);
  }

  /**
   * @return the popsize
   */
  int get_popsize() { 
    return this->popsize;
  }

  double get_mean_household_income() { 
    return this->mean_household_income;
  }

  int get_number_of_households() {
    return static_cast<int>(this->households.size());
  }

  Place* get_household(int i) {
    if(0 <= i && i < get_number_of_households()) {
      return this->households[i];
    } else {
      return NULL;
    }
  }
  
  int get_number_of_schools() {
    return (int) this->schools.size();
  }
  
  Place* get_school(int i) {
    if(0 <= i && i < get_number_of_schools()) {
      return this->schools[i];
    } else {
      return NULL;
    }
  }

  int get_number_of_workplaces() {
    return (int) this->workplaces.size();
  }

  Place* get_workplace(int i) {
    if(0 <= i && i < get_number_of_workplaces()) {
      return this->workplaces[i];
    } else {
      return NULL;
    }
  }

  int get_number_of_hospitals() {
    return (int) this->hospitals.size();
  }

  Place* get_hospital(int i) {
    if(0 <= i && i < get_number_of_hospitals()) {
      return this->hospitals[i];
    } else {
      return NULL;
    }
  }

  void register_place(Place* place);

  void set_vector_control_status(int v_s){this->vector_control_status = v_s;}

  int get_vector_control_status(){return this->vector_control_status;}

protected:
  Neighborhood_Layer* grid;
  Place* neighborhood;
  std::vector<Person*> person;
  int popsize;
  double mean_household_income;
  int vector_control_status;
  long int census_tract_fips;

  // lists of places by type
  place_vector_t households;
  place_vector_t schools;
  place_vector_t workplaces;
  place_vector_t hospitals;
  place_vector_t schools_attended_by_neighborhood_residents;
  place_vector_t schools_attended_by_neighborhood_residents_by_age[Global::GRADES];
  place_vector_t workplaces_attended_by_neighborhood_residents;

};

#endif // _FRED_NEIGHBORHOOD_PATCH_H
