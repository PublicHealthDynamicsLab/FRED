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
// File: Sexual_Transmission_Network.cc
//

#include "Sexual_Transmission_Network.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"

#include "Population.h" // for test only

//Private static variables that will be set by parameter lookups
double Sexual_Transmission_Network::sexual_contacts_per_day = 0.0;
double Sexual_Transmission_Network::sexual_transmission_per_contact = 0.0;

Sexual_Transmission_Network::Sexual_Transmission_Network(const char* lab) : Network(lab) {
  this->set_subtype(Network::SUBTYPE_SEXUAL_PARTNER);
}

void Sexual_Transmission_Network::get_parameters() {
  Params::get_param_from_string("sexual_partner_contacts", &Sexual_Transmission_Network::sexual_contacts_per_day);
  Params::get_param_from_string("sexual_trans_per_contact", &Sexual_Transmission_Network::sexual_transmission_per_contact);
}
