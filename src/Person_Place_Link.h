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

class Person;
class Place;

class Person_Place_Link {
 public:

  Person_Place_Link();

  ~Person_Place_Link() {
    delete[] infectious_enrollee_index;
  }

  void enroll(Person *person, Place *place);

  void unenroll(Person *person);

  void update_enrollee_index(int new_index);

  void enroll_infectious_person(Person *person, int disease_id);

  void unenroll_infectious_person(Person *person, int disease_id);

  void update_infectious_enrollee_index(int disease_id, int new_index);

  Place * get_place() {
    return place;
  }

  int get_enrollee_index() {
    return enrollee_index;
  }

  bool is_enrolled() {
    return enrollee_index != -1;
  }

  bool is_enrolled_as_infectious(int disease_id) {
    return infectious_enrollee_index[disease_id] != -1;
  }

 private:
  Place * place;
  int enrollee_index;
  int * infectious_enrollee_index;
};

#endif // _FRED_PERSON_PLACE_LINK_H
