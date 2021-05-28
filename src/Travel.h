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
// File: Travel.h
//

#ifndef _FRED_TRAVEL_H
#define _FRED_TRAVEL_H

typedef std::vector <Person*> pvec;		// vector of person ptrs

// travel hub record:
typedef struct hub_record {
  int id;
  double lat;
  double lon;
  pvec users;
  int pop;
  int pct;
} hub_t;


class Events;
class Person;
class Age_Map;


class Travel {
public:
  static void get_properties();
  static void setup(char* directory);
  static void read_hub_file();
  static void read_trips_per_day_file();
  static void setup_travelers_per_hub();
  static void setup_travel_lists();
  static void update_travel(int day);
  static void find_returning_travelers(int day);
  static void old_find_returning_travelers(int day);
  static void quality_control(char* directory);
  static void terminate_person(Person* per);
  static void add_return_event(int day, Person* person);
  static void delete_return_event(int day, Person* person);

private:
  static Events * return_queue;
};

#endif // _FRED_TRAVEL_H
