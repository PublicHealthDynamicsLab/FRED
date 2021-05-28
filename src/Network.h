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
// File: Network.h
//

#ifndef _FRED_NETWORK_H
#define _FRED_NETWORK_H

#include "Global.h"
#include "Group.h"

class Condition;
class Preference;
class Network_Type;

class Network : public Group {
public: 

  Network(const char* lab, int _type_id, Network_Type* net_type);
  ~Network() {}

  void get_properties();
  double get_transmission_prob(int condition_id, Person* i, Person* s);

  void print();

  bool is_connected_to(Person* p1, Person* p2);
  bool is_connected_from(Person* p1, Person* p2);
  double get_mean_degree();
  void test();
  void randomize(double mean_degree, int max_degree);
  void infect_random_nodes(double pct, Condition* condition);

  bool is_undirected();
  void read_edges();

  Network_Type* get_network_type() {
    return this->network_type;
  }
  int get_time_block(int day, int hour);

  void add_rule(Rule* rule);
  // bool add_pool_str(string str);
  bool is_qualified(Person* person, Person* other);

  const char* get_name();

  void print_person(Person* person, FILE* fp);

  static Network* get_network(string name);


protected:
  pair_vector_t edge;
  Network_Type* network_type;
  // string_vector_t pool_str;
  // int_vector_t pool;
  // clause_vector_t requirements;
  // int_vector_t restriction;
  // int_vector_t max_dist;
};

#endif // _FRED_NETWORK_H

