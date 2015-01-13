/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Travel.h
//

#ifndef _FRED_TRAVEL_H
#define _FRED_TRAVEL_H

#include <stdio.h>
class Person;

class Travel {
 public:

  /**
   * Initialize travel mode: read runtime parameters and compute 
   * travel probabilities between each large cell using gravity model.
   */
  static void setup(char * directory);

  /**
   * Perform a daily travel updates.  Select a number of new trips to instantiate.
   * For each trip, select source and destination cells using gravity model.
   * Select a random person from the source cell as the traveler.
   * Select a random person from the destination cell to be visited.
   * The traveler shares the household, neighborhood and possibly workplace
   * of the person visited.  The duration of the trip is selected from
   * the runtime trip_duration cdf.
   * When a traveler's trip is finished, the traveler returns to the
   * original set of activities.
   */
  static void update_travel(int day);

  /**
   * Select source and destination cells using gravity model.
   * Select a random person from the source cell as the traveler.
   * Select a random person from the destination cell to be visited.
   */
  static void select_visitor_and_visited(Person **visitor, Person **visited, int day);

  /**
   * Creates a sample of trip using the gravity model, prints statistics,
   * and terminates FRED.
   */
  static void test_gravity_model();


  /**
   * Prints statistics for gravity travel model.
   */
  static void quality_control(char * directory);

  static void terminate_person(Person *per);
};

#endif // _FRED_TRAVEL_H






