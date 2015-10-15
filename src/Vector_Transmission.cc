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

#include "Vector_Transmission.h"
#include "Date.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "Vector_Layer.h"
#include "Utils.h"

//////////////////////////////////////////////////////////
//
// VECTOR TRANSMISSION MODEL
//
//////////////////////////////////////////////////////////

void Vector_Transmission::setup(Disease *disease) {
}

void Vector_Transmission::spread_infection(int day, int disease_id, Place * place) {

  // abort if transmissibility == 0 or if place is closed
  Disease* disease = Global::Diseases.get_disease(disease_id);
  double beta = disease->get_transmissibility();
  if(beta == 0.0 || place->is_open(day) == false || place->should_be_open(day, disease_id) == false) {
    place->reset_place_state(disease_id);
    return;
  }

  // have place record first and last day of infectiousness
  place->record_infectious_days(day);

  visitors.clear();

  person_vec_t * tmp = place->get_enrollees();
  int tmp_size = tmp->size();
  for (int i = 0; i < tmp_size; i++) {
    Person *person = (*tmp)[i];
    person->update_schedule(day);
    if (person->is_present(day, place)) {
      visitors.push_back(person);
    }
  }
  FRED_VERBOSE(1, "Vector_Transmission::spread_infection entered place %s visitors %d\n",place->get_label(),visitors.size());
  // infections of vectors by hosts
  if(place->have_vectors_been_infected_today() == false) {
    infect_vectors(day, place);
  }

  // transmission from vectors to hosts
  infect_hosts(day, disease_id, place);

  place->reset_place_state(disease_id);
}


void Vector_Transmission::infect_vectors(int day, Place * place) {
  
  int total_hosts = visitors.size();
  if(total_hosts == 0) {
    return;
  }

  // skip if there are no susceptible vectors
  int susceptible_vectors = place->get_susceptible_vectors();
  if(susceptible_vectors == 0) {
    return;
  }
  
  // find the percent distribution of infectious hosts
  int diseases = Global::Diseases.get_number_of_diseases();
  int* infectious_hosts = new int [diseases];
  int total_infectious_hosts = 0;
  for(int disease_id = 0; disease_id < diseases; ++disease_id) {
    infectious_hosts[disease_id] = place->get_number_of_infectious_people(disease_id);
    total_infectious_hosts += infectious_hosts[disease_id];
  }
  
  // return if there are no infectious hosts
  if(total_infectious_hosts == 0) {
    return;
  }
  
  FRED_VERBOSE(1, "spread_infection::infect_vectors entered susceptible_vectors %d total_inf_hosts %d\n",
	       susceptible_vectors, total_infectious_hosts);

  // the vector infection model of Chao and Longini

  // decide on total number of vectors infected by any infectious host

  // each vector's probability of infection
  double prob_infection = 1.0 - pow((1.0 - Global::Vectors->get_infection_efficiency()), (Global::Vectors->get_bite_rate() * total_infectious_hosts) / total_hosts);

  // select a number of vectors to be infected

  int total_infections = 0;
  if(susceptible_vectors > 0 && susceptible_vectors < 18){
    for(int k = 0; k < susceptible_vectors; k++){
      double r = Random::draw_random();
      if(r < prob_infection){
	total_infections++;
      }
    }
  }else{
    total_infections = prob_infection * susceptible_vectors;
  }

  FRED_VERBOSE(1, "spread_infection::infect_vectors place %s day %d prob_infection %f total infections %d\n", place->get_label(),day,prob_infection,total_infections);

  // assign strain based on distribution of infectious hosts
  int newly_infected = 0;
  for(int disease_id = 0; disease_id < diseases; ++disease_id) {
    int exposed_vectors = total_infections *((double)infectious_hosts[disease_id] / (double)total_infectious_hosts);
    place->expose_vectors(disease_id, exposed_vectors);
    newly_infected += exposed_vectors;
  }
  place->mark_vectors_as_infected_today();
  FRED_VERBOSE(1, "newly_infected_vectors %d\n", newly_infected);
}

void Vector_Transmission::infect_hosts(int day, int disease_id, Place * place) {

  int total_hosts = visitors.size();
  if(total_hosts == 0) {
    return;
  }

  int infectious_vectors = place->get_infectious_vectors(disease_id);
  if (infectious_vectors == 0) {
    return;
  }

  double transmission_efficiency = Global::Vectors->get_transmission_efficiency();
  if(transmission_efficiency == 0.0) {
    return;
  }

  int exposed_hosts = 0;
  double bite_rate = Global::Vectors->get_bite_rate();

  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - transmission_efficiency), (bite_rate * infectious_vectors / total_hosts));
  
  // select a number of hosts to be infected
  double expected_infections = total_hosts * prob_infection;
  int max_exposed_hosts = floor(expected_infections);
  double remainder = expected_infections - max_exposed_hosts;
  if(Random::draw_random() < remainder) {
    max_exposed_hosts++;
  }
  FRED_VERBOSE(1, "infect_hosts: place %s day %d  max_exposed_hosts[%d] = %d\n", place->get_label(),day,disease_id, max_exposed_hosts);

  // attempt to infect the max_exposed_hosts

  // randomize the order of processing the susceptible list
  std::vector<int> shuffle_index;
  shuffle_index.reserve(total_hosts);
  for (int i = 0; i < total_hosts; i++) {
    shuffle_index[i] = i;
  }
  FYShuffle<int>(shuffle_index);

  // get the disease object   
  Disease* disease = Global::Diseases.get_disease(disease_id);

  for(int j = 0; j < max_exposed_hosts; ++j) {
    Person* infectee = visitors[shuffle_index[j]];
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());
    // NOTE: need to check if infectee already infected
    if(infectee->is_susceptible(disease_id)) {
      // create a new infection in infectee
      FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
      infectee->become_exposed(disease_id, NULL, place, day);
      Global::Diseases.get_disease(disease_id)->get_epidemic()->become_exposed(infectee, day);
      int diseases = Global::Diseases.get_number_of_diseases();
      if (diseases > 1) {
	// for dengue: become unsusceptible to other diseases
	for(int d = 0; d < diseases; d++) {
	  if(d != disease_id) {
	    Disease* other_disease = Global::Diseases.get_disease(d);
	    infectee->become_unsusceptible(other_disease);
	    FRED_VERBOSE(1,"host %d not susceptible to disease %d\n", infectee->get_id(),d);
	  }
	}
      }
    }
    else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
}



