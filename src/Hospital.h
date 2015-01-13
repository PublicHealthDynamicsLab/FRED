/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Hospital.h
//

#ifndef _FRED_HOSPITAL_H
#define _FRED_HOSPITAL_H

#include "Place.h"

/**
 * This class represents a hospital location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Hospital : public Place {
public: 
  /**
   * Default constructor
   */
  Hospital() {}
  ~Hospital() {}

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, double lon, double lat, Place* cont, Population *pop)
   */
  Hospital(const char*,double,double,Place *, Population *);

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Hospital(int,const char *,double,double,Place *, Population *)</code>
   */
  void get_parameters(int diseases);

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per);

  /**
   * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Hospital_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>hospital_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Hospital_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>hospital_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  /**
   * Determine if the Hospital should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the household is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) { return true; }

private:
  static double * Hospital_contacts_per_day;
  static double *** Hospital_contact_prob;
  static bool Hospital_parameters_set;
};

#endif // _FRED_HOSPITAL_H

