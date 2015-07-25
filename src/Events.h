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
class Epidemic;

#define MAX_DAYS (100*366)

typedef Person* event_t;
typedef std::vector<event_t> events_t;
typedef events_t::iterator events_itr_t;

// the following definitions are based on advice from
// https://isocpp.org/wiki/faq/pointers-to-members
typedef void (Epidemic::*EpidemicMemFn)(int day, event_t e);
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))


class Events {

public:

  Events() {
    for (int day = 0; day < MAX_DAYS; ++day) {
      clear_events(day);
    }
  }

  ~Events(){}

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


  void delete_event(int day, event_t item) {
    if (day < 0 || MAX_DAYS <= day) {
      // won't happen during this simulation
      return;
    }
    // find item in the list
    int size = get_size(day);
    for(int pos = 0; pos < size; ++pos) {
      if(this->events[day][pos] == item) {
	// copy last item in list into this slot
        this->events[day][pos] = this->events[day].back();
	// delete last slot
        this->events[day].pop_back();
	// printf("\ndelete_event day %d final size %d\n", day, get_size(day));
	// print_events(day);
	return;
      }
    }
    // item not found
    printf("delete_events: item not found\n");
    assert(false);
  }

  void clear_events(int day) {
    assert(0 <= day && day < MAX_DAYS);
    this->events[day] = events_t();
    // printf("clear_events day %d size %d\n", day, get_size(day));
  }

  int get_size(int day) {
    assert(0 <= day && day < MAX_DAYS);
    return static_cast<int>(this->events[day].size());
  }

  void print_events(FILE* fp, int day);
  void print_events(int day);

  void event_handler(int day, void (*func)(int,event_t)) {
    events_itr_t itr_end = this->events[day].end();
    for(events_itr_t itr = this->events[day].begin(); itr != itr_end; ++itr) {
      func(day, *itr);
    }
    clear_events(day);
  }

  void event_handler(int day, Epidemic * epidemic, EpidemicMemFn handler) {
    events_itr_t itr_end = this->events[day].end();
    for(events_itr_t itr = this->events[day].begin(); itr != itr_end; ++itr) {
      CALL_MEMBER_FN(*epidemic, handler)(day, *itr);
    }
    clear_events(day);
  }

private:
  events_t events[MAX_DAYS];

};


#endif /* EVENTS_H_ */
