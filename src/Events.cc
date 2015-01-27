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

void Events::print_events(FILE *fp, int day) {
  assert(0 <= day && day < MAX_DAYS);
  events_itr_t itr_end = events[day].end();
  fprintf(fp, "events[%d] = %d : ", day, get_size(day));
  for (events_itr_t itr = events[day].begin(); itr != itr_end; itr++) {
    fprintf(fp, "id %d age %d ", (*itr)->get_id(), (*itr)->get_age());
  }
  fprintf(fp,"\n");
  fflush(fp);
}

void Events::print_events(int day) {
  print_events(stdout, day);
}


