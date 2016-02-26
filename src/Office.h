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
   * Note: really only used by Allocator
   */
  Office();

  /**
   * Constructor with necessary parameters
   */
  Office(const char* lab, char _subtype, fred::geo lon, fred::geo lat);

  ~Office() {}

  static void get_parameters();

  /**
   * @see Place::get_group(int condition, Person* per)
   */
  int get_group(int condition, Person* per) {
    return 0;
  }

  int get_container_size();

  /**
   * @see Place::get_transmission_prob(int condition, Person* i, Person  s)
   *
   * This method returns the value from the static array <code>Office::Office_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Office_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>office_prob[]</code>.
   */
  double get_transmission_prob(int condition, Person* i, Person* s);

  /**
   * @see Place::get_contacts_per_day(int condition)
   *
   * This method returns the value from the static array <code>Office::Office_contacts_per_day</code>
   * that corresponds to a particular condition.<br />
   * The static array <code>Office_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>office_contacts[]</code>.
   */
  double get_contacts_per_day(int condition);

  /**
   * Determine if the office should be open. It is dependent on the condition and simulation day.
   *
   * @param day the simulation day
   * @param condition an integer representation of the condition
   * @return whether or not the office is open on the given day for the given condition
   */
  bool should_be_open(int day, int condition) {
    return true;
  }

  void set_workplace(Workplace* _workplace);

  Workplace* get_workplace() {
    return this->workplace;
  }

private:
  static double contacts_per_day;
  static double** prob_transmission_per_contact;
  Workplace *workplace;
};

#endif // _FRED_OFFICE_H

