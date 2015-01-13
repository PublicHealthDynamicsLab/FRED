/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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
  Neighborhood() {};
  ~Neighborhood() {};

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, double lon, double lat, Place* cont, Population *pop )
   */
  Neighborhood( const char *lab, double lon, double lat, Place *container, Population* pop );

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Neighborhood(int loc, const char *lab, double lon, double lat, Place *container, Population* pop)</code>
   */
  void get_parameters(int diseases);

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per);

  /**
   * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
   *
   * This method returns the value from the static array <code>Neighborhood::Neighborhood_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Neighborhood_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>neighborhood_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Neighborhood::Neighborhood_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Neighborhood_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>neighborhood_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  /**
   * Determine if the neighborhood should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the neighborhood is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) { return true; }

  /**
   * Returns the rate by which to increase neighborhood contacts on weekends
   *
   * @return the rate by which to increase neighborhood contacts on weekends
   */
  static double get_weekend_contact_rate(int disease) { return Weekend_contact_rate[disease]; }

private:
  static double * Weekend_contact_rate;
  static double * Neighborhood_contacts_per_day;
  static double *** Neighborhood_contact_prob;
  static bool Neighborhood_parameters_set;
};

#endif // _FRED_NEIGHBORHOOD_H

