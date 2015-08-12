/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
// File: HIV_Infection.h
//
#ifndef _FRED_HIV_INFECTION_H
#define _FRED_HIV_INFECTION_H

#include "Infection.h"

class Disease;
class Person;
class Place;

class HIV_Infection : public Infection {

 public:

  // if primary infection, infector and place are null.
  HIV_Infection(Disease* disease, Person* infector, Person* host, Place* place, int day);

  HIV_Infection() {}

  ~HIV_Infection() {}

  void update(int day) {}

  void setup();

  double get_infectivity(int day);

  double get_symptoms(int day);

  bool is_fatal(int day);

 private:

};

#endif // _FRED_INFECTION_H

