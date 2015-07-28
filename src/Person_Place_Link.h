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
// File: Person_Place_Link.h
//
#ifndef _FRED_PERSON_PLACE_LINK_H
#define _FRED_PERSON_PLACE_LINK_H

#include "Global.h"
#include "Place.h"

class Person_Place_Link {
 public:

  Person_Place_Link() {
    enrollee_index = -1;
    for (int d = 0; d < Global::MAX_NUM_DISEASES; d++) {
      infectious_enrollee_index[d] = -1;
    }
  }

  ~Person_Place_Link() {}

  void enroll(Person *person, Place *place) {
    if (Global::Test) {
      assert(enrollee_index == -1);
      assert(place != NULL);
      enrollee_index = place->enroll_with_link(person);
      assert(enrollee_index != -1);
    }
    else {
      assert(place != NULL);
      place->enroll(person);
    }
  }

  void unenroll(Person *person, Place *place) {
    if (Global::Test) {
      assert(enrollee_index != -1);
      assert(place != NULL);
      place->unenroll(enrollee_index);
      enrollee_index = -1;
      for (int d = 0; d < Global::MAX_NUM_DISEASES; d++) {
	if (infectious_enrollee_index[d] != -1) {
	  unenroll_infectious_person(person, place, d);
	}
      }
    }
    else {
      assert(place != NULL);
      place->unenroll(person);
    }
  }

  void update_enrollee_index(int new_index) {
    assert(enrollee_index != -1);
    assert(new_index != -1);
    enrollee_index = new_index;
  }

  void enroll_infectious_person(Person *person, Place *place, int disease_id) {
    assert(infectious_enrollee_index[disease_id] == -1);
    assert(place != NULL);
    infectious_enrollee_index[disease_id] = place->enroll_infectious_person(disease_id, person);
    assert(infectious_enrollee_index[disease_id] != -1);
  }

  void unenroll_infectious_person(Person *person, Place *place, int disease_id) {
    assert(infectious_enrollee_index[disease_id] != -1);
    assert(place != NULL);
    place->unenroll_infectious_person(disease_id, infectious_enrollee_index[disease_id]);
    infectious_enrollee_index[disease_id] = -1;
  }

  void update_infectious_enrollee_index(int disease_id, int new_index) {
    assert(infectious_enrollee_index[disease_id] != -1);
    assert(new_index != -1);
    infectious_enrollee_index[disease_id] = new_index;
  }

  bool is_enrolled() {
    return enrollee_index != -1;
  }

  bool is_enrolled_as_infectious(int disease_id) {
    return infectious_enrollee_index[disease_id] != -1;
  }

 private:
  int disease_id;
  int enrollee_index;
  int infectious_enrollee_index[Global::MAX_NUM_DISEASES];
};

#endif // _FRED_PERSON_PLACE_LINK_H
