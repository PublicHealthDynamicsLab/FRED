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
// File: Events.h
//

#ifndef EVENTS_H_
#define EVENTS_H_

#include <assert.h>
#include <stdio.h>
#include <vector>

using namespace std;

class Person;

// type definitions:
typedef Person* event_t;
typedef std::vector<event_t> events_t;
typedef events_t::iterator events_itr_t;

class Events {

public:

  Events();
  ~Events(){}

  void add_event(int step, event_t item);
  void delete_event(int step, event_t item);
  void clear_events(int step);
  int get_size(int step);
  event_t get_event(int step, int i);
  void print_events(FILE* fp, int step);
  void print_events(int step);

private:
  int event_queue_size;
  events_t* events;
};


#endif /* EVENTS_H_ */
