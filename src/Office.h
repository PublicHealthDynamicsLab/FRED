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
// File: Office.h
//

#ifndef _FRED_OFFICE_H
#define _FRED_OFFICE_H

#include "Place.h"

class Workplace;


/**
 * This class represents an office location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Office: public Place {
public: 

  /**
   * Default constructor
   */
  Office() {}
  ~Office() {}

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, fred::geo lon, fred::geo lat, Place* cont)
   */
  Office( const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat);

  static void get_parameters();

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per) {
    return 0;
  }

  int get_container_size();

  /**
   * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
   *
   * This method returns the value from the static array <code>Office::Office_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Office_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>office_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Office::Office_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Office_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>office_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  /**
   * Determine if the office should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the office is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) {
    return true;
  }

  void set_workplace(Workplace* _workplace) {
    workplace = _workplace;
  }

  Workplace * get_workplace() {
    return workplace;
  }

private:
  static double * Office_contacts_per_day;
  static double *** Office_contact_prob;
  Workplace *workplace;
};

#endif // _FRED_OFFICE_H

