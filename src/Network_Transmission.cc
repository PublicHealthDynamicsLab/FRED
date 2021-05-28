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
// File: Network_Transmission.cc
//

#include "Network_Transmission.h"
#include "Condition.h"
#include "Epidemic.h"
#include "Network.h"
#include "Person.h"
#include "Random.h"
#include "Utils.h"

//////////////////////////////////////////////////////////
//
// NETWORK TRANSMISSION MODEL
//
//////////////////////////////////////////////////////////

void Network_Transmission::transmission(int day, int hour, int condition_id, Group* group, int time_block) {

  FRED_VERBOSE(0, "network_transmission: day %d hour %d network %s time_block  = %d\n",
      day, hour, group ? group->get_label() : "NULL", time_block);

  if(group == NULL || group->is_a_network()==false) {
    return;
  }

  Network* network = static_cast<Network*>(group);

  Condition* condition = Condition::get_condition(condition_id);
  double beta = condition->get_transmissibility();
  if(beta == 0.0) {
    FRED_VERBOSE(0, "no transmission beta %f\n", beta);
    return;
  }

  int new_exposures = 0;

  person_vector_t* transmissible = group->get_transmissible_people(condition_id);
  int number_of_transmissibles = transmissible->size();

  FRED_VERBOSE(0, "network_transmission: day %d hour %d network %s transmissibles %d\n",
      day, hour, group->get_label(), (int) transmissible->size());

  // randomize the order of processing the transmissible list
  std::vector<int> shuffle_index;
  shuffle_index.clear();
  shuffle_index.reserve(number_of_transmissibles);
  for(int i = 0; i < number_of_transmissibles; ++i) {
    shuffle_index.push_back(i);
  }
  FYShuffle<int>(shuffle_index);

  for(int n = 0; n < number_of_transmissibles; ++n) {
    int source_pos = shuffle_index[n];
    // transmissible visitor
    Person* source = (*transmissible)[source_pos];
    FRED_DEBUG(0, "source id %d\n", source->get_id());

    if(source->is_transmissible(condition_id) == false) {
      FRED_DEBUG(0, "source id %d not transmissible!\n", source->get_id());
      continue;
    }

    // get the other agents connected to the source
    person_vector_t other = source->get_outward_edges(network, 1);
    int others = other.size();
    FRED_DEBUG(0, "source id %d has %d out_links\n", source->get_id(), others);
    if(others == 0) {
      FRED_DEBUG(0, "no available others\n");
      continue;
    }

    // determine how many contacts to attempt
    double real_contacts = 0;
    if(group->get_contact_count(condition_id) > 0) {
      real_contacts = group->get_contact_count(condition_id);
    } else {
      real_contacts = group->get_contact_rate(condition_id) * others;
    }
    real_contacts *= time_block * beta * source->get_transmissibility(condition_id);
    
    // find integer contact count
    int contact_count = real_contacts;
    
    // randomly round off based on fractional part
    double fractional = real_contacts - contact_count;
    double r = Random::draw_random();
    if(r < fractional) {
      contact_count++;
    }
    
    if(contact_count == 0) {
      continue;
    }

    int condition_to_transmit = condition->get_condition_to_transmit(source->get_state(condition_id));

    std::vector<int> shuffle_index;
    if(group->use_deterministic_contacts(condition_id)) {
      // randomize the order of contacts among others
      shuffle_index.clear();
      shuffle_index.reserve(others);
      for(int i = 0; i < others; ++i) {
        shuffle_index.push_back(i);
      }
      FYShuffle<int>(shuffle_index);
    }

    // get a destination for each contact
    for(int count = 0; count < contact_count; ++count) {
      Person* host = NULL;
      int pos = 0;
      if (group->use_deterministic_contacts(condition_id)) {
        // select an agent from among the others without replacement
        pos = shuffle_index[count % others];
      } else {
        // select an agent from among the others with replacement
        pos = Random::draw_random_int(0, others - 1);
      }

      host = other[pos];
      FRED_DEBUG(0, "source id %d target id %d\n", source->get_id(), host->get_id());
      host->update_activities(day);
      if(host->is_present(day, group) == false) {
        continue;
      }

      // only proceed if person is susceptible
      if(host->is_susceptible(condition_to_transmit)==false) {
        FRED_DEBUG(0, "host person %d is not susceptible\n", host->get_id());
        continue;
      }

      // attempt transmission
      double transmission_prob = 1.0;
      if(Transmission::attempt_transmission(transmission_prob, source, host, condition_id, condition_to_transmit, day, hour, group)) {
        new_exposures++;
      } else {
        FRED_DEBUG(0, "no exposure\n");
      }
    } // end contact loop
  } // end transmissible list loop

  if(new_exposures > 0) {
    FRED_VERBOSE(0, "network_transmission day %d hour %d network %s gives %d new_exposures\n", day, hour, group->get_label(), new_exposures);
  }

  FRED_VERBOSE(1, "transmission finished day %d condition %d network %s\n",
      day, condition_id, network->get_label());

  return;
}

