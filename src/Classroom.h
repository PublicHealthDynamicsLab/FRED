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
// File: Classroom.h
//

#ifndef _FRED_CLASSROOM_H
#define _FRED_CLASSROOM_H

#include "Place.h"
#include "Random.h"

// class School;

#define GRADES 20

class Classroom: public Place {
  
public: 

  /**
   * Default constructor
   */
  Classroom() {}
  ~Classroom() {}

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, fred::geo lon, fred::geo lat, Place* cont, Population *pop)
   */
  Classroom( const char *lab, fred::place_subtype subtype, fred::geo lon, fred::geo lat, Place *container);

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor <code>
   * Classroom(int loc, const char *lab, fred::geo lon, fred::geo lat, Place *container, Population *pop)
   * </code>
   */
  void get_parameters(int diseases);

  /**
   * Add a person to the classroom. This method increments the number of people in
   * the classroom and also sets the age level for the classroom, if the person added
   * is the first child to be enrolled.<br />
   * Overrides Place::enroll(Person * per)
   *
   * @param per a pointer to a Person object that may be added to the classroom
   */
  void enroll(Person * per);
  void unenroll(Person * per);

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per);

  /**
   * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
   *
   * This method returns the value from the static array <code>Classroom::Classroom_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Classroom_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>classroom_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::should_be_open(int day, int disease)
   */
  bool should_be_open(int day, int disease);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Classroom::Classroom_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Classroom_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>classroom_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  /**
   *  @return the age_level
   */
  int get_age_level() {
    return this->age_level;
  }

  void set_school(Place* _school) {
    school = _school;
  }

  Place * get_school() {
    return school;
  }

private:
  static double * Classroom_contacts_per_day;
  static double *** Classroom_contact_prob;
  static char Classroom_closure_policy[];
  static int Classroom_closure_day;
  static double Classroom_closure_threshold;
  static int Classroom_closure_period;
  static int Classroom_closure_delay;
  static bool Classroom_parameters_set;

  Place * school;
  int age_level;
};

#endif // _FRED_CLASSROOM_H

