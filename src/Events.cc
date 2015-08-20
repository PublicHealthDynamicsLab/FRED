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
// File: Events.cc
//

#include "Events.h"
#include "Person.h"

template <class T>
Events<T>::Events() {

  for (int day = 0; day < MAX_DAYS; ++day) {
    clear_events(day);
  }

}

template <class T>
void Events<T>::delete_event(int day, event_t item) {

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

template <class T>
void Events<T>::clear_events(int day) {

  assert(0 <= day && day < MAX_DAYS);
  this->events[day] = events_t();
  // printf("clear_events day %d size %d\n", day, get_size(day));

}

template <class T>
int Events<T>::get_size(int day) {

  assert(0 <= day && day < MAX_DAYS);
  return static_cast<int>(this->events[day].size());

}

template <class T>
void Events<T>::event_handler(int day, void (*func)(int,event_t)) {

  events_itr_t itr_end = this->events[day].end();
  for(events_itr_t itr = this->events[day].begin(); itr != itr_end; ++itr) {
    func(day, *itr);
  }
  clear_events(day);

}

template <class T>
void Events<T>::event_handler(int day, T * obj, TMemFn handler) {

  events_itr_t itr_end = this->events[day].end();
  for(events_itr_t itr = this->events[day].begin(); itr != itr_end; ++itr) {
    CALL_MEMBER_FN(*obj, handler)(day, *itr);
  }
  clear_events(day);

}


template <class T>
void Events<T>::print_events(FILE* fp, int day) {

  assert(0 <= day && day < MAX_DAYS);
  events_itr_t itr_end = this->events[day].end();
  fprintf(fp, "events[%d] = %d : ", day, get_size(day));
  for (events_itr_t itr = events[day].begin(); itr != itr_end; ++itr) {
    fprintf(fp, "id %d age %d ", (*itr)->get_id(), (*itr)->get_age());
  }
  fprintf(fp,"\n");
  fflush(fp);

}

template <class T>
void Events<T>::print_events(int day) {

  print_events(stdout, day);

}

class Demographics;
class Epidemic;
class Travel;

template class Events<Demographics>;
template class Events<Epidemic>;
template class Events<Travel>;

