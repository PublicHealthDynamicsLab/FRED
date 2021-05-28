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

#include "Link.h"
#include "Group.h"
#include "Network.h"
#include "Person.h"
#include "Place.h"

Link::Link() {
  this->group = NULL;
  this->member_index = -1;
  this->inward_edge.clear();
  this->outward_edge.clear();
  this->inward_timestamp.clear();
  this->outward_timestamp.clear();
  this->inward_weight.clear();
  this->outward_weight.clear();
}

void Link::begin_membership(Person* person, Group* new_group) {
  // FRED_VERBOSE(0, "begin_membership in group %s\n", new_group? new_group->get_label() : "NULL");
  if (is_member()) {
    return;
  }
  this->group = new_group;
  this->member_index = this->group->begin_membership(person);
  // FRED_VERBOSE(0, "finish begin_membership in group %s index %d\n", this->group? this->group->get_label() : "NULL", this->member_index);
}

void Link::remove_from_network(Person* person) {
  // remove edges to other people
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    this->outward_edge[i]->delete_edge_from(person, get_network());
  }

  // remove edges from other people
  size = this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    this->inward_edge[i]->delete_edge_to(person, get_network());
  }

  // end_membership in this network
  this->end_membership(person);
}

void Link::end_membership(Person* person) {
  if (this->group) {
    this->group->end_membership(this->member_index);
    this->group = NULL;
  }
  this->member_index = -1;
}

Network* Link::get_network() {
  return static_cast<Network*>(this->group);
}
 
Place* Link::get_place() {
  return static_cast<Place*>(this->group);
}

// these methods should be used to add or delete edges

void Link::add_edge_to(Person* other_person) {
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(other_person == this->outward_edge[i]) {
      return;
    }
  }

  // add other_person to my outward_edge list.
  this->outward_edge.push_back(other_person);
  this->outward_timestamp.push_back(Global::Simulation_Step);
  this->outward_weight.push_back(1.0);
}

void Link::add_edge_from(Person* other_person) {
  int size =  this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(other_person == this->inward_edge[i]) {
      return;
    }
  }

  // add other_person to my inward_edge list.
  this->inward_edge.push_back(other_person);
  this->inward_timestamp.push_back(Global::Simulation_Step);
  this->inward_weight.push_back(1.0);
}

void Link::delete_edge_to(Person* other_person) {
  // delete other_person from my outward_edge list.
  int size =  this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(other_person == this->outward_edge[i]) {
      this->outward_edge[i] =  this->outward_edge.back();
      this->outward_edge.pop_back();
      this->outward_timestamp[i] =  this->outward_timestamp.back();
      this->outward_timestamp.pop_back();
      this->outward_weight[i] =  this->outward_weight.back();
      this->outward_weight.pop_back();
    }
  }
}

void Link::delete_edge_from(Person* other_person) {
  // delete other_person from my inward_edge list.
  int size =  this->inward_edge.size();
  // FRED_VERBOSE(0, "Link::delete_edge_from person %d size %d\n", other_person->get_id(),size);
  for(int i = 0; i < size; i++) {
    if(other_person == this->inward_edge[i]) {
      this->inward_edge[i] =  this->inward_edge.back();
      this->inward_edge.pop_back();
      this->inward_timestamp[i] =  this->inward_timestamp.back();
      this->inward_timestamp.pop_back();
      this->inward_weight[i] =  this->inward_weight.back();
      this->inward_weight.pop_back();
    }
  }
  // FRED_VERBOSE(0, "Link::delete_edge_from finished person %d size %d\n", other_person->get_id(),size);
}



void Link::print(FILE *fp) {
  /*
  fprintf(fp,"%d ->", this->myself->get_id());
  int size = this->outward_edge.size();
  for (int i = 0; i < size; ++i) {
    fprintf(fp," %d", this->outward_edge[i]->get_id());
  }
  fprintf(fp,"\n");
  return;

  size = inward_edge.size();
  for(int i = 0; i < size; ++i) {
    fprintf(fp,"%d ", this->inward_edge[i]->get_id());
  }
  fprintf(fp,"-> %d\n\n", this->myself->get_id());
  */
}

bool Link::is_connected_to(Person* other_person) {
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->outward_edge[i] == other_person) {
      return true;
    }
  }
  return false;
}

bool Link::is_connected_from(Person* other_person) {
  int size = this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->inward_edge[i] == other_person) {
      return true;
    }
  }
  return false;
}

void Link::update_member_index(int new_index) {
  assert(this->member_index != -1);
  assert(new_index != -1);
  this->member_index = new_index;
}

void Link::set_weight_to(Person* other_person, double value) {
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->outward_edge[i] == other_person) {
      this->outward_weight[i] = value;
      return;
    }
  }
  return;
}

double Link::get_weight_to(Person* other_person) {
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->outward_edge[i] == other_person) {
      return this->outward_weight[i];
    }
  }
  return 0.0;
}

void Link::set_weight_from(Person* other_person, double value) {
  int size = this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->inward_edge[i] == other_person) {
      this->inward_weight[i] = value;
      return;
    }
  }
  return;
}

double Link::get_weight_from(Person* other_person) {
  int size = this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->inward_edge[i] == other_person) {
      return this->inward_weight[i];
    }
  }
  return 0.0;
}

int Link::get_timestamp_to(Person* other_person) {
  int size = this->outward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->outward_edge[i] == other_person) {
      return this->outward_timestamp[i];
    }
  }
  return -1;
}

int Link::get_timestamp_from(Person* other_person) {
  int size = this->inward_edge.size();
  for(int i = 0; i < size; ++i) {
    if(this->inward_edge[i] == other_person) {
      return this->inward_timestamp[i];
    }
  }
  return -1;
}

int Link::get_id_of_last_outward_edge() {
  int size = this->outward_edge.size();
  int max_time = -9999999;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (max_time < this->outward_timestamp[i]) {
      max_time = this->outward_timestamp[i];
      pos = i;
    }
  }
  if (0 <= pos) {
    return this->outward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_id_of_last_inward_edge() {
  int size = this->inward_edge.size();
  int max_time = -9999999;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (max_time < this->inward_timestamp[i]) {
      max_time = this->inward_timestamp[i];
      pos = i;
    }
  }
  if (0 <= pos) {
    return this->inward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_id_of_max_weight_outward_edge() {
  int size = this->outward_edge.size();
  double max_weight;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (i==0) {
      max_weight = this->outward_weight[i];
      pos = i;
    }
    else {
      if (max_weight < this->outward_weight[i]) {
	max_weight = this->outward_weight[i];
	pos = i;
      }
    }
  }
  if (0 <= pos) {
    return this->outward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_id_of_max_weight_inward_edge() {
  int size = this->inward_edge.size();
  double max_weight;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (i==0) {
      max_weight = this->inward_weight[i];
      pos = i;
    }
    else {
      if (max_weight < this->inward_weight[i]) {
	max_weight = this->inward_weight[i];
	pos = i;
      }
    }
  }
  if (0 <= pos) {
    return this->inward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_id_of_min_weight_outward_edge() {
  int size = this->outward_edge.size();
  double min_weight;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (i==0) {
      min_weight = this->outward_weight[i];
      pos = i;
    }
    else {
      if (min_weight > this->outward_weight[i]) {
	min_weight = this->outward_weight[i];
	pos = i;
      }
    }
  }
  if (0 <= pos) {
    return this->outward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_id_of_min_weight_inward_edge() {
  int size = this->inward_edge.size();
  double min_weight;
  int pos = -1;
  for(int i = 0; i < size; ++i) {
    if (i==0) {
      min_weight = this->inward_weight[i];
      pos = i;
    }
    else {
      if (min_weight > this->inward_weight[i]) {
	min_weight = this->inward_weight[i];
	pos = i;
      }
    }
  }
  if (0 <= pos) {
    return this->inward_edge[pos]->get_id();
  }
  else {
    return -99999999;
  }
}

int Link::get_timestamp_of_last_inward_edge() {
  int size = this->inward_edge.size();
  int max_time = -1;
  for(int i = 0; i < size; ++i) {
    if (max_time < this->inward_timestamp[i]) {
      max_time = this->inward_timestamp[i];
    }
  }
  return max_time;
}

int Link::get_timestamp_of_last_outward_edge() {
  int size = this->outward_edge.size();
  int max_time = -1;
  for(int i = 0; i < size; ++i) {
    if (max_time < this->outward_timestamp[i]) {
      max_time = this->outward_timestamp[i];
    }
  }
  return max_time;
}


////////////////////////////////////////////


void Link::link(Person* person, Group* new_group) {
  this->group = new_group;
  // printf("LINK: group %s size %d\n", this->group->get_label(), this->group->get_size()); fflush(stdout);
}

void Link::unlink(Person* person) {
  this->member_index = -1;
  this->group = NULL;
  // printf("UNLINK: group %s size %d\n", this->group->get_label(), this->group->get_size()); fflush(stdout);
}
  
