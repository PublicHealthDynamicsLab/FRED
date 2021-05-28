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
// File: Epidemic.cc
//
#include <stdlib.h>
#include <stdio.h>
#include <new>
#include <iostream>
#include <vector>
#include <map>
#include <set>

using namespace std;

#include "Condition.h"
#include "Date.h"
#include "Epidemic.h"
#include "Events.h"
#include "Expression.h"
#include "Geo.h"
#include "Global.h"
#include "Hospital.h"
#include "Household.h"
#include "Natural_History.h"
#include "Neighborhood_Layer.h"
#include "Network.h"
#include "Network_Type.h"
#include "Property.h"
#include "Person.h"
#include "Place.h"
#include "Place_Type.h"
#include "Random.h"
#include "Rule.h"
#include "Transmission.h"
#include "Utils.h"
#include "Visualization_Layer.h"


/**
 * This static factory method is used to get an instance of a specific
 * Epidemic Model.  Depending on the model, it will create a
 * specific Epidemic Model and return a pointer to it.
 */

Epidemic* Epidemic::get_epidemic(Condition* condition) {
  return new Epidemic(condition);
}

//////////////////////////////////////////////////////
//
// CONSTRUCTOR
//
//

Epidemic::Epidemic(Condition* _condition) {
  this->condition = _condition;
  this->id = _condition->get_id();
  strcpy(this->name, _condition->get_natural_history()->get_name());

  // these are total (among current population)
  this->total_cases = 0;

  // reporting 
  this->report_generation_time = false;
  this->total_serial_interval = 0.0;
  this->total_secondary_cases = 0;
  this->enable_health_records = false;

  this->RR = 0.0;
  this->natural_history = NULL;

  this->daily_cohort_size = new int[Global::Simulation_Days];
  this->number_infected_by_cohort = new int[Global::Simulation_Days];
  for(int i = 0; i < Global::Simulation_Days; ++i) {
    this->daily_cohort_size[i] = 0;
    this->number_infected_by_cohort[i] = 0;
  }

  this->new_exposed_people_list.clear();
  this->active_people_list.clear();
  this->transmissible_people_list.clear();
  this->group_state_count.clear();
  this->total_group_state_count.clear();
  this->susceptible_count = 0;

  this->vis_case_fatality_loc_list.clear();
  this->enable_visualization = false;
}

Epidemic::~Epidemic() {
  this->group_state_count.clear();
  this->total_group_state_count.clear();
}

//////////////////////////////////////////////////////
//
// SETUP
//
//

void Epidemic::setup() {
  using namespace Utils;

  // read optional properties
  Property::disable_abort_on_failure();
  
  int temp = 0;
  Property::get_property(this->name, "report_generation_time", &temp);
  this->report_generation_time = temp;

  temp = 0;
  Property::get_property(this->name, "enable_health_records", &temp);
  this->enable_health_records = temp;

  // initialize state specific-variables here:
  this->natural_history = this->condition->get_natural_history();

  this->number_of_states = this->natural_history->get_number_of_states();
  // FRED_VERBOSE(0, "Epidemic::setup states = %d\n", this->number_of_states);

  //vectors of group state counts, size to number of states
  this->group_state_count.reserve(this->number_of_states);
  for (int i=0; i < this->number_of_states; i++) {
    group_counter_t new_counter;
    this->group_state_count.push_back(new_counter);
  }
  this->total_group_state_count.reserve(this->number_of_states);
  for (int i=0; i < this->number_of_states; i++) {
    group_counter_t new_counter;
    this->total_group_state_count.push_back(new_counter);
  }

  // initialize state counters
  this->incidence_count = new int [this->number_of_states];
  this->total_count = new int [this->number_of_states];
  this->current_count = new int [this->number_of_states];
  this->daily_incidence_count = new int* [this->number_of_states];
  this->daily_current_count = new int* [this->number_of_states];

  // visualization loc lists for dormant states
  this->vis_dormant_loc_list = new vis_loc_vec_t [this->number_of_states];
  for (int i = 0; i < this->number_of_states; i++) {
    this->vis_dormant_loc_list[i].clear();
  }

  // read state specific properties
  visualize_state = new bool [this->number_of_states];
  visualize_state_place_type = new int [this->number_of_states];

  for (int i = 0; i < this->number_of_states; i++) {
    char label[FRED_STRING_SIZE];
    this->incidence_count[i] = 0;
    this->total_count[i] = 0;
    this->current_count[i] = 0;

    this->daily_incidence_count[i] = new int [Global::Simulation_Days+1];
    this->daily_current_count[i] = new int [Global::Simulation_Days+1];
    for (int day = 0; day <= Global::Simulation_Days; day++) {
      this->daily_incidence_count[i][day] = 0;
      this->daily_current_count[i][day] = 0;
    }

    sprintf(label, "%s.%s",
	    this->natural_history->get_name(),
	    this->natural_history->get_state_name(i).c_str());

    // do we collect data for visualization?
    int n = 0;
    Property::get_property(label, "visualize", &n);
    this->visualize_state[i] = n;
    if (Global::Enable_Visualization_Layer && n > 0) {
      this->enable_visualization = true;
    }

    char type_name[FRED_STRING_SIZE];
    sprintf(type_name, "Household");
    Property::get_property(label, "visualize_place_type", type_name);
    this->visualize_state_place_type[i] = Place_Type::get_type_id(type_name);
  }

  // read in transmissible networks if using network transmission
  if (strcmp(this->condition->get_transmission_mode(),"network")==0) {
    this->transmissible_networks.clear();
    string nets = "";
    Property::get_property(this->name, "transmissible_networks", &nets);
    string_vector_t net_vec = Utils::get_string_vector(nets, ' ');

    for (int i = 0; i < net_vec.size(); i++) {
      printf("transmissible network for %s is %s\n", this->name, net_vec[i].c_str());
      Network* network = Network_Type::get_network_type(net_vec[i])->get_network();
      if (network != NULL) {
	this->transmissible_networks.push_back(network);
      }
      else {
	FRED_VERBOSE(0, "Help: no network named %s found.\n", net_vec[i].c_str());
      }
    }
  }

  // restore requiring properties
  Property::set_abort_on_failure();

  FRED_VERBOSE(0, "setup for epidemic condition %s finished\n", this->name);

}


//////////////////////////////////////////////////////
//
// FINAL PREP FOR THE RUN
//
//

void Epidemic::prepare_to_track_counts() {
  this->track_counts_for_group_state = new bool* [this->number_of_states];
  int number_of_group_types = Group_Type::get_number_of_group_types();
  for (int state = 0; state < this->number_of_states; state++) {
    this->track_counts_for_group_state[state] = new bool [number_of_group_types];
    for (int type = 0; type < number_of_group_types; type++) {
      this->track_counts_for_group_state[state][type] = false;
    }
  }
}


void Epidemic::prepare() {
  
  FRED_VERBOSE(0, "Epidemic::prepare epidemic %s started\n", this->name);

  // set maximum number of loops in update_state()
  if (Global::Max_Loops==-1) {
    Global::Max_Loops = Person::get_population_size();
  }
  FRED_VERBOSE(0, "Max_Loops %d\n", Global::Max_Loops);

  int number_of_group_types = Group_Type::get_number_of_group_types();
  for (int state = 0; state < this->number_of_states; state++) {
    for (int type = 0; type < number_of_group_types; type++) {
      if (this->track_counts_for_group_state[state][type]) {
	printf("TRACKING state %s.%s for group type %s\n",
	       this->name, 
	       this->natural_history->get_state_name(state).c_str(),
	       Group_Type::get_group_type_name(type).c_str());
      }
    }
  }

  // setup administrative agents
  int admin_agents = Person::get_number_of_admin_agents();
  for(int p = 0; p < admin_agents; ++p) {
    Person* admin_agent = Person::get_admin_agent(p);
    int new_state = 0;
    update_state(admin_agent, 0, 0, new_state, 0);
  }

  // initialize the population
  int day = 0;
  int popsize = Person::get_population_size();
  for(int p = 0; p < popsize; ++p) {
    Person* person = Person::get_person(p);
    initialize_person(person, day);
  }
  
  // setup meta_agent responsible for exogeneous transmission
  this->import_agent = Person::get_import_agent();
  int new_state = this->natural_history->get_import_start_state();
  if (0 <= new_state) {
    FRED_VERBOSE(0, "Epidemic::initialize meta_agent %s\n", this->name);
    update_state(this->import_agent, 0, 0, new_state, 0);
  }

  // setup visualization directories
  if (this->enable_visualization) {
    create_visualization_data_directories();
    FRED_VERBOSE(0, "visualization directories created\n");
  }

  FRED_VERBOSE(0, "epidemic prepare finished\n");

}


void Epidemic::initialize_person(Person* person, int day) {

  FRED_VERBOSE(1, "Epidemic::initialize_person %s started\n", this->name);

  int old_state = -1;
  int new_state = 0;
  int hour = 0;
    
  /*
  FRED_VERBOSE(0, "INITIAL_STATE COND %d %s day %d id %d age %0.2f race %d sex %c old_state %d new_state %d\n", 
	       this->id, this->name,
	       day, person->get_id(), person->get_real_age(),
	       person->get_race(), person->get_sex(), old_state, new_state);
  */

  if (new_state == this->natural_history->get_exposed_state()) {
    // person is initially exposed
    // FRED_VERBOSE(0, "person %d is initially exposed \n", person->get_id());
    person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
    this->new_exposed_people_list.push_back(person);
  }
  
  // update epidemic counters
  update_state(person, day, 0, new_state, 0);

  FRED_VERBOSE(1, "Epidemic::initialize_person %s finished\n", this->name);

}


///////////////////////////////////////////////////////
//
// DAILY UPDATES
//
//

void Epidemic::update(int day, int hour) {
  char msg[FRED_STRING_SIZE];

  FRED_VERBOSE(1, "epidemic update for condition %s day %d hour %d\n",
	       this->name, day, hour);
  Utils::fred_start_epidemic_timer();

  int step = 24*day + hour;
  FRED_VERBOSE(1, "epidemic update for condition %s day %d hour %d step %d\n",
	       this->name, day, hour, step);

  if (hour==0) {
    prepare_for_new_day(day);
  }    

  // handle scheduled transitions for import_agent
  int size = this->meta_agent_transition_event_queue.get_size(step);
  // FRED_VERBOSE(0, "META_TRANSITION_EVENT_QUEUE day %d %s hour %d cond %s size %d\n",
  // day, Date::get_date_string().c_str(), hour, this->name, size);
  for(int i = 0; i < size; ++i) {
    Person* person = this->meta_agent_transition_event_queue.get_event(step, i);
    update_state(person, day, hour, -1, 0);
  }

  // handle scheduled transitions
  size = this->state_transition_event_queue.get_size(step);
  FRED_VERBOSE(1, "TRANSITION_EVENT_QUEUE day %d %s hour %d cond %s size %d\n",
	       day, Date::get_date_string().c_str(), hour, this->name, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->state_transition_event_queue.get_event(step, i);
    update_state(person, day, hour, -1, 0);
  }
  this->state_transition_event_queue.clear_events(step);

  if (this->condition->get_transmissibility() > 0.0) {
    FRED_VERBOSE(1, "update transmissions for condition %s with transmissibility = %f\n", 
		 this->name,
		 this->condition->get_transmissibility());

    if (strcmp(this->condition->get_transmission_mode(), "proximity")==0) {
      update_proximity_transmissions(day, hour);
    }

    if (strcmp(this->condition->get_transmission_mode(), "respiratory")==0) {
      update_proximity_transmissions(day, hour);
    }

    if (strcmp(this->condition->get_transmission_mode(), "network")==0) {
      update_network_transmissions(day, hour);
    }
  }

  // FRED_VERBOSE(0, "epidemic update finished for condition %d day %d\n", id, day);
  return;
}


void Epidemic::update_proximity_transmissions(int day, int hour) {

  // spread infection in places attended by transmissible people

  int number_of_place_types = Place_Type::get_number_of_place_types();
  for(int type = 0; type < number_of_place_types; ++type) {
    Place_Type* place_type = Place_Type::get_place_type(type);
    int time_block = place_type->get_time_block(day, hour);
    if (time_block > 0) {
      FRED_VERBOSE(1, "place_type %s opens at hour %d on %s for %d hours on %s\n",
		   place_type->get_name(), hour,
		   Date::get_day_of_week_string().c_str(),
		   time_block,
		   Date::get_date_string().c_str());
      find_active_places_of_type(day, hour, type);
      transmission_in_active_places(day, hour, time_block);
    }
    else {
      FRED_VERBOSE(1, "place_type %s does not open at hour %d on %s on %s\n",
		   place_type->get_name(), hour,
		   Date::get_day_of_week_string().c_str(),
		   Date::get_date_string().c_str());
    }
  }
}

void Epidemic::update_network_transmissions(int day, int hour) {

  // FRED_VERBOSE(0, "update_network_transmission entered day %d hour %d for cond %d\n", day, hour, this->id);

  // spread infection in network attended by transmissible people

  int number_of_networks = Network_Type::get_number_of_network_types();
  for(int i = 0; i < number_of_networks; ++i) {
    Network* network = Network_Type::get_network_number(i);
    // FRED_VERBOSE(0, "update_network_transmission entered day %d hour %d network %s\n", day, hour, network->get_label());
    if (network->can_transmit(this->id) == false) {
      continue;
    }
    // printf("network %s can transmit this condition %d\n", network->get_label(), this->id);
    int time_block = network->get_time_block(day, hour);
    if (time_block == 0) {
      // printf("network %s time_block = %d\n", network->get_label(), time_block);
      continue;
    }
    if (network->has_admin_closure()) {
      FRED_VERBOSE(1, "network %s has an admin closure on day %d hour %d\n",
		   network->get_label(), day, hour);
      continue;
    }
    FRED_VERBOSE(1, "network %s is open at hour %d on %s for %d hours on %s\n",
		 network->get_label(), hour,
		 Date::get_day_of_week_string().c_str(),
		 time_block,
		 Date::get_date_string().c_str());
      
    // is this network active (does it have transmissible people attending?)
    bool active = false;
    for(person_set_iterator itr = this->transmissible_people_list.begin(); itr != this->transmissible_people_list.end(); ++itr ) {
      Person* person = (*itr);
      assert(person!=NULL);
      if (person->is_member_of_network(network)) {
	person->update_activities(day);
	if(person->is_present(day, network)) {
	  FRED_VERBOSE(1, "FOUND transmissible person %d day %d network %s\n",
		       person->get_id(), day, network->get_label());
	  network->add_transmissible_person(this->id, person);
	  active = true;
	}
	else {
	  FRED_VERBOSE(1, "FOUND transmissible person %d day %d NOT PRESENT network %s\n",
		       person->get_id(), day, network->get_label());
	}
      }
      else {
	FRED_VERBOSE(1, "FOUND transmissible person %d day %d NOT MEMBER OF network %s\n",
		     person->get_id(), day, network->get_label());
      }
    }
    if (active) {
      FRED_VERBOSE(1, "network %s is active day %d transmissible_people = %d\n",
		   network->get_label(), day, network->get_number_of_transmissible_people(this->id));
      this->condition->get_transmission()->transmission(day, hour, this->id, network, time_block);
      // prepare for next step
      network->clear_transmissible_people(this->id);
    }
    else {
      FRED_VERBOSE(1, "network %s is not active day %d transmissible_people = %d\n",
		   network->get_label(), day, get_number_of_transmissible_people());
    }
  }
}


void Epidemic::prepare_for_new_day(int day) {

  FRED_VERBOSE(1, "epidemic %s prepare for new day %d\n", this->name, day);

  // clear dormant location lists
  if (day > 0) {
    for (int i = 0; i < this->number_of_states; i++) {
      this->vis_dormant_loc_list[i].clear();
    }
  }
}

void Epidemic::find_active_places_of_type(int day, int hour, int place_type_id) {

  Place_Type* place_type = Place_Type::get_place_type(place_type_id);
  FRED_VERBOSE(1, "find_active_places_of_type %s day %d hour %d transmissible_people = %d\n",
	       place_type->get_name(), day, hour, get_number_of_transmissible_people());

  this->active_places_list.clear();
  for(person_set_iterator itr = this->transmissible_people_list.begin(); itr != this->transmissible_people_list.end(); ++itr ) {
    Person* person = (*itr);
    assert(person!=NULL);
    person->update_activities(day);
    Place* place = person->get_place_of_type(place_type_id);
    if (place==NULL) {
      continue;
    }
    FRED_VERBOSE(1, "find_active_places_of_type %d day %d person %d place %s\n",
		 place_type_id, day, person->get_id(), place? place->get_label() : "NULL");
    if (place->has_admin_closure()) {
      FRED_VERBOSE(1, "place %s has admin closure\n", place->get_label());
      continue;
    }
    if (person->is_present(day, place)) {
      FRED_VERBOSE(1, "FOUND transmissible person %d day %d hour %d place %s\n", 
		   person->get_id(), day, hour, place->get_label());
      place->add_transmissible_person(this->id, person);
      this->active_places_list.insert(place);
    }
  }
  
  if (0 && this->active_places_list.size() > 0) {
    FRED_VERBOSE(0, "find_active_places_of_type %s found %d\n",
		 place_type->get_name(), this->active_places_list.size());
  }
}
  
void Epidemic::transmission_in_active_places(int day, int hour, int time_block) {
  // FRED_VERBOSE(0, "transmission_in_active_places day %d hour %d places %lu\n", day, hour, active_places_list.size());
  for(place_set_iterator itr = active_places_list.begin(); itr != this->active_places_list.end(); ++itr) {
    Place* place = *itr;
    // FRED_VERBOSE(0, "transmission_in_active_place day %d hour %d place %d\n", day, hour, place->get_id());
    this->condition->get_transmission()->transmission(day, hour, this->id, place, time_block);

    // prepare for next step
    place->clear_transmissible_people(this->id);
  }
  return;
}


//////////////////////////////////////////////////////////////
//
// HANDLING CHANGES TO AN INDIVIDUAL'S STATUS
//
//


// called from attempt_transmission():

void Epidemic::become_exposed(Person* person, int day, int hour) {
  int new_state = this->natural_history->get_exposed_state();
  update_state(person, day, hour, new_state, 0);
  this->new_exposed_people_list.push_back(person);
}


void Epidemic::become_active(Person* person, int day) {
  FRED_VERBOSE(1, "Epidemic::become_active day %d person %d\n", day, person->get_id());

  // add to active people list
  this->active_people_list.insert(person);

  // update epidemic counters
  this->total_cases++;
}

void Epidemic::inactivate(Person* person, int day, int hour) {

  FRED_VERBOSE(1, "inactivate day %d person %d\n", day, person->get_id());
  
  person_set_iterator itr = this->transmissible_people_list.find(person);
  if(itr != this->transmissible_people_list.end()) {
    // delete from transmissible list
    // FRED_VERBOSE(0, "DELETE inactive from TRANSMISSIBLE_PEOPLE_LIST day %d hour %d person %d\n", day, hour, person->get_id());
    this->transmissible_people_list.erase(itr);
  }

  itr = this->active_people_list.find(person);
  if(itr != this->active_people_list.end()) {
    // delete from active list
    FRED_VERBOSE(1, "DELETE from ACTIVE_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->active_people_list.erase(itr);
  }

  if (this->enable_visualization) {
    int state = person->get_state(this->id);
    if (this->visualize_state[state]) {
      Place* place = person->get_place_of_type(this->visualize_state_place_type[state]);
      if (place != NULL) {
	double lat = place->get_latitude();
	double lon = place->get_longitude();
	VIS_Location* vis_loc = new VIS_Location(lat,lon);
	this->vis_dormant_loc_list[state].push_back(vis_loc);
	/*
	  printf("day %d add to dormant_loc_list[%s] lat %f lon %f size %d\n",
	  day, this->natural_history->get_state_name(state).c_str(),
	  lat, lon, (int)this->vis_dormant_loc_list[state].size());
	*/
      }
    }
  }

  FRED_VERBOSE(1, "inactivate day %d person %d finished\n", day, person->get_id());
}

void Epidemic::terminate_person(Person* person, int day) {

  FRED_VERBOSE(1, "EPIDEMIC %s TERMINATE person %d day %d\n",
               this->name, person->get_id(), day);

  // note: the person is still in its current health state
  int state = person->get_state(this->id);
  // FRED_VERBOSE(0, "state = %d\n", state);

  if (0 <= state && this->natural_history->is_fatal_state(state)==false) {
    current_count[state]--;
    daily_current_count[state][day]--;
    FRED_VERBOSE(1, "EPIDEMIC TERMINATE person %d day %d %s removed from state %d\n",
		 person->get_id(), day, Date::get_date_string().c_str(), state);
  }

  delete_from_epidemic_lists(person);

  // update total case count
  if (person->was_ever_exposed(this->id)) {
    this->total_cases--;
  }

  // cancel any scheduled transition
  int transition_step = person->get_next_transition_step(this->id);
  if (24*day <= transition_step) {
    // printf("person %d delete_event for transition_step %d\n", person->get_id(), transition_step);
    this->state_transition_event_queue.delete_event(transition_step, person);
  }
  person->set_next_transition_step(this->id, -1);

  FRED_VERBOSE(1, "EPIDEMIC TERMINATE person %d finished\n", person->get_id());
}

//////////////////////////////////////////////////////////////
//
// REPORTING
//
//

void Epidemic::report(int day) {
  print_stats(day);
  Utils::fred_print_lap_time("day %d %s report", day, this->name);
  if (this->enable_visualization && (day % Global::Visualization->get_period() == 0)) {
    print_visualization_data(day);
    Utils::fred_print_lap_time("day %d %s print_visualization_data", day, this->name);
  }
}

void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic print stats for condition %d day %d\n", id, day);

  FILE* fp;
  char file[FRED_STRING_SIZE];

  // set daily cohort size
  this->daily_cohort_size[day] = this->new_exposed_people_list.size();

  if(this->report_generation_time || Global::Report_Serial_Interval) {
    // FRED_VERBOSE(0, "report serial interval\n");
    report_serial_interval(day);
  }
  
  // prepare for next day
  this->new_exposed_people_list.clear();
  if (this->natural_history != NULL) {
    for (int i = 0; i < this->number_of_states; ++i) {
      this->incidence_count[i] = 0;
      this->daily_current_count[i][day+1] = this->daily_current_count[i][day];
    }
  }

  FRED_VERBOSE(1, "epidemic finished print stats for condition %d day %d\n", id, day);
}

void Epidemic::report_serial_interval(int day) {

  int people_exposed_today = this->new_exposed_people_list.size();
  for(int i = 0; i < people_exposed_today; ++i) {
    Person* host = this->new_exposed_people_list[i];
    Person* source = host->get_source(id);
    if(source != NULL) {
      int serial_interval = host->get_exposure_day(this->id)
	- source->get_exposure_day(this->id);
      this->total_serial_interval += static_cast<double>(serial_interval);
      this->total_secondary_cases++;
    }
  }

  double mean_serial_interval = 0.0;
  if(this->total_secondary_cases > 0) {
    mean_serial_interval = this->total_serial_interval / static_cast<double>(this->total_secondary_cases);
  }

  if(Global::Report_Serial_Interval) {
    //Write to log file
    Utils::fred_log("\nday %d SERIAL_INTERVAL:", day);
    Utils::fred_log("\n ser_int %.2f\n", mean_serial_interval);
  }

}

void Epidemic::create_visualization_data_directories() {
  char vis_var_dir[FRED_STRING_SIZE];
  // create directories for each state
  for (int i = 0; i < this->number_of_states; i++) {
    if (this->visualize_state[i]) {
      sprintf(vis_var_dir, "%s/%s.%s",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str());
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/%s.new%s",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str());
      Utils::fred_make_directory(vis_var_dir);
      // add this variable name to visualization list
      char filename[FRED_STRING_SIZE];
      sprintf(filename, "%s/VARS", Global::Visualization_directory);
      FILE* fp = fopen(filename, "a");
      if (fp != NULL) {
	fprintf(fp, "%s.%s\n", 
		this->name,
		this->natural_history->get_state_name(i).c_str());
	fprintf(fp, "%s.new%s\n", 
		this->name,
		this->natural_history->get_state_name(i).c_str());
	fclose(fp);
      }
    }
  }
}


void Epidemic::print_visualization_data(int day) {
  char filename[FRED_STRING_SIZE];
  FILE* fp;
  Person* person;
  Place* household;
  long long int tract;
  double lat, lon;
  char location[FRED_STRING_SIZE];
  FILE* statefp[this->number_of_states];
  FILE* newstatefp[this->number_of_states];

  // open file pointers for each state
  for (int i = 0; i < this->number_of_states; i++) {
    statefp[i] = NULL;
    newstatefp[i] = NULL;
    if (this->visualize_state[i]) {
      sprintf(filename, "%s/%s.new%s/loc-%d.txt",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str(),
	      day);
      newstatefp[i] = fopen(filename, "w");
      sprintf(filename, "%s/%s.%s/loc-%d.txt",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str(),
	      day);
      statefp[i] = fopen(filename, "w");
    }
  }

  for(person_set_iterator itr = this->active_people_list.begin(); itr != this->active_people_list.end(); ++itr) {
    person = (*itr);
    int state = person->get_state(this->id);
    // printf("print_vis day %d person %d state %d\n", day, person->get_id(), state);
    assert(0 <= state);
    if (this->visualize_state[state]) {
      Place* place = person->get_place_of_type(this->visualize_state_place_type[state]);
      if (place != NULL) {
	lat = place->get_latitude();
	lon = place->get_longitude();
	sprintf(location,  "%f %f\n", lat, lon);
	if (day == person->get_last_transition_step(this->id)/24) {
	  fputs(location, newstatefp[state]);
	}
	fputs(location, statefp[state]);
      }
    }
  }

  // print data for dormant people
  for (int state = 0; state < this->number_of_states; state++) {
    if (this->visualize_state[state]) {
      if (this->natural_history->is_dormant_state(state)) {
	int size = this->vis_dormant_loc_list[state].size();
	// printf("day %d dormant_state %s inactive_loc_list size %d\n",
	// day, this->natural_history->get_state_name(state).c_str(), size);
	for (int i = 0; i < size; i++) {
	  fprintf(newstatefp[state], "%f %f\n",
		  this->vis_dormant_loc_list[state][i]->get_lat(),
		  this->vis_dormant_loc_list[state][i]->get_lon());
	  fprintf(statefp[state], "%f %f\n",
		  this->vis_dormant_loc_list[state][i]->get_lat(),
		  this->vis_dormant_loc_list[state][i]->get_lon());
	}
      }
    }
  }

  // print data for case_fatalities
  for (int state = 0; state < this->number_of_states; state++) {
    if (this->natural_history->is_fatal_state(state)) {
      if (this->visualize_state[state]) {
	int size = this->vis_case_fatality_loc_list.size();
	for (int i = 0; i < size; i++) {
	  fprintf(newstatefp[state], "%f %f\n",
		  this->vis_case_fatality_loc_list[i]->get_lat(),
		  this->vis_case_fatality_loc_list[i]->get_lon());
	  fprintf(statefp[state], "%f %f\n",
		  this->vis_case_fatality_loc_list[i]->get_lat(),
		  this->vis_case_fatality_loc_list[i]->get_lon());
	}
      }
    }
  }
  this->vis_case_fatality_loc_list.clear();

  // close file pointers for each state
  for (int i = 0; i < this->number_of_states; i++) {
    if (statefp[i] != NULL) {
      fclose(statefp[i]);
    }
    if (newstatefp[i] != NULL) {
      fclose(newstatefp[i]);
    }
  }

}


void Epidemic::delete_from_epidemic_lists(Person* person) {

  // this only happens for terminated people
  FRED_VERBOSE(1, "deleting terminated person %d from active_people_list list\n", person->get_id());

  person_set_iterator itr = this->active_people_list.find(person);
  if(itr != this->active_people_list.end()) {
    // delete from active list
    FRED_VERBOSE(1, "DELETE from ACTIVE_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->active_people_list.erase(itr);
  }

  itr = this->transmissible_people_list.find(person);
  if(itr != this->transmissible_people_list.end()) {
    // delete from transmissible list
    FRED_VERBOSE(1, "DELETE from TRANSMISSIBLE_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->transmissible_people_list.erase(itr);
  }

}


void Epidemic::update_state(Person* person, int day, int hour, int new_state, int loop_counter) {

  int step = 24*day + hour;
  int old_state = person->get_state(this->id);

  double age = person->get_real_age();

  FRED_VERBOSE(1, "UPDATE_STATE ENTERED condition %s day %d hour %d person %d age %0.2f old_state %s new_state %s\n", 
	       this->name, day, hour, person->get_id(), age,
	       this->natural_history->get_state_name(old_state).c_str(),
	       this->natural_history->get_state_name(new_state).c_str());

  if (new_state < 0) {
    // this is a scheduled state transition
    new_state = this->natural_history->get_next_state(person, old_state);

    if (0) {
      FRED_VERBOSE(0, "UPDATE_STATE day %d hour %d person %d age %0.2f old_state %s SCHEDULED TRANSITION TO new_state %s\n", 
		   day, hour, person->get_id(), age,
		   this->natural_history->get_state_name(old_state).c_str(),
		   this->natural_history->get_state_name(new_state).c_str());

    }

    assert(0 <= new_state);

    // this handles a new exposure as the result of get_next_state()
    if (new_state == this->natural_history->get_exposed_state() && person->get_exposure_day(this->id) < 0) {
      person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
    }
  }
  else {

    // the following applies iff we are changing state through a state
    // modification process or a cross-infection from another condition:

    // cancel any scheduled transition
    int transition_step = person->get_next_transition_step(this->id);
    if (step <= transition_step) {
      // printf("person %d delete_event for transition_step %d\n", person->get_id(), transition_step);
      this->state_transition_event_queue.delete_event(transition_step, person);
    }
  }

  // at this point we should not have any transitions scheduled
  person->set_next_transition_step(this->id, -1);

  // get the next transition step
  int transition_step = this->natural_history->get_next_transition_step(person, new_state, day, hour);

  // DEBUGGING
  if (0) {
    FRED_VERBOSE(0, "UPDATE_STATE condition %s day %d hour %d person %d age %0.2f race %d sex %c old_state %d new_state %d next_transition_step %d\n", 
		 this->name,
		 day, hour, person->get_id(), age, person->get_race(), person->get_sex(), old_state, new_state,
		 transition_step);
  }

  if (transition_step > step) {

    FRED_VERBOSE(1, "UPDATE_STATE day %d hour %d adding person %d to state_transition_event_queue for step %d\n",
		 day, hour, person->get_id(), transition_step);

    if (person->is_meta_agent()) {
      FRED_VERBOSE(1, "UPDATE_STATE META cond %s day %d hour %d adding person %d with old_state %d new_state %d step %d to meta_agent_transition_event_queue for step %d\n",
		   this->name, day, hour, person->get_id(), old_state, new_state, step, transition_step);
      this->meta_agent_transition_event_queue.add_event(transition_step, person);
    }
    else {
      this->state_transition_event_queue.add_event(transition_step, person);
    }

    person->set_next_transition_step(this->id, transition_step);
    // printf("person %d cond %s next transition_step %d\n", person->get_id(), this->name, person->get_next_transition_step(this->id));
  }

  // does entering this state cause the import_agent to cause transmissions?
  if (person == this->import_agent) {
    if (0 <= new_state) {
      int max_imported = this->natural_history->get_import_count(new_state);
      double per_cap = this->natural_history->get_import_per_capita_transmissions(new_state);
      if (max_imported > 0 || per_cap > 0.0) {
	fred::geo lat = this->natural_history->get_import_latitude(new_state);
	fred::geo lon = this->natural_history->get_import_longitude(new_state);
	double radius = this->natural_history->get_import_radius(new_state);
	long long int admin_code = this->natural_history->get_import_admin_code(new_state);
	double min_age = this->natural_history->get_import_min_age(new_state);
	double max_age = this->natural_history->get_import_max_age(new_state);
	select_imported_cases(day, max_imported, per_cap, lat, lon, radius, admin_code, min_age, max_age, false);
      }
      else {
	if (this->natural_history->get_import_list_rule(new_state)) {
	  get_imported_list(this->natural_history->get_import_list_rule(new_state)->get_expression()->get_list_value(person));
	}
	else {
	  int max_imported = this->natural_history->get_import_count_rule(new_state) ? this->natural_history->get_import_count_rule(new_state)->get_expression()->get_value(person) : 0;
	  double per_cap = this->natural_history->get_import_per_capita_rule(new_state) ? this->natural_history->get_import_per_capita_rule(new_state)->get_expression()->get_value(person) : 0;
	  if (max_imported > 0 || per_cap > 0.0) {
	    FRED_VERBOSE(0, "UPDATE_STATE day %d hour %d person %d IMPORT max_imported %d per_cap %f\n", 
			 day, hour, person->get_id(), max_imported, per_cap);
	    fred::geo lat = this->natural_history->get_import_location_rule(new_state) ? this->natural_history->get_import_location_rule(new_state)->get_expression()->get_value(person) : 0;
	    fred::geo lon = this->natural_history->get_import_location_rule(new_state) ? this->natural_history->get_import_location_rule(new_state)->get_expression2()->get_value(person) : 0;
	    double radius = this->natural_history->get_import_location_rule(new_state) ? this->natural_history->get_import_location_rule(new_state)->get_expression3()->get_value(person) : 0;
	    long long int admin_code = this->natural_history->get_import_admin_code_rule(new_state) ? this->natural_history->get_import_admin_code_rule(new_state)->get_expression()->get_value(person) : 0;
	    double min_age = this->natural_history->get_import_ages_rule(new_state) ? this->natural_history->get_import_ages_rule(new_state)->get_expression()->get_value(person) : 0;
	    double max_age = this->natural_history->get_import_ages_rule(new_state) ? this->natural_history->get_import_ages_rule(new_state)->get_expression2()->get_value(person) : 999;
	    bool count_all = this->natural_history->all_import_attempts_count(new_state);
	    select_imported_cases(day, max_imported, per_cap, lat, lon, radius, admin_code, min_age, max_age, count_all);
	  }
	}
      }
    }
  }

  if (old_state != new_state) {

    // update the epidemic variables for this person's new state
    if (0 <= old_state) {
      this->current_count[old_state]--;
      this->daily_current_count[old_state][day]--;
      dec_state_count(person, old_state);
      // FRED_VERBOSE(0, "Modified counters for old_state %d\n", old_state);
    }

    if (0 <= new_state) {
      this->incidence_count[new_state]++;
      this->daily_incidence_count[new_state][day]++;
      this->total_count[new_state]++;
      this->current_count[new_state]++;
      this->daily_current_count[new_state][day]++;
      inc_state_count(person, new_state);
      // FRED_VERBOSE(0, "Modified counters for new_state %d\n", new_state);
    }

    // note: person's health state is still old_state

    // this is true if person was exposed through a modification operator
    if (new_state == this->natural_history->get_exposed_state() && person->get_exposure_day(this->id) < 0) {
      person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
    }

    if (this->natural_history->is_dormant_state(new_state) == false && 
	this->active_people_list.find(person) == this->active_people_list.end()) {
      become_active(person, day);
    }

    // now finally, update person's health state, transmissibility etc
    person->set_state(this->id, new_state, day);

    // DEBUGGING:
    if (0) {
      // show place counters for this condition for this agent
      FRED_VERBOSE(0, "UPDATE_STATE day %d person %d to state %s household count %d school count %d workplace count %d neighborhood count %d\n",
		   day,
		   person->get_id(),
		   this->condition->get_state_name(new_state).c_str(),
		   get_group_state_count(person->get_household(), new_state), 
		   get_group_state_count(person->get_school(), new_state),
		   get_group_state_count(person->get_workplace(), new_state), 
		   get_group_state_count(person->get_neighborhood(), new_state));
    }

    // update person health record
    if (0 <= old_state && this->enable_health_records && Global::Enable_Records) {
      if (0 <= new_state && this->natural_history->get_state_name(new_state)!="Excluded") {
	char tmp[FRED_STRING_SIZE];
	person->get_record_string(tmp);
	fprintf(Global::Recordsfp,
		"%s CONDITION %s CHANGES from %s to %s\n",
		tmp,
		this->name,
		old_state>=0 ? this->natural_history->get_state_name(old_state).c_str() : "-1",
		new_state>=0 ? this->natural_history->get_state_name(new_state).c_str() : "-1");
	fflush(Global::Recordsfp);
      }
    }

    if (this->natural_history->is_dormant_state(new_state)) {
      inactivate(person, day, hour);
    }

    // handle case fatalities
    if (this->natural_history->is_fatal_state(new_state)) {

      // update person's health record
      person->become_case_fatality(this->id, day);

      if(this->enable_visualization && this->visualize_state[new_state]) {
	// save visualization data for case fatality
	Place* place = person->get_place_of_type(this->visualize_state_place_type[new_state]);
	if (place != NULL) {
	  double lat = place->get_latitude();
	  double lon = place->get_longitude();
	  VIS_Location* vis_loc = new VIS_Location(lat,lon);
	  this->vis_case_fatality_loc_list.push_back(vis_loc);
	}
      }
      delete_from_epidemic_lists(person);
    }

  } // end of change to a new state

  else {				       // continue in same state
    // DEBUGGING
    if (0) {
      // update person health record
      if (this->enable_health_records && Global::Enable_Records) {
	char tmp[FRED_STRING_SIZE];
	person->get_record_string(tmp);
	fprintf(Global::Recordsfp,
		"%s CONDITION %s STATE %s STAYS %s\n",
		tmp,
		this->name,
		old_state>=0 ? this->natural_history->get_state_name(old_state).c_str() : "-1",
		new_state>=0 ? this->natural_history->get_state_name(new_state).c_str() : "-1");
	fflush(Global::Recordsfp);
      }
    } // end DEBUGGING

  } // end continue in same state

  // record old state of person
  bool was_susceptible = person->is_susceptible(this->id);
  bool was_transmissible = person->is_transmissible(this->id);

  // action rules
  person->run_action_rules(this->id, new_state, this->natural_history->get_action_rules(new_state));

  // new status
  bool is_now_susceptible = person->is_susceptible(this->id);
  bool is_now_transmissible = person->is_transmissible(this->id);

  // report change of status
  if (is_now_susceptible && !was_susceptible) {
    this->susceptible_count++;
  }
  if (!is_now_susceptible && was_susceptible) {
    this->susceptible_count--;
  }
  if (is_now_transmissible && !was_transmissible) {
    // add to transmissible_people_list
    this->transmissible_people_list.insert(person);
  }
  if (!is_now_transmissible && was_transmissible) {
    // delete from transmissible list
    person_set_iterator itr = this->transmissible_people_list.find(person);
    if(itr != this->transmissible_people_list.end()) {
      this->transmissible_people_list.erase(itr);
    }
  }
  
  // does entering this state cause agent to starting hosting?
  if (0 <= this->natural_history->get_place_type_to_transmit() &&
      this->natural_history->should_start_hosting(new_state)) {
    person->start_hosting(this->natural_history->get_place_type_to_transmit());
  }

  FRED_VERBOSE(1, "UPDATE_STATE FINISHED person %d condition %s day %d hour %d old_state %s new_state %s loops %d\n",
	       person->get_id(), this->name, day, hour, 
	       0<=old_state? this->natural_history->get_state_name(old_state).c_str(): "NONE",
	       0<=new_state? this->natural_history->get_state_name(new_state).c_str(): "NONE",
	       loop_counter);

  if (transition_step == step) {

    // counts loops
    if (old_state==new_state) {
      loop_counter++;
    }
    else{
      loop_counter=0;
    }

    if (0)
      FRED_VERBOSE(1, "UPDATE_STATE RECURSE person %d condition %s day %d hour %d old_state %s new_state %s loops %d max_loops %d\n",
		   person->get_id(), this->name, day, hour, 
		   0<=old_state? this->natural_history->get_state_name(old_state).c_str(): "NONE",
		   0<=new_state? this->natural_history->get_state_name(new_state).c_str(): "NONE",
		   loop_counter, Global::Max_Loops);

    // infinite loop protection
    if (loop_counter < Global::Max_Loops) {
      // recurse if this is an instant transition
      update_state(person, day, hour, -1, loop_counter);
    }
  }

}

void Epidemic::finish() {

  char dir[FRED_STRING_SIZE];
  char outfile[FRED_STRING_SIZE];
  FILE* fp;

  sprintf(dir, "%s/RUN%d/DAILY",
	  Global::Simulation_directory,
	  Global::Simulation_run_number);
  Utils::fred_make_directory(dir);

  for (int i = 0; i < this->number_of_states; i++) {
    sprintf(outfile, "%s/%s.new%s.txt",
	    dir,
	    this->name,
	    this->natural_history->get_state_name(i).c_str());
    fp = fopen(outfile, "w");
    if (fp == NULL) {
      Utils::fred_abort("Fred: can open file %s\n", outfile);
    }
    for (int day = 0; day < Global::Simulation_Days; day++) {
      fprintf(fp, "%d %d\n", day, this->daily_incidence_count[i][day]);
    }
    fclose(fp);

    sprintf(outfile, "%s/%s.%s.txt",
	    dir,
	    this->name,
	    this->natural_history->get_state_name(i).c_str());
    fp = fopen(outfile, "w");
    if (fp == NULL) {
      Utils::fred_abort("Fred: can open file %s\n", outfile);
    }
    for (int day = 0; day < Global::Simulation_Days; day++) {
      fprintf(fp, "%d %d\n", day, this->daily_current_count[i][day]);
    }
    fclose(fp);

    sprintf(outfile, "%s/%s.tot%s.txt",
	    dir,
	    this->name,
	    this->natural_history->get_state_name(i).c_str());
    fp = fopen(outfile, "w");
    if (fp == NULL) {
      Utils::fred_abort("Fred: can open file %s\n", outfile);
    }
    int tot = 0;
    for (int day = 0; day < Global::Simulation_Days; day++) {
      tot += this->daily_incidence_count[i][day];
      fprintf(fp, "%d %d\n", day, tot);
    }
    fclose(fp);
  }

  // reproductive rate
  sprintf(outfile, "%s/%s.RR.txt",
	  dir,
	  this->name);
  fp = fopen(outfile, "w");
  if (fp == NULL) {
    Utils::fred_abort("Fred: can open file %s\n", outfile);
  }
  for (int day = 0; day < Global::Simulation_Days; day++) {
    double rr = 0.0;
    if (this->daily_cohort_size[day] > 0) {
      rr = static_cast<double>(this->number_infected_by_cohort[day]) / static_cast<double>(this->daily_cohort_size[day]);
    }
    fprintf(fp, "%d %f\n", day, rr);
  }
  fclose(fp);

  // create a csv file for this condition

  // this joins two files with same value in column 1, from
  // https://stackoverflow.com/questions/14984340/using-awk-to-process-input-from-multiple-files
  char awkcommand[FRED_STRING_SIZE];
  sprintf(awkcommand, "awk 'FNR==NR{a[$1]=$2 FS $3;next}{print $0, a[$1]}' ");

  char command[FRED_STRING_SIZE];
  char dailyfile[FRED_STRING_SIZE];
  sprintf(outfile, "%s/RUN%d/%s.csv", Global::Simulation_directory, Global::Simulation_run_number, this->name);

  sprintf(dailyfile, "%s/%s.new%s.txt", dir, this->name, this->natural_history->get_state_name(0).c_str());
  sprintf(command, "cp %s %s", dailyfile, outfile);
  system(command);

  for (int i = 0; i < this->number_of_states; i++) {
    if (i > 0) {
      sprintf(dailyfile, "%s/%s.new%s.txt", dir, this->name, this->natural_history->get_state_name(i).c_str());
      sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s",
	      awkcommand, dailyfile, outfile, outfile, outfile, outfile);
      system(command);
    }
    
    sprintf(dailyfile, "%s/%s.%s.txt", dir, this->name, this->natural_history->get_state_name(i).c_str());
    sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s",
	    awkcommand, dailyfile, outfile, outfile, outfile, outfile);
    system(command);
    
    sprintf(dailyfile, "%s/%s.tot%s.txt", dir, this->name, this->natural_history->get_state_name(i).c_str());
    sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s",
	    awkcommand, dailyfile, outfile, outfile, outfile, outfile);
    system(command);
  }  
  sprintf(dailyfile, "%s/%s.RR.txt", dir, this->name);
  sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s",
	  awkcommand, dailyfile, outfile, outfile, outfile, outfile);
  system(command);

  // create a header line for the csv file
  char headerfile[FRED_STRING_SIZE];
  sprintf(headerfile, "%s/RUN%d/%s.header", Global::Simulation_directory, Global::Simulation_run_number, this->name);
  fp = fopen(headerfile, "w");
  fprintf(fp, "Day ");
  for (int i = 0; i < this->number_of_states; i++) {
    fprintf(fp, "%s.new%s ", this->name, this->natural_history->get_state_name(i).c_str());
    fprintf(fp, "%s.%s ", this->name, this->natural_history->get_state_name(i).c_str());
    fprintf(fp, "%s.tot%s ", this->name, this->natural_history->get_state_name(i).c_str());
  }
  fprintf(fp, "%s.RR\n", this->name);
  fclose(fp);

  // concatenate header line
  sprintf(command, "cat %s %s > %s.tmp; mv %s.tmp %s; unlink %s",
	  headerfile, outfile, outfile, outfile, outfile, headerfile);
  system(command);

  // replace spaces with commas
  sprintf(command, "sed -E 's/ +/,/g' %s | sed -E 's/,$//' | sed -E 's/,/ /' > %s.tmp; mv %s.tmp %s",
	  outfile, outfile, outfile, outfile);
  system(command);

}

double Epidemic::get_attack_rate() {
  return (double) this->total_cases / (double) Person::get_population_size();
}

void Epidemic::inc_state_count(Person* person, int state){

  for (int type_id = 0; type_id < Group_Type::get_number_of_group_types(); type_id++) {
    if (this->track_counts_for_group_state[state][type_id]) {
      Group* group = person->get_group_of_type(type_id);
      if (group != NULL) {
	if ( this->group_state_count[state].find(group) == this->group_state_count[state].end() ) {
	  // not present so insert with count of one
	  std::pair<Group*,int> new_count(group,1);
	  this->group_state_count[state].insert(new_count);
	  this->total_group_state_count[state].insert(new_count);
	}
	else { // present so increment count
	  this->group_state_count[state][group]++;
	  this->total_group_state_count[state][group]++;
	}  
	FRED_VERBOSE(1, "inc_state_count person %d group %s cond %s state %s count %d total_count %d\n",
		     person->get_id(), group->get_label(), this->name,
		     this->natural_history->get_state_name(state).c_str(),
		     this->group_state_count[state][group],
		     this->total_group_state_count[state][group]); 
      }
    }
  }
}

void Epidemic::dec_state_count(Person* person, int state){
  for (int type_id = 0; type_id < Group_Type::get_number_of_group_types(); type_id++) {
    if (this->track_counts_for_group_state[state][type_id]) {
      Group* group = person->get_group_of_type(type_id);
      if (group != NULL) {
	if ( this->group_state_count[state].find(group) != this->group_state_count[state].end() ) {
	  this->group_state_count[state][group]--;
	  FRED_VERBOSE(1, "dec_state_count person %d group %s cond %s state %s count %d\n",
		       person->get_id(), group->get_label(), this->name,
		       this->natural_history->get_state_name(state).c_str(),
		       this->group_state_count[state][group]); 
	}  
      }
    }
  }
}


int Epidemic::get_group_state_count(Group* place, int state) {
  int count = 0;
  if( this->group_state_count[state].find(place) != this->group_state_count[state].end() ){
    count = this->group_state_count[state][place];
  }
  return count;
}

int Epidemic::get_total_group_state_count(Group* place, int state) {
  int count = 0;
  if( this->total_group_state_count[state].find(place) != this->total_group_state_count[state].end() ){
    count = this->total_group_state_count[state][place];
  }
  return count;
}


void Epidemic::get_imported_list(double_vector_t id_list) {
  FRED_STATUS(0, "GET_IMPORTED_LIST: id_list size = %d\n", id_list.size());
  if (Global::Compile_FRED) {
    return;
  }
  int day = Global::Simulation_Day;
  int hour = Global::Simulation_Hour;

  int imported_cases = 0;
  for(int n = 0; n < id_list.size(); ++n) {
    Person* person = Person::get_person_with_id(id_list[n]);
    if (person != NULL) {
      person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
      become_exposed(person, day, hour);
      imported_cases++;
    }
  }
  FRED_STATUS(0, "GET_IMPORTED_LIST: imported cases = %d\n", imported_cases);
}

void Epidemic::select_imported_cases(int day, int max_imported, double per_cap, double lat, double lon, double radius, long long int admin_code, double min_age, double max_age, bool count_all) {

  FRED_VERBOSE(0,"IMPORT SPEC for %s day %d: max = %d per_cap = %f lat = %f lon = %f rad = %f fips = %lld min_age = %f max_age = %f\n",
	       this->name, day, max_imported, per_cap, lat, lon, radius, admin_code, min_age, max_age);
      
  if (Global::Compile_FRED) {
    return;
  }

  int popsize = Person::get_population_size();
  if (popsize==0) {
    return;
  }

  // FRED_VERBOSE(0, "popsize = %d susceptible_count = %d\n", popsize, this->susceptible_count);

  int hour = Global::Simulation_Hour;
  hour = 0;

  // number imported so far
  int imported_cases = 0;

  if (lat==0.0 && lon==0.0 && admin_code==0 && min_age==0 && max_age > 100 && (this->susceptible_count > 0.1 * popsize)) {

    // no location or age restriction -- select from entire population
    // using this optimization: select people from population at random,
    // and expose only the susceptibles ones.

    FRED_VERBOSE(0, "IMPORT OPTIMIZATION popsize = %d susceptible_count = %d\n", popsize, this->susceptible_count);

    // determine the target number of susceptibles to expose
    int real_target = 0;
    if (per_cap==0.0) {
      if (count_all) {
	real_target = max_imported * (double) this->susceptible_count / (double) popsize;
      }
      else {
	real_target = max_imported;
      }
    }
    else {
      // per_cap overrides max_imported.
      // NOTE: per_cap target is the same whether or not count_all = true
      real_target = per_cap * this->susceptible_count;
    }

    // determine integer number of susceptibles to expose
    int target = (int) real_target;
    double remainder = real_target - target;
    if (Random::draw_random(0,1) < remainder) {
      target++;
    }

    if (target == 0) {
      return;
    }

    // select people at random, and exposed if they are susceptible
    int tries = 0;
    while (imported_cases < target) {
      tries++;
      Person* person = Person::select_random_person();
      if (person->is_susceptible(this->id)) {
	double susc = person->get_susceptibility(this->id);
	// give preference to people with higher susceptibility

	// FRED_VERBOSE(0, "IMPORT person %d susceptibility = %f\n", person->get_id(), susc);
	if (susc == 1.0 || Random::draw_random(0,1) < susc) {
	  // expose the candidate
	  person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
	  become_exposed(person, day, hour);
	  // FRED_VERBOSE(0, "HEALTH IMPORT exposing candidate %d person %d\n", tries, person->get_id());
	  imported_cases++;
	}
      }
    } // end while loop import_cases

    if (tries > 0) {
      FRED_STATUS(0, "day %d IMPORT: %d tries yielded %d imported cases of %s\n",
		  day, tries, imported_cases, this->name);
    }
  }

  else {

    FRED_VERBOSE(0, "Enter susceptible selection process\n");

    // list of susceptible people that qualify by distance and age
    person_vector_t people;
    bool finished = false;

    // clear the list of candidates
    people.clear();

    // find households that qualify by location
    int hsize = Place::get_number_of_households();
    // printf("IMPORT: houses  %d\n", hsize); fflush(stdout);
    for(int i = 0; i < hsize; ++i) {
      Household* hh = Place::get_household(i);
      if (admin_code) {
	if (hh->get_census_tract_admin_code() != admin_code) {
	  continue;
	}
      }
      else {
	double dist = 0.0;
	if(radius > 0 || lat != 0 || lon != 0) {
	  dist = Geo::xy_distance(lat,lon,hh->get_latitude(),hh->get_longitude());
	  if(radius < dist) {
	    continue;
	  }
	}
      }
      // this household qualifies by location
      // find all susceptible Housemates who qualify by age.
      int size = hh->get_size();
      // printf("IMPORT: house %d %s size %d admin_code %lld\n", i, hh->get_label(), size, hh->get_census_tract_admin_code()); fflush(stdout);
      for(int j = 0; j < size; ++j) {
	Person* person = hh->get_member(j);
	if(person->is_susceptible(this->id)) {
	  double age = person->get_real_age();
	  if(min_age <= age && age < max_age) {
	    people.push_back(person);
	  }
	}
      }
    }

    int susceptibles = (int) people.size();
    // FRED_VERBOSE(0, "IMPORT susceptibles %d  popsize %d   max_imported  %d   per_cap %f\n", susceptibles, popsize, max_imported, per_cap);

    // determine the target number of susceptibles to expose
    double real_target = 0;
    if (per_cap==0.0) {
      if (count_all) {
	real_target = max_imported * (double) susceptibles / (double) popsize;
	// FRED_VERBOSE(0, "IMPORT: count_all real_target = %f candidates, found %d\n", real_target, susceptibles);
      }
      else {
	real_target = max_imported;
	// FRED_VERBOSE(0, "IMPORT: real_target = %f candidates, found %d\n", real_target, susceptibles);
      }
    }
    else {
      // per_cap overrides max_imported.
      // NOTE: per_cap target is the same whether or not count_all = true
      real_target = per_cap * susceptibles;
      // FRED_VERBOSE(0, "IMPORT: per_cap real_target = %f candidates, found %d\n", real_target, susceptibles);
    }

    // determine integer number of susceptibles to expose
    int target = (int) real_target;
    double remainder = real_target - target;
    if (remainder > 0 && Random::draw_random(0,1) < remainder) {
      target++;
    }

    // FRED_VERBOSE(0, "IMPORT: target = %d candidates, found %d\n", target, susceptibles);

    if (target == 0) {
      return;
    }

    // sort the candidates
    std::sort(people.begin(), people.end(), Utils::compare_id);

    if(target <= people.size()) {
      // we have at least the minimum number of candidates.

      for(int n = 0; n < target; ++n) {
	// FRED_VERBOSE(0, "IMPORT candidate %d people.size %d\n", n, (int)people.size());

	// pick a candidate without replacement
	int pos = Random::draw_random_int(0,people.size()-1);
	Person* person = people[pos];
	people[pos] = people[people.size() - 1];
	people.pop_back();

	// try to expose the candidate
	double susc = person->get_susceptibility(this->id);
	// FRED_VERBOSE(0, "IMPORT person %d susceptibility = %f\n", person->get_id(), susc);
	if (susc == 1.0 || Random::draw_random(0,1) < susc) {
	  person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
	  become_exposed(person, day, hour);
	  imported_cases++;
	  // FRED_VERBOSE(0, "HEALTH IMPORT exposing candidate %d person %d finished\n", n, person->get_id());
	}
      }
      FRED_STATUS(0, "IMPORT SUCCESS: day = %d imported %d cases of %s\n",
		  day, imported_cases, this->name);
      finished = true; // success!
    }
    else {
      // try to expose all the candidates
      // FRED_VERBOSE(0, "HEALTH IMPORT expose all %d candidates\n", people.size());
      for(int n = 0; n < people.size(); ++n) {
	Person* person = people[n];
	double susc = person->get_susceptibility(this->id);
	// FRED_VERBOSE(0, "IMPORT person %d susceptibility = %f\n", person->get_id(), susc);
	if (susc == 1.0 || Random::draw_random(0,1) < susc) {
	  person->become_exposed(this->id, Person::get_import_agent(), NULL, day, hour);
	  become_exposed(person, day, hour);
	  imported_cases++;
	}
      }
    }
    if (imported_cases < target) {
      FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases, target);
    }
  } // end else -- select from susceptibles
}


void Epidemic::increment_group_state_count(int group_type_id, Group* group, int state) {
  if (this->track_counts_for_group_state[state][group_type_id]) {
    if (group != NULL) {
      if ( this->group_state_count[state].find(group) == this->group_state_count[state].end() ) {
	// not present so insert with count of one
	std::pair<Group*,int> new_count(group,1);
	this->group_state_count[state].insert(new_count);
	this->total_group_state_count[state].insert(new_count);
      }
      else { // present so increment count
	this->group_state_count[state][group]++;
	this->total_group_state_count[state][group]++;
      }  
      FRED_VERBOSE(1, "increment_group_state_count group %s cond %s state %s count %d total_count %d\n",
		   group->get_label(), this->name,
		   this->natural_history->get_state_name(state).c_str(),
		   this->group_state_count[state][group],
		   this->total_group_state_count[state][group]); 
    }
  }
}

void Epidemic::decrement_group_state_count(int group_type_id, Group* group, int state) {
  if (this->track_counts_for_group_state[state][group_type_id]) {
    if (group != NULL) {
      /*
	printf("decrementing_group_state_count for cond %d state %d group %s\n",
	this->id, state, group->get_label());
      */
      if ( this->group_state_count[state].find(group) != this->group_state_count[state].end() ) {
	/*
	if (this->group_state_count[state][group]<1) {
	  printf("HELP: decrementing_group_state_count for cond %d state %d group %s would result in negative count\n",
		 this->id, state, group->get_label());
	  exit(0);
	}
	*/
	this->group_state_count[state][group]--;
	/*
	printf("decrementing_group_state_count for cond %d state %d group %s ==> %d\n",
	       this->id, state, group->get_label(), this->group_state_count[state][group]);
	*/
	FRED_VERBOSE(1, "decrement_group_state_count group %s cond %s state %s count = %d\n",
		     group->get_label(), this->name,
		     this->natural_history->get_state_name(state).c_str(),
		     this->group_state_count[state][group]); 
      }  
    }
  }
}

