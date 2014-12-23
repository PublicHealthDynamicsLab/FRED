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

class Person;

class Travel {
 public:
  static void setup(char * directory);
  static void read_hub_file();
  static void read_trips_per_day_file();
  static void setup_travelers_per_hub();
  static void setup_travel_lists();
  static void update_travel(int day);
  static void quality_control(char * directory);
  static void terminate_person(Person *per);
};

#endif // _FRED_TRAVEL_H






