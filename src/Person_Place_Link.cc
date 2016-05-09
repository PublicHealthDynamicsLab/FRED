/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Person_Place_Link.h"
#include "Place.h"

Person_Place_Link::Person_Place_Link() {
  this->place = NULL;
  this->enrollee_index = -1;
}

void Person_Place_Link::enroll(Person* person, Place* new_place) {
  if(this->place != NULL) {
    FRED_VERBOSE(0,"enroll failed: place %d %s  enrollee_index %d \n",
	   this->place->get_id(), this->place->get_label(), enrollee_index);
  }
  assert(this->place == NULL);
  assert(this->enrollee_index == -1);
  this->place = new_place;
  this->enrollee_index = this->place->enroll(person);
  // printf("ENROLL: place %s size %d\n", place->get_label(), place->get_size()); fflush(stdout);
  assert(this->enrollee_index != -1);
}


void Person_Place_Link::unenroll(Person* person) {
  assert(this->enrollee_index != -1);
  assert(this->place != NULL);
  this->place->unenroll(this->enrollee_index);
  // printf("UNENROLL: place %s size %d\n", place->get_label(), place->get_size()); fflush(stdout);
  this->enrollee_index = -1;
  this->place = NULL;
}
  
void Person_Place_Link::update_enrollee_index(int new_index) {
  assert(this->enrollee_index != -1);
  assert(new_index != -1);
  // printf("update_enrollee_index: old = %d new = %d\n", enrollee_index, new_index); fflush(stdout);
  this->enrollee_index = new_index;
}


