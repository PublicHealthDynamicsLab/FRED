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
// File: Condition.cc
//

#include "Condition.h"
#include "Date.h"
#include "Epidemic.h"
#include "Global.h"
#include "Household.h"
#include "Natural_History.h"
#include "Neighborhood_Patch.h"
#include "Network.h"
#include "Network_Type.h"
#include "Property.h"
#include "Person.h"
#include "Random.h"
#include "Rule.h"
#include "Transmission.h"

#define PI 3.14159265359


Condition::Condition() {
  this->id = -1;
  this->transmissibility = -1.0;
  this->epidemic = NULL;
  this->natural_history = NULL;
}

Condition::~Condition() {
  delete this->natural_history;
  delete this->epidemic;
}

void Condition::get_properties(int condition_id, string name) {
  char property_name[256];

  // set condition id
  this->id = condition_id;
  
  // set condition name
  strcpy(this->condition_name, name.c_str());

  FRED_VERBOSE(0, "condition %d %s read_properties entered\n", this->id, this->condition_name);
  
  // optional properties:
  Property::disable_abort_on_failure();

  // type of transmission mode
  strcpy(this->transmission_mode,"none");
  Property::get_property(this->condition_name, "transmission_mode", this->transmission_mode);

  // transmission_network
  this->transmission_network = NULL;
  strcpy(this->transmission_network_name,"none");
  Property::get_property(this->condition_name, "transmission_network", this->transmission_network_name);
  if (strcmp(this->transmission_network_name,"none")!=0) {
    Network_Type::include_network_type(this->transmission_network_name);
  }

  Property::set_abort_on_failure();

  FRED_VERBOSE(0, "condition %d %s read_properties finished\n", this->id, this->condition_name);
}

void Condition::setup() {

  FRED_VERBOSE(0, "condition %d %s setup entered\n", this->id, this->condition_name);

  // Initialize Natural History
  this->natural_history = new Natural_History;

  // read in properties and files associated with this natural history model: 
  this->natural_history->setup(this);
  this->natural_history->get_properties();

  // contagiousness
  this->transmissibility = this->natural_history->get_transmissibility();
  
  if(this->transmissibility > 0.0) {
    // Initialize Transmission Model
    this->transmission = Transmission::get_new_transmission(this->transmission_mode);

    // read in properties and files associated with this transmission mode: 
    this->transmission->setup(this);

  }

  // Initialize Epidemic Model
  this->epidemic = Epidemic::get_epidemic(this);
  this->epidemic->setup();

  FRED_VERBOSE(0, "condition %d %s setup finished\n", this->id, this->condition_name);
}

void Condition::prepare() {

  FRED_VERBOSE(0, "condition %d %s prepare entered\n", this->id, this->condition_name);

  // get transmission_network if any
  if (strcmp(this->transmission_network_name,"none")!=0) {
    this->transmission_network = Network_Type::get_network_type(this->transmission_network_name)->get_network();
    assert(this->transmission_network != NULL);
  }

  // final prep for natural history model
  this->natural_history->prepare();

  // final prep for epidemic
  this->epidemic->prepare();

  FRED_VERBOSE(0, "condition %d %s prepare finished\n", this->id, this->condition_name);
}


void Condition::initialize_person(Person* person) {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    Condition::conditions[condition_id]->initialize_person(person, Global::Simulation_Day);
  }
}

void Condition::initialize_person(Person* person, int day) {
  this->epidemic->initialize_person(person, day);
}



double Condition::get_attack_rate() {
  return this->epidemic->get_attack_rate();
}

void Condition::report(int day) {
  this->epidemic->report(day);
}


void Condition::increment_cohort_host_count(int day) {
  this->epidemic->increment_cohort_host_count(day);
}
 
void Condition::update(int day, int hour) {
  this->epidemic->update(day, hour);
}

void Condition::terminate_person(Person* person, int day) {
  this->epidemic->terminate_person(person, day);
}

void Condition::finish() {
  this->epidemic->finish();
  this->natural_history->finish();
}

int Condition::get_number_of_states() {
  return this->natural_history->get_number_of_states();
}

string Condition::get_state_name(int i) {
  if (i < get_number_of_states()) {
    return this->natural_history->get_state_name(i);
  }
  else {
    return "UNDEFINED";
  }
}


void Condition::track_group_state_counts(int type_id, int state) {
  this->epidemic->track_group_state_counts(type_id, state);
}

int Condition::get_current_group_state_count(Group* group, int state){
  return epidemic->get_group_state_count(group, state);
}

int Condition::get_total_group_state_count(Group* group, int state){
  return epidemic->get_total_group_state_count(group, state);
}

int Condition::get_state_from_name(string state_name) {
  for (int i = 0; i < get_number_of_states(); ++i) {
    if (get_state_name(i) == state_name) {
      return i;
    }
  }
  return -1;
}

int Condition::get_condition_to_transmit(int state) {
  return this->natural_history->get_condition_to_transmit(state);
}

int Condition::get_incidence_count(int state) {
  return this->epidemic->get_incidence_count(state);
}

int Condition::get_current_count(int state) {
  return this->epidemic->get_current_count(state);
}

int Condition::get_total_count(int state) {
  return this->epidemic->get_total_count(state);
}

bool Condition::is_external_update_enabled() {
  return this->natural_history->is_external_update_enabled();
}

bool Condition::state_gets_external_updates(int state) {
  return this->natural_history->state_gets_external_updates(state);
}

bool Condition::is_absent(int state, int group_type_id) {
  return this->natural_history->is_absent(state, group_type_id);
}

bool Condition::is_closed(int state, int group_type_id) {
  return this->natural_history->is_closed(state, group_type_id);
}


//////////////////////////

std::vector <Condition*> Condition::conditions;
std::vector<string> Condition::condition_names;
int Condition::number_of_conditions = 0;


void Condition::get_condition_properties() {

  // read in the list of condition names
  Condition::conditions.clear();
  char property_name[FRED_STRING_SIZE];
  char property_value[FRED_STRING_SIZE];

  // optional properties:
  Property::disable_abort_on_failure();

  Property::set_abort_on_failure();

  strcpy(property_name, "conditions");
  if (Property::does_property_exist(property_name)) {
    Condition::condition_names.clear();
    Property::get_property(property_name, property_value);

    // parse the property value into separate strings
    char *pch;
    double v;
    pch = strtok(property_value," \t\n\r");
    while (pch != NULL) {
      Condition::include_condition(pch);
      //Condition::condition_names.push_back(string(pch));
      pch = strtok(NULL, " \t\n\r");
    }
  }

  Condition::number_of_conditions = Condition::condition_names.size();

  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {

    // create new Condition object
    Condition * condition = new Condition();

    // get its properties
    condition->get_properties(condition_id, Condition::condition_names[condition_id]);
    Condition::conditions.push_back(condition);
    printf("condition %d = %s\n", condition_id, Condition::condition_names[condition_id].c_str());
  }
}

void Condition::setup_conditions() {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    Condition::conditions[condition_id]->setup();
  }
}

void Condition::prepare_to_track_group_state_counts() {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    Condition *condition = Condition::conditions[condition_id]; 
    condition->get_epidemic()->prepare_to_track_counts();
  }
}

void Condition::prepare_conditions() {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    Condition *condition = Condition::conditions[condition_id]; 
    condition->prepare();
    // printf("FRED_SETUP cond %d %d\n", condition_id, Random::draw_random_int(0,10000));
  }
}


Condition* Condition::get_condition(string condition_name) {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    if (strcmp(condition_name.c_str(), Condition::conditions[condition_id]->get_name()) == 0) {
      return Condition::conditions[condition_id]; 
    }
  }
  return NULL;
}


int Condition::get_condition_id(string condition_name) {
  // printf("get_condition_id for name = |%s|\n", condition_name.c_str());
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    // printf("%d |%s|\n", condition_id, Condition::conditions[condition_id]->get_name()); fflush(stdout);
    if (strcmp(condition_name.c_str(), Condition::conditions[condition_id]->get_name()) == 0) {
      return condition_id; 
    }
  }
  return -1;
}


void Condition::finish_conditions() {
  for(int condition_id = 0; condition_id < Condition::number_of_conditions; ++condition_id) {
    Condition::conditions[condition_id]->finish(); 
  }
}

int Condition::get_place_type_to_transmit() {
  return this->natural_history->get_place_type_to_transmit();
}

bool Condition::health_records_are_enabled() {
  return this->epidemic->health_records_are_enabled();
}

void Condition::increment_group_state_count(int group_type_id, Group* group, int state) {
  this->epidemic->increment_group_state_count(group_type_id, group, state);
}

void Condition::decrement_group_state_count(int group_type_id, Group* group, int state) {
  this->epidemic->decrement_group_state_count(group_type_id, group, state);
}




