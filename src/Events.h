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

// the following definition is based on advice from
// https://isocpp.org/wiki/faq/pointers-to-members
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

template <class T>
class Events {

public:

  // the following definition is based on advice from
  // https://isocpp.org/wiki/faq/pointers-to-members
  typedef void (T::*TMemFn)(int day, event_t e);

  Events<T>();
  ~Events<T>(){}

  void add_event(int day, event_t item) {
    if (day < 0 || MAX_DAYS <= day) {
      // won't happen during this simulation
      return;
    }
    if(this->events[day].size() == this->events[day].capacity()) {
      if(events[day].capacity() < 4) {
	this->events[day].reserve(4);
      }
      this->events[day].reserve(2 * this->events[day].capacity());
    }
    this->events[day].push_back(item);
    // printf("\nadd_event day %d new size %d\n", day, get_size(day));
    // print_events(day);
  }

  void delete_event(int day, event_t item);
  void clear_events(int day);
  int get_size(int day);
  void print_events(FILE* fp, int day);
  void print_events(int day);
  void event_handler(int day, void (*func)(int,event_t));
  void event_handler(int day, T * obj, TMemFn handler);

private:
  events_t events[MAX_DAYS];
};


#endif /* EVENTS_H_ */
