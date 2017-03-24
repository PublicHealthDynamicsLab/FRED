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
// File: Person_Place_Link.h
//
#ifndef _FRED_PERSON_PLACE_LINK_H
#define _FRED_PERSON_PLACE_LINK_H

class Person;
class Place;

class Person_Place_Link {
 public:

  Person_Place_Link();

  ~Person_Place_Link() {}

  void enroll(Person* person, Place* place);

  void unenroll(Person* person);

  void update_enrollee_index(int new_index);

  Place* get_place() {
    return this->place;
  }

  int get_enrollee_index() {
    return this->enrollee_index;
  }

  bool is_enrolled() {
    return this->enrollee_index != -1;
  }

 protected:
  Place* place;
  int enrollee_index;
};

#endif // _FRED_PERSON_PLACE_LINK_H
