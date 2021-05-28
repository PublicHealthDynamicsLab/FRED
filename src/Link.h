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
// File: Link.h
//
#ifndef _FRED_LINK_H
#define _FRED_LINK_H

#include "Global.h"

class Group;
class Network;
class Person;
class Place;


class Link {
 public:

  Link();
  ~Link() {}

  void begin_membership(Person* person, Group* new_group);
  void end_membership(Person* person);
  void remove_from_network(Person* person);
  void add_edge_to(Person* other_person);
  void add_edge_from(Person* other_person);
  void delete_edge_to(Person* other_person);
  void delete_edge_from(Person* other_person);
  void print(FILE* fp);
  Group* get_group() {
    return this->group;
  }
  Network* get_network();
  Place* get_place();
  int get_member_index() {
    return this->member_index;
  }
  bool is_member() {
    return this->member_index != -1;
  }
  bool is_connected_to(Person* other_person);
  bool is_connected_from(Person* other_person);
  int get_out_degree() {
    return this->outward_edge.size();
  }
  int get_in_degree() {
    return this->inward_edge.size();
  }
  void clear() {
    this->inward_edge.clear();
    this->outward_edge.clear();
    this->inward_timestamp.clear();
    this->outward_timestamp.clear();
    this->inward_weight.clear();
    this->outward_weight.clear();
  }

  Person * get_inward_edge(int n) {
    return this->inward_edge[n];
  }
  Person * get_outward_edge(int n) {
    return this->outward_edge[n];
  }

  person_vector_t get_outward_edges() {
    return this->outward_edge;
  }
  person_vector_t get_inward_edges() {
    return this->inward_edge;
  }

  void set_weight_to(Person* other_person, double value);
  void set_weight_from(Person* other_person, double value);

  double get_weight_to(Person* other_person);
  double get_weight_from(Person* other_person);

  int get_timestamp_to(Person* other_person);
  int get_timestamp_from(Person* other_person);

  int get_id_of_last_inward_edge();
  int get_id_of_last_outward_edge();

  int get_timestamp_of_last_inward_edge();
  int get_timestamp_of_last_outward_edge();

  int get_id_of_max_weight_inward_edge();
  int get_id_of_max_weight_outward_edge();

  int get_id_of_min_weight_inward_edge();
  int get_id_of_min_weight_outward_edge();

  void update_member_index(int new_index);
  void link(Person* person, Group* new_group);
  void unlink(Person* person);

 private:
  Group* group;
  int member_index;

  person_vector_t inward_edge;
  person_vector_t outward_edge;

  int_vector_t inward_timestamp;
  int_vector_t outward_timestamp;

  double_vector_t inward_weight;
  double_vector_t outward_weight;

};

#endif // _FRED_LINK_H

