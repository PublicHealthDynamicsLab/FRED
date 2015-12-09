/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
// File: Drug_Use_Infection.h
//
#ifndef _FRED_DRUG_USE_INFECTION_H
#define _FRED_DRUG_USE_INFECTION_H

#include "Infection.h"

class Disease;
class Person;
class Mixing_Group;

class Drug_Use_Infection : public Infection {

 public:

  // if primary infection, infector and place are null.
  Drug_Use_Infection(Disease* disease, Person* infector, Person* host, Mixing_Group* mixing_group, int day);

  ~Drug_Use_Infection() {}

  void update(int day);

  void setup();

  double get_infectivity(int day);

  double get_symptoms(int day);

  bool is_fatal(int day);

 private:

};

#endif // _FRED_DRUG_USE_INFECTION_H

