/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Disease_List.h"
#include "Global.h"
#include "Person_Place_Link.h"
#include "Place.h"

Person_Place_Link::Person_Place_Link() {
  this->place = NULL;
  this->enrollee_index = -1;
  int diseases = Global::Diseases.get_number_of_diseases();
  this->infectious_enrollee_index = new int [diseases];
  for (int d = 0; d < diseases; d++) {
    this->infectious_enrollee_index[d] = -1;
  }
}

void Person_Place_Link::enroll(Person *person, Place *new_place) {
  if (this->place != NULL) {
    FRED_VERBOSE(0,"enroll failed: place %d %s  enrollee_index %d \n",
	   this->place->get_id(), this->place->get_label(), enrollee_index);
  }
  assert(this->place == NULL);
  assert(this->enrollee_index == -1);
  this->place = new_place;
  enrollee_index = this->place->enroll_with_link(person);
  assert(this->enrollee_index != -1);
}


void Person_Place_Link::unenroll(Person *person) {
  assert(enrollee_index != -1);
  assert(place != NULL);
  int diseases = Global::Diseases.get_number_of_diseases();
  for (int d = 0; d < diseases; d++) {
    if (infectious_enrollee_index[d] != -1) {
      unenroll_infectious_person(person, d);
    }
  }
  place->unenroll(enrollee_index);
  enrollee_index = -1;
  place = NULL;
}
  
void Person_Place_Link::update_enrollee_index(int new_index) {
  assert(enrollee_index != -1);
  assert(new_index != -1);
  // printf("update_enrollee_index: old = %d new = %d\n", enrollee_index, new_index); fflush(stdout);
  enrollee_index = new_index;
}

void Person_Place_Link::enroll_infectious_person(Person *person, int disease_id) {
  return;
  assert(infectious_enrollee_index[disease_id] == -1);
  assert(place != NULL);
  infectious_enrollee_index[disease_id] = place->enroll_infectious_person(disease_id, person);
  assert(infectious_enrollee_index[disease_id] != -1);
}

void Person_Place_Link::unenroll_infectious_person(Person *person, int disease_id) {
  return;
  assert(infectious_enrollee_index[disease_id] != -1);
  assert(place != NULL);
  place->unenroll_infectious_person(disease_id, infectious_enrollee_index[disease_id]);
  infectious_enrollee_index[disease_id] = -1;
}

void Person_Place_Link::update_infectious_enrollee_index(int disease_id, int new_index) {
  return;
  assert(infectious_enrollee_index[disease_id] != -1);
  assert(new_index != -1);
  infectious_enrollee_index[disease_id] = new_index;
}

