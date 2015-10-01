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

#define GRADES 20

class Neighborhood_Patch : public Abstract_Patch {
public:

  /**
   * Default constructor
   */
  Neighborhood_Patch() {}
  ~Neighborhood_Patch() {}

  /**
   * Set all of the attributes for the Neighborhood_Patch
   *
   * @param grd the Neighborhood_Layer to which this patch belongs
   * @param i the row of this Neighborhood_Patch in the Neighborhood_Layer
   * @param j the column of this Neighborhood_Patch in the Neighborhood_Layer
   *
   */
  void setup(Neighborhood_Layer * grd, int i, int j);

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
  double distance_to_patch(Neighborhood_Patch * patch2);

  /**
   * Setup the neighborhood in this Neighborhood_Patch
   */
  void make_neighborhood(Place::Allocator<Neighborhood> & neighborhood_allocator);

  /**
   * Add household to this Neighborhood_Patch's household vector
   */
  void add_household(Household *p);

  /**
   * Create lists of persons, workplaces, schools (by age)
   */
  void record_daily_activity_locations();

  /**
   * @return a pointer to a random Person in this Neighborhood_Patch
   */
  Person * select_random_person();

  /**
   * @return a pointer to a random Household in this Neighborhood_Patch
   */
  Place * select_random_household();

  /**
   * @return a pointer to a random Workplace in this Neighborhood_Patch
   */
  Place * select_random_workplace();
  Place * select_workplace();
  Place * select_workplace_in_neighborhood();

  Place * select_random_school(int age);
  Place * select_school(int age);
  Place * select_school_in_neighborhood(int age, double threshold);
  void find_schools_for_age(int age, place_vector * schools);

  /**
   * @return a count of houses in this Neighborhood_Patch
   */
  int get_houses() { 
    return households.size();
  }

  /**
   * @return a pointer to this Neighborhood_Patch's Neighborhood
   */
  Place * get_neighborhood() { 
    return neighborhood; 
  }

  int enroll(Person * per) { 
    return neighborhood->enroll(per); 
  }

  /**
   * @return the popsize
   */
  int get_popsize() { 
    return popsize; 
  }

  double get_mean_household_income() { 
    return mean_household_income; 
  }

  int get_number_of_households() {
    return (int) this->households.size();
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

protected:
  Neighborhood_Layer * grid;
  Place * neighborhood;
  std::vector<Person *> person;
  int popsize;
  double mean_household_income;

  // lists of places by type
  place_vector_t households;
  place_vector_t schools;
  place_vector_t workplaces;
  place_vector_t hospitals;
  place_vector_t schools_attended_by_neighborhood_residents;
  place_vector_t schools_attended_by_neighborhood_residents_by_age[GRADES];
  place_vector_t workplaces_attended_by_neighborhood_residents;

};

#endif // _FRED_NEIGHBORHOOD_PATCH_H
