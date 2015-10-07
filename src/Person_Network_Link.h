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

class Person_Network_Link: public Person_Place_Link {
 public:
  Person_Network_Link();
  ~Person_Network_Link() {}
  Person_Network_Link(Person * person, Network * network);
  void remove_from_network();
  void create_link_from(Person* person);
  void create_link_to(Person* person);
  void destroy_link_from(Person* person);
  void destroy_link_to(Person* person);
  void add_link_to(Person* person);
  void add_link_from(Person* person);
  void delete_link_to(Person* person);
  void delete_link_from(Person* person);
  void print(FILE *fp);

 private:
  Person * myself;
  std::vector<Person*> links_to;
  std::vector<Person*> links_from;
};

#endif // _FRED_PERSON_NETWORK_LINK_H
