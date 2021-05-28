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
// File: Transmission.cc
//

#include <math.h>

#include "Transmission.h"
#include "Condition.h"
#include "Epidemic.h"
#include "Group.h"
#include "Person.h"
#include "Environmental_Transmission.h"
#include "Network_Transmission.h"
#include "Proximity_Transmission.h"
#include "Property.h"
#include "Random.h"
#include "Utils.h"

Transmission* Transmission::get_new_transmission(char* transmission_mode) {
  
  if(strcmp(transmission_mode, "respiratory")==0 || strcmp(transmission_mode, "proximity")==0) {
    printf("new Proximity_Transmission\n");
    return new Proximity_Transmission();
  }
  
  if(strcmp(transmission_mode, "network") == 0) {
    printf("new Network_Transmission\n");
    return new Network_Transmission();
  }

  if(strcmp(transmission_mode, "environmental") == 0) {
    printf("new Environmental_Transmission\n");
    return new Environmental_Transmission();
  }
  
  if(strcmp(transmission_mode, "none") == 0) {
    printf("new Null_Transmission\n");
    return new Null_Transmission();
  }

  Utils::fred_abort("Unknown transmission_mode (%s).\n", transmission_mode);
  return NULL;
}


bool Transmission::attempt_transmission(double transmission_prob, Person* source, Person* dest,
					int condition_id, int condition_to_transmit, int day, int hour, Group* group) {
  
  assert(dest->is_susceptible(condition_to_transmit));
  FRED_STATUS(1, "source %d -- dest %d is susceptible\n", source->get_id(), dest->get_id());

  double susceptibility = dest->get_susceptibility(condition_to_transmit);

  FRED_VERBOSE(2, "susceptibility = %f\n", susceptibility);

  double r = Random::draw_random();
  double infection_prob = transmission_prob * susceptibility;

  if(r < infection_prob) {
    // successful transmission; create a new infection in dest
    source->expose(dest, condition_id, condition_to_transmit, group, day, hour);

    // FRED_VERBOSE(1, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob);
    if (source->get_exposure_day(condition_id) == 0) {
      // FRED_VERBOSE(1, "SEED infection day %i from %d to %d\n", day, source->get_id(), dest->get_id());
    }
    else {
      // FRED_VERBOSE(1, "infection day %i of condition %i from %d to %d prob %f\n",
      // day, condition_to_transmit, source->get_id(), dest->get_id(), transmission_prob);
    }
    if (infection_prob > 1) {
      // FRED_VERBOSE(0, "infection_prob exceeded unity! trans %f susc %f\n", transmission_prob, susceptibility);
    }

    // notify the epidemic
    Condition::get_condition(condition_to_transmit)->get_epidemic()->become_exposed(dest, day, hour);

    return true;
  } else {
    // FRED_VERBOSE(1, "transmission failed: r = %f  prob = %f\n", r, infection_prob);
    return false;
  }
}

