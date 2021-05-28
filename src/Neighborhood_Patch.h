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
#include "Neighborhood_Layer.h"


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

  void setup(Neighborhood_Layer* grd, int i, int j);

  void setup(int phase);
  void prepare();

  void quality_control();

  void make_neighborhood(int type);

  void record_activity_groups();

  Place* select_random_household();

  int get_houses() { 
    return get_number_of_households();
  }

  Place* get_neighborhood() {
    return this->neighborhood;
  }

  int begin_membership(Person* per) {
    return this->neighborhood->begin_membership(per);
  }

  int get_popsize() { 
    return this->popsize;
  }

  int get_number_of_households() {
    return (int) this->places[Place_Type::get_type_id("Household")].size();
  }

  Place* get_household(int i) {
    return get_place(Place_Type::get_type_id("Household"), i);
  }
  
  int get_number_of_schools() {
    return (int) this->places[Place_Type::get_type_id("School")].size();
  }
  
  Place* get_school(int i) {
    return get_place(Place_Type::get_type_id("School"), i);
  }

  int get_number_of_workplaces() {
    return (int) this->places[Place_Type::get_type_id("Workplace")].size();
  }

  Place* get_workplace(int i) {
    if(0 <= i && i < get_number_of_workplaces()) {
      return get_place(Place_Type::get_type_id("Workplace"), i);
    } else {
      return NULL;
    }
  }

  int get_number_of_hospitals() {
    return (int) this->places[Place_Type::get_type_id("Hospital")].size();
  }

  Place* get_hospital(int i) {
    if(0 <= i && i < get_number_of_hospitals()) {
      return get_place(Place_Type::get_type_id("Hospital"), i);
    } else {
      return NULL;
    }
  }


  int get_number_of_places(int type_id) {
    if (0 <= type_id && type_id < Place_Type::get_number_of_place_types()) {
      return (int) this->places[type_id].size();
    }
    else {
      return 0;
    }
  }

  Place* get_place(int type_id, int i) {
    if(0 <= i && i < get_number_of_places(type_id)) {
      return this->places[type_id][i];
    } else {
      return NULL;
    }
  }

  void add_elevation_site(double lat, double lon, double elev) {
    elevation_t *elevation_ptr = new elevation_t;
    elevation_ptr->lat = lat;
    elevation_ptr->lon = lon;
    elevation_ptr->elevation = elev;
    this->elevation_sites.push_back(elevation_ptr);
  }

  void add_elevation_site(elevation_t *esite) {
    this->elevation_sites.push_back(esite);
  }

  double get_elevation(double lat, double lon);

  void add_place(Place* place);

  place_vector_t get_places(int type_id) {
    return places[type_id];
  }
  
  place_vector_t get_places_at_distance(int type_id, int dist);

private:
  Neighborhood_Layer* grid;
  Place* neighborhood;
  person_vector_t person;
  int popsize;
  long long int admin_code;
  std::vector<elevation_t*> elevation_sites;

  // lists of places by type
  place_vector_t schools_attended_by_neighborhood_residents;
  place_vector_t schools_attended_by_neighborhood_residents_by_age[Global::GRADES];
  place_vector_t workplaces_attended_by_neighborhood_residents;
  place_vector_t* places;

};

#endif // _FRED_NEIGHBORHOOD_PATCH_H
