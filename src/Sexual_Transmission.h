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
// File: Sexual_Transmission.h
//

#ifndef _FRED_SEXUAL_TRANSMISSION_H
#define _FRED_SEXUAL_TRANSMISSION_H

#include "Transmission.h"
class Place;
class Disease;
class Person;
class Sexual_Transmission_Network;

class Sexual_Transmission: public Transmission {

public:
  Sexual_Transmission() {} 
  ~Sexual_Transmission() {} 
  static void get_parameters();
  void setup(Disease* disease);
  void spread_infection(int day, int disease_id, Mixing_Group* mixing_group);
  void spread_infection(int day, int disease_id, Sexual_Transmission_Network* sexual_trans_network);

private:
  bool attempt_transmission(Person* infector, Person* infectee, int disease_id, int day, Sexual_Transmission_Network* sexual_trans_network);
};


#endif // _FRED_SEXUAL_TRANSMISSION_H
