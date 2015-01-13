/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Classroom.h
//

#ifndef _FRED_CLASSROOM_H
#define _FRED_CLASSROOM_H

#include "Place.h"

/**
 * This class represents a classroom location in the FRED application. It inherits from <code>Place</code>.
 * The class contains many static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
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
   * @see Place::setup( const char *lab, double lon, double lat, Place* cont, Population *pop)
   */
  Classroom( const char *lab, double lon, double lat, Place *container, Population *pop);

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor <code>
   * Classroom(int loc, const char *lab, double lon, double lat, Place *container, Population *pop)
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
  int get_age_level() { return age_level; }

 private:
  static double * Classroom_contacts_per_day;
  static double *** Classroom_contact_prob;
  static char Classroom_closure_policy[];
  static int Classroom_closure_day;
  static double Classroom_closure_threshold;
  static int Classroom_closure_period;
  static int Classroom_closure_delay;
  static bool Classroom_parameters_set;

  int age_level;
};

#endif // _FRED_CLASSROOM_H

