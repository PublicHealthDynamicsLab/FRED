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
// File: Travel.h
//

#ifndef _FRED_TRAVEL_H
#define _FRED_TRAVEL_H

#include "Events.h"
class Person;

class Travel {
public:
  static void setup(char* directory);
  static void read_hub_file();
  static void read_trips_per_day_file();
  static void setup_travelers_per_hub();
  static void setup_travel_lists();
  static void update_travel(int day);
  static void find_returning_travelers(int day);
  static void old_find_returning_travelers(int day);
  static void return_handler(int day, Person* person);
  static void quality_control(char* directory);
  static void terminate_person(Person* per);

  static void add_return_event(int day, Person* person) {
    Travel::return_queue->add_event(day, person);
  }

  static void delete_return_event(int day, Person* person) {
    Travel::return_queue->delete_event(day, person);
  }

private:
  static Events* return_queue;
};

#endif // _FRED_TRAVEL_H






