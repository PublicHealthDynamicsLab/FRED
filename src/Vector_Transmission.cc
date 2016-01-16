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
// File: Transmission.cc
//
#include <algorithm>

#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Global.h"
#include "Mixing_Group.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "Vector_Layer.h"
#include "Vector_Transmission.h"
#include "Utils.h"

//////////////////////////////////////////////////////////
//
// VECTOR TRANSMISSION MODEL
//
//////////////////////////////////////////////////////////

void Vector_Transmission::setup(Condition* condition) {
}

void Vector_Transmission::spread_infection(int day, int condition_id, Mixing_Group* mixing_group) {
  if(dynamic_cast<Place*>(mixing_group) == NULL) {
    //Vector_Transmission must occur on a Place type
    return;
  } else {
    this->spread_infection(day, condition_id, dynamic_cast<Place*>(mixing_group));
  }
}

void Vector_Transmission::spread_infection(int day, int condition_id, Place* place) {
  // abort if transmissibility == 0 or if place is closed
  Condition* condition = Global::Conditions.get_condition(condition_id);
  double beta = condition->get_transmissibility();
  if(beta == 0.0 || place->is_open(day) == false || place->should_be_open(day, condition_id) == false) {
    place->reset_place_state(condition_id);
    return;
  }

  // have place record first and last day of infectiousness
  place->record_infectious_days(day);

  visitors.clear();
  susceptible_visitors.clear();
  person_vec_t* tmp = place->get_enrollees();
  int tmp_size = tmp->size();
  for (int i = 0; i < tmp_size; i++) {
    Person* person = (*tmp)[i];
    person->update_schedule(day);
    if (person->is_present(day, place)) {
      visitors.push_back(person);
      if(person->is_susceptible(condition_id)) {
	susceptible_visitors.push_back(person);
      }
    }
  }
  FRED_VERBOSE(1, "Vector_Transmission::spread_infection entered place %s visitors %d\n",place->get_label(),visitors.size());
  // infections of vectors by hosts
  if(place->have_vectors_been_infected_today() == false) {
    infect_vectors(day, place);
  }

  // transmission from vectors to hosts
  infect_hosts(day, condition_id, place);

  place->reset_place_state(condition_id);
}


void Vector_Transmission::infect_vectors(int day, Place* place) {
  
  int total_hosts = this->visitors.size();
  if(total_hosts == 0) {
    return;
  }

  // skip if there are no susceptible vectors
  int susceptible_vectors = place->get_susceptible_vectors();
  if(susceptible_vectors == 0) {
    return;
  }
  
  // find the percent distribution of infectious hosts
  int conditions = Global::Conditions.get_number_of_conditions();
  int* infectious_hosts = new int [conditions];
  int total_infectious_hosts = 0;
  for(int condition_id = 0; condition_id < conditions; ++condition_id) {
    infectious_hosts[condition_id] = place->get_number_of_infectious_people(condition_id);
    total_infectious_hosts += infectious_hosts[condition_id];
  }
  
  FRED_VERBOSE(0, "infect_vectors on day %d in place %s susceptible_vectors %d total_inf_hosts %d\n",
	       day, place->get_label(), susceptible_vectors, total_infectious_hosts);

  // return if there are no infectious hosts
  if(total_infectious_hosts == 0) {
    return;
  }
  
  FRED_VERBOSE(1, "spread_infection::infect_vectors entered susceptible_vectors %d total_inf_hosts %d total_hosts %d\n",
	       susceptible_vectors, total_infectious_hosts,total_hosts);

  // the vector infection model of Chao and Longini

  // decide on total number of vectors infected by any infectious host

  // each vector's probability of infection
  double prob_infection = 1.0 - pow((1.0 - Global::Vectors->get_infection_efficiency()), (Global::Vectors->get_bite_rate() * total_infectious_hosts) / total_hosts);

  // select a number of vectors to be infected

  int total_infections = 0;
  if(susceptible_vectors > 0 && susceptible_vectors < 18){
    for(int k = 0; k < susceptible_vectors; k++){
      double r = Random::draw_random(0,1);
      if(r <= prob_infection){
	total_infections++;
      }
    }
  }else{
    total_infections = prob_infection * susceptible_vectors;
  }

  FRED_VERBOSE(1, "spread_infection::infect_vectors place %s day %d prob_infection %f total infections %d\n", place->get_label(),day,prob_infection,total_infections);

  // assign strain based on distribution of infectious hosts
  int newly_infected = 0;
  for(int condition_id = 0; condition_id < conditions; ++condition_id) {
    int exposed_vectors = total_infections *((double)infectious_hosts[condition_id] / (double)total_infectious_hosts);
    place->expose_vectors(condition_id, exposed_vectors);
    newly_infected += exposed_vectors;
  }
  place->mark_vectors_as_infected_today();
  if(Global::Vectors->get_vector_control_status()){
    FRED_VERBOSE(1, "Infect_vectors attempting to add infectious patch, day %d place %s\n",day,place->get_label());
    Global::Vectors->add_infectious_patch(place,day);
  }
  FRED_VERBOSE(1, "newly_infected_vectors %d\n", newly_infected);
}

void Vector_Transmission::infect_hosts(int day, int condition_id, Place* place) {

  int total_hosts = visitors.size();
  if(total_hosts == 0) {
    return;
  }

  int susceptible_hosts = susceptible_visitors.size();
  if(susceptible_hosts == 0){
    return;
  }

  int infectious_vectors = place->get_infectious_vectors(condition_id);
  if (infectious_vectors == 0) {
    return;
  }

  double transmission_efficiency = Global::Vectors->get_transmission_efficiency();
  if(transmission_efficiency == 0.0) {
    return;
  }

  int exposed_hosts = 0;

  double bite_rate = Global::Vectors->get_bite_rate();
  /*
  if(total_hosts <= 10){
    int actual_infections = 0;
    double number_of_bites_per_host = bite_rate * infectious_vectors / total_hosts;
    int min_number_of_bites_per_host = floor(number_of_bites_per_host);
    double remainder = number_of_bites_per_host - min_number_of_bites_per_host;
    for(int j = 0; j < visitors.size();++j){
      Person* infectee = visitors[j];
      int max_number_of_bites_per_host = min_number_of_bites_per_host;
      if(Random::draw_random(0,1) < remainder) {
	max_number_of_bites_per_host++;
      }
      if(infectee->is_susceptible(condition_id)) {
	bool effective_bite = false;
	for(int k=0;k<max_number_of_bites_per_host && effective_bite == false;k++){
	  if(Random::draw_random(0,1) < transmission_efficiency) {
	    effective_bite = true;
	  }
	}
	if(effective_bite){
	  // create a new infection in infectee
	  actual_infections++;
	  FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
	  infectee->become_exposed(condition_id, NULL, place, day);
	  Global::Conditions.get_condition(condition_id)->get_epidemic()->become_exposed(infectee, day);
	  int conditions = Global::Conditions.get_number_of_conditions();
	  if (conditions > 1) {
	    // for dengue: become unsusceptible to other conditions
	    for(int d = 0; d < conditions; d++) {
	      if(d != condition_id) {
		Condition* other_condition = Global::Conditions.get_condition(d);
		infectee->become_unsusceptible(other_condition);
		FRED_VERBOSE(1,"host %d not susceptible to condition %d\n", infectee->get_id(),d);
	      }
	    }
	  }
	}
      }
    }
    FRED_VERBOSE(0, "infect_hosts: place %s day %d number of bites %d number of bites %f  actual_infections %d infectious mosquitoes %d total hosts %d\n", place->get_label(),day,min_number_of_bites_per_host,number_of_bites_per_host,actual_infections,infectious_vectors,total_hosts);
  }else{
  */
  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - transmission_efficiency), (bite_rate * infectious_vectors / total_hosts));
  
  // select a number of hosts to be infected
  FRED_VERBOSE(1, "infect_hosts: place %s day %d  infectious_vectors %d prob_infection %f total_hosts %d\n", place->get_label(),day,infectious_vectors,prob_infection,total_hosts);
  //  double number_of_hosts_receiving_a_potentially_infectious_bite = susceptible_hosts * prob_infection;
  double number_of_hosts_receiving_a_potentially_infectious_bite = susceptible_hosts * prob_infection;
  int max_exposed_hosts = floor(number_of_hosts_receiving_a_potentially_infectious_bite);
  double remainder = number_of_hosts_receiving_a_potentially_infectious_bite - max_exposed_hosts;
  if(Random::draw_random(0,1) < remainder) {
    max_exposed_hosts++;
  }
  FRED_VERBOSE(1, "infect_hosts: place %s day %d  max_exposed_hosts[%d] = %d\n", place->get_label(),day,condition_id, max_exposed_hosts);
  // attempt to infect the max_exposed_hosts
  
  // randomize the order of processing the susceptible list
  std::vector<int> shuffle_index;
  shuffle_index.reserve(total_hosts);
  for(int i = 0; i < total_hosts; ++i) {
    shuffle_index[i] = i;
  }
  int actual_infections = 0;
  FYShuffle<int>(shuffle_index);
  
  // get the condition object   
  Condition* condition = Global::Conditions.get_condition(condition_id);
  for(int j = 0; j < max_exposed_hosts && j < susceptible_visitors.size(); ++j) {
    //for(int j = 0; j < max_exposed_hosts && j < visitors.size(); ++j) {
    Person* infectee = susceptible_visitors[shuffle_index[j]];
    //Person* infectee = visitors[shuffle_index[j]];
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());
    // NOTE: need to check if infectee already infected
    if(infectee->is_susceptible(condition_id)) {
      // create a new infection in infectee
      FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
      infectee->become_exposed(condition_id, NULL, place, day);
      actual_infections++;
      Global::Conditions.get_condition(condition_id)->get_epidemic()->become_exposed(infectee, day);
      int conditions = Global::Conditions.get_number_of_conditions();
      if (conditions > 1) {
	// for dengue: become unsusceptible to other conditions
	for(int d = 0; d < conditions; d++) {
	  if(d != condition_id) {
	    Condition* other_condition = Global::Conditions.get_condition(d);
	    infectee->become_unsusceptible(other_condition);
	    FRED_VERBOSE(1,"host %d not susceptible to condition %d\n", infectee->get_id(),d);
	  }
	}
      }
    } else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
  FRED_VERBOSE(1, "infect_hosts: place %s day %d  max_exposed_hosts[%d] = %d actual_infections %d\n", place->get_label(),day,condition_id, max_exposed_hosts,actual_infections);
}



