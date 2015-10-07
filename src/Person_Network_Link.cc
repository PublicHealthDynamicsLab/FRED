/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Network.h"
#include "Person.h"
#include "Person_Network_Link.h"


Person_Network_Link::Person_Network_Link(Person * person, Network * network) {
  this->place = NULL;
  this->enrollee_index = -1;
  this->myself = person;
  links_to.clear();
  links_from.clear();
  enroll(this->myself, network);
}

void Person_Network_Link::remove_from_network() {
  // remove links to other people
  int size = links_to.size();
  for (int i = 0; i < size; i++) {
    links_to[i]->delete_network_link_from(this->myself, static_cast<Network*>(this->place));
  }

  // remove links from other people
  size = links_from.size();
  for (int i = 0; i < size; i++) {
    links_from[i]->delete_network_link_to(this->myself, static_cast<Network*>(this->place));
  }

  // unenroll in this network
  this->unenroll(this->myself);
}

// these methods should be used to form or destroy new links

void Person_Network_Link::create_link_from(Person* person) {
  add_link_from(person);
  person->add_network_link_to(this->myself, static_cast<Network*>(this->place));
}

void Person_Network_Link::create_link_to(Person* person) {
  add_link_to(person);
  person->add_network_link_from(this->myself, static_cast<Network*>(this->place));
}

void Person_Network_Link::destroy_link_from(Person* person) {
  delete_link_from(person);
  person->delete_network_link_to(this->myself, static_cast<Network*>(this->place));
}

void Person_Network_Link::destroy_link_to(Person* person) {
  delete_link_to(person);
  person->delete_network_link_from(this->myself, static_cast<Network*>(this->place));
}

// helper methods

void Person_Network_Link::add_link_to(Person* person) {
  int size = links_to.size();
  for (int i = 0; i < size; i++) {
    if (person == links_to[i]) {
      return;
    }
  }

  // add person to my links_to list.
  links_to.push_back(person);
}

void Person_Network_Link::add_link_from(Person* person) {
  int size = links_from.size();
  for (int i = 0; i < size; i++) {
    if (person == links_from[i]) {
      return;
    }
  }

  // add person to my links_from list.
  links_from.push_back(person);
}

void Person_Network_Link::delete_link_to(Person* person) {
  // delete person from my links_to list.
  int size = links_to.size();
  for (int i = 0; i < size; i++) {
    if (person == links_to[i]) {
      links_to[i] = links_to.back();
      links_to.pop_back();
    }
  }
}

void Person_Network_Link::delete_link_from(Person* person) {
  // delete person from my links_from list.
  int size = links_from.size();
  for (int i = 0; i < size; i++) {
    if (person == links_from[i]) {
      links_from[i] = links_from.back();
      links_from.pop_back();
    }
  }
}



void Person_Network_Link::print(FILE *fp) {
  fprintf(fp,"%d : ", this->myself->get_id());
  int size = links_to.size();
  for (int i = 0; i < size; i++) {
    fprintf(fp," %d", links_to[i]->get_id());
  }
  fprintf(fp,"\n");
  return;

  fprintf(fp,"%d from : ", this->myself->get_id());
  size = links_from.size();
  for (int i = 0; i < size; i++) {
    fprintf(fp," %d", links_from[i]->get_id());
  }
  fprintf(fp,"\n\n");
}
