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

char Mixing_Group::TYPE_UNSET = 'U';

char Mixing_Group::SUBTYPE_NONE = 'X';

Mixing_Group::Mixing_Group(const char* lab) {

  this->id = -1;
  this->type = Mixing_Group::TYPE_UNSET;
  this->subtype = Mixing_Group::SUBTYPE_NONE;

  strcpy(this->label, lab);

  this->first_day_infectious = -1;
  this->last_day_infectious = -2;
  
  this->N_orig = 0;             // orig number of enrollees

  // lists of people
  this->enrollees.clear();

  // track whether or not place is infectious with each disease
  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->recovered_bitset.reset();
  this->exposed_bitset.reset();
  
  int diseases = Global::Diseases.get_number_of_diseases();
  this->infectious_people = new std::vector<Person*>[diseases];

  // epidemic counters
  this->new_infections = new int[diseases];
  this->current_infections = new int[diseases];
  this->total_infections = new int[diseases];
  this->new_symptomatic_infections = new int[diseases];
  this->current_symptomatic_infections = new int[diseases];
  this->total_symptomatic_infections = new int[diseases];
  // these counts refer to today's agents
  this->current_infectious_agents = new int[diseases];
  this->current_symptomatic_agents = new int[diseases];

  // zero out all disease-specific counts
  for(int d = 0; d < diseases; ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;
    this->current_infectious_agents[d] = 0;
    this->current_symptomatic_agents[d] = 0;
    this->infectious_people[d].clear();
  }
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

int Mixing_Group::enroll(Person* per) {
  if(this->get_size() == this->enrollees.capacity()) {
    // double capacity if needed (to reduce future reallocations)
    this->enrollees.reserve(2 * this->get_size());
  }
  this->enrollees.push_back(per);
  FRED_VERBOSE(1, "Enroll person %d age %d in mixing group %d %s\n", per->get_id(), per->get_age(), this->get_id(), this->get_label());
  return this->enrollees.size()-1;
}

void Mixing_Group::unenroll(int pos) {
  int size = this->enrollees.size();
  if(!(0 <= pos && pos < size)) {
    FRED_VERBOSE(1, "mixing group %d %s pos = %d size = %d\n", this->get_id(), this->get_label(), pos, size);
  }
  assert(0 <= pos && pos < size);
  Person* removed = this->enrollees[pos];
  if(pos < size-1) {
    Person* moved = this->enrollees[size - 1];
    FRED_VERBOSE(1, "UNENROLL mixing group %d %s pos = %d size = %d removed %d moved %d\n",
      this->get_id(), this->get_label(), pos, size, removed->get_id(), moved->get_id());
    this->enrollees[pos] = moved;
    moved->update_enrollee_index(this, pos);
  } else {
    FRED_VERBOSE(1, "UNENROLL mixing group %d %s pos = %d size = %d removed %d moved NONE\n",
     this->get_id(), this->get_label(), pos, size, removed->get_id());
  }
  this->enrollees.pop_back();
  FRED_VERBOSE(1, "UNENROLL mixing group %d %s size = %d\n", this->get_id(), this->get_label(), this->enrollees.size());
}

void Mixing_Group::print_infectious(int disease_id) {
  printf("INFECTIOUS in Mixing_Group %s Disease %d: ", this->get_label(), disease_id);
  int size = this->infectious_people[disease_id].size();
  for(int i = 0; i < size; ++i) {
    printf(" %d", this->infectious_people[disease_id][i]->get_id());
  }
  printf("\n");
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
