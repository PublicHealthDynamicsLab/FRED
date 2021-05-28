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
// File: Events.cc
//

#include "Events.h"
#include "Global.h"
#include "Utils.h"

Events::Events() {
  this->event_queue_size = 24*Global::Simulation_Days;
  this->events = new events_t [ event_queue_size ];
  for (int step = 0; step < this->event_queue_size; ++step) {
    clear_events(step);
  }

}

void Events::add_event(int step, event_t item) {

  if (step < 0 || this->event_queue_size <= step) {
    // won't happen during this simulation
    return;
  }
  if(this->events[step].size() == this->events[step].capacity()) {
    if(this->events[step].capacity() < 4) {
      this->events[step].reserve(4);
    }
    this->events[step].reserve(2 * this->events[step].capacity());
  }
  this->events[step].push_back(item);
  // printf("\nadd_event step %d new size %d\n", step, get_size(step));
  // print_events(step);
}

void Events::delete_event(int step, event_t item) {

  if(step < 0 || this->event_queue_size <= step) {
    // won't happen during this simulation
    return;
  }
  // find item in the list
  int size = get_size(step);
  for(int pos = 0; pos < size; ++pos) {
    if(this->events[step][pos] == item) {
      // copy last item in list into this slot
      this->events[step][pos] = this->events[step].back();
      // delete last slot
      this->events[step].pop_back();
      // printf("\ndelete_event step %d final size %d\n", step, get_size(step));
      // print_events(step);
      return;
    }
  }
  // item not found
  FRED_WARNING("delete_events: item not found\n");
  assert(false);
}

void Events::clear_events(int step) {
  assert(0 <= step && step < this->event_queue_size);
  this->events[step] = events_t();
  // printf("clear_events step %d size %d\n", step, get_size(step));
}

int Events::get_size(int step) {
  assert(0 <= step && step < this->event_queue_size);
  return static_cast<int>(this->events[step].size());
}

event_t Events::get_event(int step, int i) {
  assert(0 <= step && step < this->event_queue_size);
  if (0 <= i && i < static_cast<int>(this->events[step].size())) {
    return this->events[step][i];
  }
  else {
    Utils::fred_abort("get_event: i = %d size = %d\n",
		      i, (int)this->events[step].size());
    return NULL;
  }
}


void Events::print_events(FILE* fp, int step) {
  assert(0 <= step && step < this->event_queue_size);
  events_itr_t itr_end = this->events[step].end();
  fprintf(fp, "events[%d] = %d : ", step, get_size(step));
  for(events_itr_t itr = this->events[step].begin(); itr != itr_end; ++itr) {
    // fprintf(fp, "id %d age %d ", (*itr)->get_id(), (*itr)->get_age());
  }
  fprintf(fp,"\n");
  fflush(fp);
}

void Events::print_events(int step) {
  print_events(stdout, step);
}

