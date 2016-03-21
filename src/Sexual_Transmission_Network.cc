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
#include "Place_List.h"
#include "Random.h"
#include "Person.h"
#include "Population.h"

//Private static variables that will be set by parameter lookups
Sexual_Transmission_Network::Sexual_Transmission_Network(const char* lab) : Network(lab) {
  this->set_type(Network::TYPE_NETWORK);
  this->set_subtype(Network::SUBTYPE_SEXUAL_PARTNER);
  this->set_id(Global::Places.get_new_place_id());
  printf("STN: type %c subtype %c id %d\n", get_type(), get_subtype(), get_id());
  this->sexual_contacts_per_day = 0.0;
  this->probability_of_transmission_per_contact = 0.0;
}

void Sexual_Transmission_Network::get_parameters() {
  Params::get_param_from_string("sexual_partner_contacts", &(this->sexual_contacts_per_day));
  Params::get_param_from_string("sexual_trans_per_contact", &(this->probability_of_transmission_per_contact));
}

void Sexual_Transmission_Network::setup() {

  // set optional parameters
  Params::disable_abort_on_failure();
  char sexual_partner_file[FRED_STRING_SIZE];
  strcpy(sexual_partner_file, "");

  Params::get_param_from_string("sexual_partner_file", sexual_partner_file);

  // restore requiring parameters
  Params::set_abort_on_failure();

  if (strcmp(sexual_partner_file,"") != 0) {
    read_sexual_partner_file(sexual_partner_file);
  }
  else {

    // initialize MSM network
    for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
      Person* person = Global::Pop.get_person(p);
      int age = person->get_age();
      char sex = person->get_sex();
      person->become_unsusceptible(this->id);
      if (18 <= age && age < 60 && sex == 'M') {
	if (Random::draw_random() < 0.01) {
	  add_person_to_sexual_partner_network(person);
	}
      }
    }
    
    // create random sexual partnerships
    Global::Sexual_Partner_Network->create_random_network(2.0);
  }
  print();
}

void Sexual_Transmission_Network::read_sexual_partner_file(char* sexual_partner_file) {
  int id1, id2;
  FILE* fp = Utils::fred_open_file(sexual_partner_file);
  while(fscanf(fp, "%d %d", &id1, &id2) == 2) {
    Person* partner1 = Global::Pop.get_person(id1);
    add_person_to_sexual_partner_network(partner1);

    Person* partner2 = Global::Pop.get_person(id2);
    add_person_to_sexual_partner_network(partner2);

    if(partner1->is_connected_to(partner2, this) == false) {
      partner1->create_network_link_to(partner2, this);
    }

    if(partner2->is_connected_to(partner1, this) == false) {
      partner2->create_network_link_to(partner1, this);
    }
  }
  fclose(fp);
}


void Sexual_Transmission_Network::add_person_to_sexual_partner_network(Person* person) {
  if (person->is_enrolled_in_network(this)==false) {
    person->join_network(Global::Sexual_Partner_Network);
    person->clear_network(this);
  }
}


