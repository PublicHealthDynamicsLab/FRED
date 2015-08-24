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

class Sexual_Transmission: public Transmission {

public:
  Sexual_Transmission() {} 
  ~Sexual_Transmission() {} 
  void get_parameters() {}
  void setup(Disease *disease) {}
  void spread_infection(int day, int disease_id, Place *place) {}

private:

};


#endif // _FRED_SEXUAL_TRANSMISSION_H
