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
// File: Vector_Transmission.h
//

#ifndef _FRED_VECTOR_TRANSMISSION_H
#define _FRED_VECTOR_TRANSMISSION_H

#include "Global.h"
#include "Transmission.h"

class Condition;
class Mixing_Group;
class Person;
class Place;

class Vector_Transmission: public Transmission {

public:
  Vector_Transmission() {}
  ~Vector_Transmission() {}
  void setup(Condition* condition);
  void spread_infection(int day, int condition_id, Mixing_Group* mixing_group);
  void spread_infection(int day, int condition_id, Place* place);

private:
  bool attempt_transmission(double transmission_prob, Person* infector, Person* infectee, int condition_id, int day, Place* place);
  void infect_vectors(int day, Place* place);
  void infect_hosts(int day, int condition_id, Place* place);
  std::vector<Person*> visitors;
  std::vector<Person*> susceptible_visitors;
};

#endif // _FRED_VECTOR_TRANSMISSION_H
