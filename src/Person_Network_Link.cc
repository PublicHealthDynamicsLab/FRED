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

Person_Network_Link::Person_Network_Link(Person* person, Network* network) {
  this->myself = person;
  this->network = NULL;
  this->enrollee_index = -1;
  this->links_to.clear();
  this->links_from.clear();
  this->enroll(this->myself, network);
}

void Person_Network_Link::enroll(Person* person, Network* new_network) {
  if(this->network != NULL) {
    FRED_VERBOSE(0,"enroll failed: network %s  enrollee_index %d \n",
      this->network->get_label(), enrollee_index);
  }
  assert(this->network == NULL);
  assert(this->enrollee_index == -1);
  this->network = new_network;
  this->enrollee_index = this->network->enroll(person);
  assert(this->enrollee_index != -1);
}

void Person_Network_Link::remove_from_network() {
  // remove links to other people
  int size = this->links_to.size();
  for(int i = 0; i < size; ++i) {
    this->links_to[i]->delete_network_link_from(this->myself, this->network);
  }

  // remove links from other people
  size = this->links_from.size();
  for(int i = 0; i < size; ++i) {
    this->links_from[i]->delete_network_link_to(this->myself, this->network);
  }

  // unenroll in this network
  this->unenroll(this->myself);
}

void Person_Network_Link::unenroll(Person* person) {
  assert(this->enrollee_index != -1);
  assert(this->network != NULL);
  this->network->unenroll(this->enrollee_index);
  this->enrollee_index = -1;
  this->network = NULL;
}

// these methods should be used to form or destroy new links

void Person_Network_Link::create_link_from(Person* person) {
  add_link_from(person);
  person->add_network_link_to(this->myself, this->network);
}

void Person_Network_Link::create_link_to(Person* person) {
  add_link_to(person);
  person->add_network_link_from(this->myself, this->network);
}

void Person_Network_Link::destroy_link_from(Person* person) {
  delete_link_from(person);
  person->delete_network_link_to(this->myself, this->network);
}

void Person_Network_Link::destroy_link_to(Person* person) {
  delete_link_to(person);
  person->delete_network_link_from(this->myself, this->network);
}

// helper methods

void Person_Network_Link::add_link_to(Person* person) {
  int size = this->links_to.size();
  for(int i = 0; i < size; ++i) {
    if(person ==  this->links_to[i]) {
      return;
    }
  }

  // add person to my links_to list.
  this->links_to.push_back(person);
}

void Person_Network_Link::add_link_from(Person* person) {
  int size =  this->links_from.size();
  for(int i = 0; i < size; ++i) {
    if(person ==  this->links_from[i]) {
      return;
    }
  }

  // add person to my links_from list.
  this->links_from.push_back(person);
}

void Person_Network_Link::delete_link_to(Person* person) {
  // delete person from my links_to list.
  int size =  this->links_to.size();
  for(int i = 0; i < size; ++i) {
    if(person ==  this->links_to[i]) {
      this->links_to[i] =  this->links_to.back();
      this->links_to.pop_back();
    }
  }
}

void Person_Network_Link::delete_link_from(Person* person) {
  // delete person from my links_from list.
  int size =  this->links_from.size();
  for(int i = 0; i < size; i++) {
    if(person ==  this->links_from[i]) {
      this->links_from[i] =  this->links_from.back();
      this->links_from.pop_back();
    }
  }
}



void Person_Network_Link::print(FILE *fp) {
  fprintf(fp,"%d ->", this->myself->get_id());
  int size = this->links_to.size();
  for (int i = 0; i < size; ++i) {
    fprintf(fp," %d", this->links_to[i]->get_id());
  }
  fprintf(fp,"\n");
  return;

  size = links_from.size();
  for(int i = 0; i < size; ++i) {
    fprintf(fp,"%d ", this->links_from[i]->get_id());
  }
  fprintf(fp,"-> %d\n\n", this->myself->get_id());
}

bool Person_Network_Link::is_connected_to(Person* person) {
  int size = this->links_to.size();
  for(int i = 0; i < size; ++i) {
    if(this->links_to[i] == person) {
      return true;
    }
  }
  return false;
}

bool Person_Network_Link::is_connected_from(Person* person) {
  int size = this->links_from.size();
  for(int i = 0; i < size; ++i) {
    if(this->links_from[i] == person) {
      return true;
    }
  }
  return false;
}

