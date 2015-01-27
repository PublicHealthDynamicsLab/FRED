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
class Person;

using namespace std;

#define MAX_DAYS (100*366)

typedef Person* event_t;
typedef std::vector<event_t> events_t;
typedef events_t::iterator events_itr_t;

class Events {

public:

  Events() {
    for (int day = 0; day < MAX_DAYS; day++) {
      clear_events(day);
    }
  }

  ~Events(){}

  void add_event(int day, event_t item) {
    assert(0 <= day && day < MAX_DAYS);
    if (events[day].size() == events[day].capacity()) {
      if (events[day].capacity() < 4) {
	events[day].reserve(4);
      }
      events[day].reserve(2*events[day].capacity());
    }
    events[day].push_back(item);
    // printf("\nadd_event day %d new size %d\n", day, get_size(day));
    // print_events(day);
  }


  void delete_event(int day, event_t item) {
    assert(0 <= day && day < MAX_DAYS);
    // find item in the list
    int size = get_size(day);
    for (int pos = 0; pos < size; pos++) {
      if (events[day][pos] == item) {
	// copy last item in list into this slot
	events[day][pos] = events[day].back();
	// delete last slot
	events[day].pop_back();
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
    events[day] = events_t();
    // printf("clear_events day %d size %d\n", day, get_size(day));
  }

  int get_size(int day) {
    assert(0 <= day && day < MAX_DAYS);
    return (int) events[day].size();
  }

  void print_events(FILE *fp, int day);
  void print_events(int day);

  void event_handler(int day, void (*func)(int,event_t)) {
    events_itr_t itr_end = events[day].end();
    for (events_itr_t itr = events[day].begin(); itr != itr_end; itr++) {
      func(day, *itr);
    }
    clear_events(day);
  }

 private:
  events_t events[MAX_DAYS];

};


/*
#include <assert.h>
#include <tr1/unordered_set>
#include <stdio.h>
#include <vector>
class Person;

using namespace std;

#define MAX_DAYS (100*366)

typedef std::tr1::unordered_set<Person *> events_t;
typedef events_t::iterator events_itr_t;

class Events {

public:

  Events() {
    for (int day = 0; day < MAX_DAYS; day++) {
      clear_events(day);
    }
  }

  ~Events(){}

  void add_event(int day, Person * item) {
    assert(0 <= day && day < MAX_DAYS);
    events[day].insert(item);
    printf("\nadd_event day %d new size %d\n", day, get_number_of_events(day));
    print_events(day);
  }


  void delete_event(int day, Person * item) {
    assert(0 <= day && day < MAX_DAYS);
    events[day].erase(item);
    printf("\ndelete_event day %d final size %d\n", day, get_number_of_events(day));
    print_events(day);
  }

  void clear_events(int day) {
    assert(0 <= day && day < MAX_DAYS);
    events[day] = std::tr1::unordered_set<Person *>();
    printf("clear_events day %d size %d\n", day, get_number_of_events(day));
  }

  std::vector <Person *> get_events(int day) {
    std::vector <Person *> vec;
    size_t size = events[day].size();
    vec.reserve(size);
    vec.clear();
    assert(0 <= day && day < MAX_DAYS);
    events_itr_t itr_end = events[day].end();
    for (events_itr_t itr = events[day].begin(); itr != itr_end; itr++) {
      vec.push_back(*itr);
    }
    return vec;
  }

  int get_number_of_events(int day) {
    assert(0 <= day && day < MAX_DAYS);
    return (int) events[day].size();
  }

  size_t get_size(int day) {
    assert(0 <= day && day < MAX_DAYS);
    return events[day].size();
  }

  void print_events(FILE *fp, int day);
  void print_events(int day);

 private:
  events_t events[MAX_DAYS];

};
*/

#endif /* EVENTS_H_ */
