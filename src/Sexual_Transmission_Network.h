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
// File: Sexual_Transmission_Network.h
//

#ifndef _FRED_SEXUAL_TRANSMISSION_NETWORK_H
#define _FRED_SEXUAL_TRANSMISSION_NETWORK_H

#include "Network.h"
class Condition;

class Sexual_Transmission_Network : public Network {
public: 
  Sexual_Transmission_Network(const char* lab);
  ~Sexual_Transmission_Network() {}

  void get_parameters();

  double get_sexual_contacts_per_day() {
    return this->sexual_contacts_per_day;
  }

  double get_sexual_transmission_per_contact() {
    return this->probability_of_transmission_per_contact;
  }

  void setup();

  void read_sexual_partner_file(char* sexual_partner_file);

private:
  int id;
  double sexual_contacts_per_day;
  double probability_of_transmission_per_contact;
};

#endif // _FRED_SEXUAL_TRANSMISSION_NETWORK_H

