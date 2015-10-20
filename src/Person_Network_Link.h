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
// File: Person_Network_Link.h
//
#ifndef _FRED_PERSON_NETWORK_LINK_H
#define _FRED_PERSON_NETWORK_LINK_H

class Person;
class Network;

class Person_Network_Link {
 public:
  ~Person_Network_Link() {}
  Person_Network_Link(Person* person, Network* network);

  void enroll(Person* person, Network* new_network);
  void unenroll(Person* person);
  void remove_from_network();
  void create_link_from(Person* person);
  void create_link_to(Person* person);
  void destroy_link_from(Person* person);
  void destroy_link_to(Person* person);
  void add_link_to(Person* person);
  void add_link_from(Person* person);
  void delete_link_to(Person* person);
  void delete_link_from(Person* person);
  void print(FILE* fp);
  Network* get_network() {
    return this->network;
  }
  bool is_connected_to(Person* person);
  bool is_connected_from(Person* person);
  int get_out_degree() {
    return this->links_to.size();
  }
  int get_in_degree() {
    return this->links_from.size();
  }
  void clear() {
    this->links_to.clear();
    this->links_from.clear();
  }

 private:
  Person* myself;
  Network* network;
  int enrollee_index;
  std::vector<Person*> links_to;
  std::vector<Person*> links_from;
};

#endif // _FRED_PERSON_NETWORK_LINK_H
