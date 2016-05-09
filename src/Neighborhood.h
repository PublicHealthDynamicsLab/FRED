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
// File: Neighborhood.h
//

#ifndef _FRED_NEIGHBORHOOD_H
#define _FRED_NEIGHBORHOOD_H

#include "Place.h"

/**
 * This class represents a neighborhood location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Neighborhood : public Place {
public: 

  /**
   * Default constructor
   */
  Neighborhood() : Place() {}

  /**
   * Constructor with necessary parameters
   */
  Neighborhood(const char* lab, char _subtype, fred::geo lon, fred::geo lat);

  ~Neighborhood() {};

  static void get_parameters();

  /**
   * @see Place::get_group(int condition, Person* per)
   */
  int get_group(int condition, Person* per);

  double get_transmission_probability(int condition, Person* i, Person* s);

  /**
   * @see Place::get_transmission_prob(int condition, Person* i, Person* s)
   *
   * This method returns the value from the static array <code>Neighborhood::Neighborhood_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Neighborhood_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>neighborhood_prob[]</code>.
   */
  double get_transmission_prob(int condition, Person* i, Person* s);

  /**
   * @see Place::get_contacts_per_day(int condition)
   *
   * This method returns the value from the static array <code>Neighborhood::Neighborhood_contacts_per_day</code>
   * that corresponds to a particular condition.<br />
   * The static array <code>Neighborhood_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>neighborhood_contacts[]</code>.
   */
  double get_contacts_per_day(int condition);

  /**
   * Determine if the neighborhood should be open. It is dependent on the condition and simulation day.
   *
   * @param day the simulation day
   * @param condition an integer representation of the condition
   * @return whether or not the neighborhood is open on the given day for the given condition
   */
  bool should_be_open(int day, int condition) {
    return true;
  }

  /**
   * Returns the rate by which to increase neighborhood contacts on weekends
   *
   * @return the rate by which to increase neighborhood contacts on weekends
   */
  static double get_weekend_contact_rate(int condition) {
    return Neighborhood::weekend_contact_rate;
  }

private:
  static double contacts_per_day;
  static double same_age_bias;
  static double** prob_transmission_per_contact;
  static double weekend_contact_rate;
};

#endif // _FRED_NEIGHBORHOOD_H

