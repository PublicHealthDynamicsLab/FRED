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
// File: Mixing_Group.cc
//

#include "Mixing_Group.h"

Mixing_Group::Mixing_Group(const char* lab) {

  strcpy(this->label, lab);

  this->first_day_infectious = -1;
  this->last_day_infectious = -2;
  this->infectious_people = NULL;

  // epidemic counters
  this->new_infections = NULL;
  this->current_infections = NULL;
  this->total_infections = NULL;
  this->new_symptomatic_infections = NULL;
  this->current_symptomatic_infections = NULL;
  this->total_symptomatic_infections = NULL;
  // these counts refer to today's agents
  this->current_infectious_agents = NULL;
  this->current_symptomatic_agents = NULL;

  this->N_orig = 0;             // orig number of enrollees

  // lists of people
  this->enrollees.clear();

  // track whether or not place is infectious with each disease
  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->recovered_bitset.reset();
  this->exposed_bitset.reset();
}

Mixing_Group::~Mixing_Group() {
  if(this->infectious_people != NULL) {
    delete this->infectious_people;
  }
  if(this->infectious_people != NULL) {
    delete this->infectious_people;
  }
  if(this->new_infections != NULL) {
    delete this->new_infections;
  }
  if(this->current_infections != NULL) {
    delete this->current_infections;
  }
  if(this->total_infections != NULL) {
    delete this->total_infections;
  }
  if(this->infectious_people != NULL) {
    delete this->infectious_people;
  }
  if(this->new_symptomatic_infections != NULL) {
    delete this->new_symptomatic_infections;
  }
  if(this->current_symptomatic_infections != NULL) {
    delete this->current_symptomatic_infections;
  }
  if(this->total_symptomatic_infections != NULL) {
    delete this->total_symptomatic_infections;
  }
  if(this->current_infectious_agents != NULL) {
    delete this->current_infectious_agents;
  }
  if(this->current_symptomatic_agents != NULL) {
    delete this->current_symptomatic_agents;
  }
}

int Mixing_Group::get_children() {
  int children = 0;
  for(int i = 0; i < this->enrollees.size(); ++i) {
    children += (this->enrollees[i]->get_age() < Global::ADULT_AGE);
  }
  return children;
}

int Mixing_Group::get_adults() {
  return (this->enrollees.size() - this->get_children());
}

int Mixing_Group::get_recovereds(int disease_id) {
  int count = 0;
  int size = this->enrollees.size();
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    count += person->is_recovered(disease_id);
  }
  return count;
}

void Mixing_Group::add_infectious_person(int disease_id, Person* person) {
  FRED_VERBOSE(0, "ADD_INF: person %d mix_group %s\n", person->get_id(), this->label);
  this->infectious_people[disease_id].push_back(person);
}


void Mixing_Group::record_infectious_days(int day) {
  if(this->first_day_infectious == -1) {
    this->first_day_infectious = day;
  }
  this->last_day_infectious = day;
}
