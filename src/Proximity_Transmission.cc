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
// File: Proximity_Transmission.cc
//
#include <algorithm>

#include "Proximity_Transmission.h"
#include "Condition.h"
#include "Global.h"
#include "Group.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"


Proximity_Transmission::Proximity_Transmission() {
}

Proximity_Transmission::~Proximity_Transmission() {
}

void Proximity_Transmission::setup(Condition* condition) {
}


/////////////////////////////////////////
//
// REQUIRED ENTRY POINT TO TRANSMISSION MODELS
//
/////////////////////////////////////////

void Proximity_Transmission::transmission(int day, int hour, int condition_id, Group* group, int time_block) {

  //Proximity_Transmission must occur on a Place type
  if(group==NULL || group->is_a_place()==false) {
    return;
  }

  Place* place = static_cast<Place*>(group);

  FRED_VERBOSE(1, "transmission day %d condition %d place %d %s\n",
	       day, condition_id, place->get_id(), place->get_label());

  // abort if transmissibility == 0 or if place is closed
  Condition* condition = Condition::get_condition(condition_id);
  double beta = condition->get_transmissibility();
  if(beta == 0.0) {
    FRED_VERBOSE(1, "no transmission beta %f\n", beta);
    return;
  }

  // have place record first and last day of possible transmission
  place->record_transmissible_days(day, condition_id);

  // need at least one susceptible
  if(place->get_size() == 0) {
    FRED_VERBOSE(1, "no transmission size = 0\n");
    return;
  }

  int new_exposures = 0;

  person_vector_t* transmissibles = place->get_transmissible_people(condition_id);
  int number_of_transmissibles = transmissibles->size();

  FRED_VERBOSE(1, "default_transmission DAY %d PLACE %s size %d trans %d\n",
	       day, place->get_label(), place->get_size(), number_of_transmissibles);

  // place-specific contact rate
  double contact_rate =  place->get_proximity_contact_rate();

  // scale by transmissibility of condition
  contact_rate *= condition->get_transmissibility();

  // take into number of hours in the time_block
  contact_rate *= time_block;

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
    Person* source = (*transmissibles)[source_pos];
    // printf("source id %d\n", source->get_id());

    if(source->is_transmissible(condition_id) == false) {
      // printf("source id %d not transmissible!\n", source->get_id());
      continue;
    }

    // get the actual number of contacts to attempt to infect
    double real_contacts = contact_rate * source->get_transmissibility(condition_id);

    // find integer contact count
    int contact_count = real_contacts;

    // randomly round off based on fractional part
    double fractional = real_contacts - contact_count;
    double r = Random::draw_random();
    if(r < fractional) {
      contact_count++;
    }

    if (contact_count == 0) {
      continue;
    }

    std::vector<Person*> target;
    // get a target for each contact attempt (with replacement)
    int count = 0;
    while (count < contact_count) {
      int pos = Random::draw_random_int(0, place->get_size() - 1);
      Person* other = place->get_member(pos);
      if(source != other) {
	target.push_back(other);
	count++;
      }
      else {
	if (place->get_size() > 1) {
	  continue; // try again
	}
	else {
	  break; // give up
	}
      }
    }

    int condition_to_transmit = condition->get_condition_to_transmit(source->get_state(condition_id));

    for(int count = 0; count < target.size(); count++) {
      Person* host = target[count];

      host->update_activities(day);
      if(!host->is_present(day, place)) {
	continue;
      }

      // get the transmission probs for given source/host pair
      double transmission_prob = 1.0;
      if(Global::Enable_Transmission_Bias) {
	double age_s = source->get_real_age();
	double age_h = host->get_real_age();
	double diff = fabs(age_h - age_s);
	double bias = place->get_proximity_same_age_bias();
	transmission_prob = exp(-bias * diff);
      }

      // only proceed if person is susceptible
      if(host->is_susceptible(condition_to_transmit)==false) {
	// printf("host person %d is not susceptible\n", host->get_id());
	continue;
      }

      if (Transmission::attempt_transmission(transmission_prob, source, host, condition_id, condition_to_transmit, day, hour, place)) {
	new_exposures++;
      }
      else {
	// printf("no exposure\n");
      }
    } // end contact loop
  } // end transmissible list loop

  if (0 && new_exposures > 0) {
    FRED_VERBOSE(1, "default_transmission DAY %d PLACE %s gives %d new_exposures\n",
		 day, place->get_label(), new_exposures);
  }

  FRED_VERBOSE(1, "transmission finished day %d condition %d place %d %s\n",
	       day, condition_id, place->get_id(), place->get_label());

  return;
}


