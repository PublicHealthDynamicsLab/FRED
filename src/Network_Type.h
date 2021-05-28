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
// File: Network_Type.h
//

#ifndef _FRED_Network_Type_H
#define _FRED_Network_Type_H

#include "Global.h"
#include "Group_Type.h"


namespace Network_Action {
  enum e {NONE, JOIN, ADD_EDGE_TO, ADD_EDGE_FROM, DELETE_EDGE_TO, DELETE_EDGE_FROM, RANDOMIZE, QUIT  };
};

class Network_Type : public Group_Type {
public:

  /**
   * Default constructor
   */
  Network_Type(int _id, string _name);
  ~Network_Type();

  void get_properties();

  void prepare();

  // access methods

  Network* get_network() {
    return this->network;
  }

  bool is_undirected() {
    return this->undirected;
  }

  // static methods

  static void get_network_type_properties();

  static void prepare_network_types();

  static Network_Type* get_network_type(string name) {
    return static_cast<Network_Type*>(Group_Type::get_group_type(name));
  }

  static Network_Type* get_network_type(int type_id) {
    return static_cast<Network_Type*>(Group_Type::get_group_type(type_id));
  }

  static Network_Type* get_network_type_number(int index) {
    if (0 <= index && index < get_number_of_network_types()) {
      return Network_Type::network_types[index];
    }
    else {
      return NULL;
    }
  }

  static Network* get_network_number(int index) {
    if (0 <= index && index < get_number_of_network_types()) {
      return Network_Type::network_types[index]->get_network();
    }
    else {
      return NULL;
    }
  }

  static Network* get_network(int type_id) {
    Network_Type* network_type = Network_Type::get_network_type(type_id);
    if (network_type == NULL) {
      return NULL;
    }
    else {
      return network_type->get_network();
    }
  }

  static int get_network_index(string name) {
    int size = Network_Type::names.size();
    for (int i = 0; i < size; i++) {
      if (name == Network_Type::names[i]) {
	return i;
      }
    }
    return -1;
  }

  static void include_network_type(string name) {
    int size = Network_Type::names.size();
    for (int i = 0; i < size; i++) {
      if (name == Network_Type::names[i]) {
	printf("INCLUDE_NETWORK %s found at network pos %d\n", name.c_str(),i);
	return;
      }
    }
    Network_Type::names.push_back(name);
    printf("INCLUDE_NETWORK %s added as network pos %d\n", name.c_str(),(int)Network_Type::names.size()-1);
  }

  static void exclude_network_type(string name) {
    int size = Network_Type::names.size();
    for (int i = 0; i < size; i++) {
      if (name == Network_Type::names[i]) {
	for (int j = i+1; j < size; j++) {
	  Network_Type::names[j-1] = Network_Type::names[j];	  
	}
	Network_Type::names.pop_back();
      }
    }
  }

  static int get_number_of_network_types() { 
    return Network_Type::network_types.size();
  }

  static void print_network_types(int day);

  static void finish_network_types();

private:

  // index in this vector of network types
  int index;

  // type id
  int id;
  bool undirected;

  // each network type has one network
  Network* network;

  // print interval (days)
  int print_interval;
  int next_print_day;

  // lists of network types
  static std::vector <Network_Type*> network_types;
  static std::vector<std::string> names;
  static int number_of_place_types;
};

#endif // _FRED_Network_Type_H
