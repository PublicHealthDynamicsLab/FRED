/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
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

#define MAX_DAYS (100*366)

// type definitions:
typedef Person* event_t;
typedef std::vector<event_t> events_t;
typedef events_t::iterator events_itr_t;

class Events {

public:

  Events();
  ~Events(){}

  void add_event(int day, event_t item);
  void delete_event(int day, event_t item);
  void clear_events(int day);
  int get_size(int day);
  event_t get_event(int day, int i);
  void print_events(FILE* fp, int day);
  void print_events(int day);

private:
  events_t events[MAX_DAYS];
};


#endif /* EVENTS_H_ */
