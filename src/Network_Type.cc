/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Network_Type.cc
//


#include "Property.h"
#include "Network.h"
#include "Network_Type.h"
#include "Place_Type.h"
#include "Utils.h"


//////////////////////////
//
// STATIC VARIABLES
//
//////////////////////////

std::vector <Network_Type*> Network_Type::network_types;
std::vector<std::string> Network_Type::names;
int Network_Type::number_of_place_types = 0;


Network_Type::Network_Type(int type_id, string _name) : Group_Type(_name) {
  // generate one network for each network_type
  this->id = type_id;
  this->network = new Network(_name.c_str(), type_id, this);
  this->print_interval = 0;
  this->next_print_day = 999999;
  Group_Type::add_group_type(this);
}

Network_Type::~Network_Type() {
}


void Network_Type::prepare() {

  FRED_STATUS(0, "network_type %s type_id %d prepare entered\n", this->name.c_str(), this->id);

  if (this->has_admin) {
    this->network->create_administrator();
  }

  get_network()->read_edges();

  FRED_STATUS(0, "network_type %s prepare finished\n", this->name.c_str());
}

//////////////////////////
//
// STATIC METHODS
//
//////////////////////////

void Network_Type::get_network_type_properties() {

  Network_Type::network_types.clear();

  Network_Type::number_of_place_types = Place_Type::get_number_of_place_types();

  for(int index = 0; index < Network_Type::names.size(); ++index) {

    string name = Network_Type::names[index];

    int type_id = index + Network_Type::number_of_place_types;

    // create new Network_Type object
    Network_Type * network_type = new Network_Type(type_id, name);

    // add to network_type list
    Network_Type::network_types.push_back(network_type);

    printf("CREATED_NETWORK_TYPE index %d type_id %d = %s\n", index, type_id, name.c_str());

    // get its properties
    network_type->get_properties();
  }

}

void Network_Type::get_properties() {

  // first get the base class properties
  Group_Type::get_properties();

  char property_name[FRED_STRING_SIZE];

  FRED_STATUS(0, "network_type %s read_properties entered\n", this->name.c_str());
  
  // optional properties:
  Property::disable_abort_on_failure();

  sprintf(property_name, "%s.is_undirected", this->name.c_str());
  int n = 0;
  Property::get_property(property_name, &n);
  this->undirected = n;

  sprintf(property_name, "%s.print_interval", this->name.c_str());
  Property::get_property(property_name, &this->print_interval);
  if (this->print_interval > 0) {
    this->next_print_day = 0;
  }

  Property::set_abort_on_failure();

  FRED_STATUS(0, "network_type %s read_properties finished\n", this->name.c_str());
}


void Network_Type::prepare_network_types() {
  int n = Network_Type::get_number_of_network_types();
  FRED_STATUS(0, "prepare_network_type entered types = %d\n", n);
  for(int index = 0; index < n; ++index) {
    Network_Type::network_types[index]->prepare(); 
  }
}

void Network_Type::print_network_types(int day) {
  for(int index = 0; index < Network_Type::get_number_of_network_types(); ++index) {
    Network_Type* net_type = Network_Type::network_types[index];
    if (net_type->next_print_day <= day) {
      net_type->get_network()->print();
      net_type->next_print_day += net_type->print_interval;
    }
  }
}

void Network_Type::finish_network_types() {
  for(int index = 0; index < Network_Type::get_number_of_network_types(); ++index) {
    if (Network_Type::network_types[index]->print_interval > 0) {
      Network_Type::network_types[index]->get_network()->print();
    }
  }
}


