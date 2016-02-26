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

class School;

#define GRADES 20

class Classroom: public Place {
  
public: 

  /**
   * Default constructor
   * Note: really only used by Allocator
   */
  Classroom();

  /**
   * Constructor with necessary parameters
   */
  Classroom(const char *lab, char _subtype, fred::geo lon, fred::geo lat);
  ~Classroom() {}

  static void get_parameters();

  int enroll(Person* per);
  void unenroll(int pos);

  /**
   * @see Place::get_group(int condition, Person* per)
   */
  int get_group(int condition, Person* per);

  /**
   * @see Mixing_Group::get_transmission_prob(int condition, Person* i, Person* s)
   *
   * This method returns the value from the static array <code>Classroom::Classroom_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Classroom_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>classroom_prob[]</code>.
   */
  double get_transmission_prob(int condition, Person* i, Person* s);

  bool is_open(int day);

  /**
   * @see Place::should_be_open(int day, int condition)
   */
  bool should_be_open(int day, int condition);

  /**
   * @see Place::get_contacts_per_day(int condition)
   *
   * This method returns the value from the static array <code>Classroom::Classroom_contacts_per_day</code>
   * that corresponds to a particular condition.<br />
   * The static array <code>Classroom_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>classroom_contacts[]</code>.
   */
  double get_contacts_per_day(int condition);

  /**
   *  @return the age_level
   */
  int get_age_level() {
    return this->age_level;
  }

  void set_school(School* _school);

  School* get_school() {
    return this->school;
  }

  int get_container_size();

private:
  static double contacts_per_day;
  static double** prob_transmission_per_contact;
  static char Classroom_closure_policy[];
  static int Classroom_closure_day;
  static double Classroom_closure_threshold;
  static int Classroom_closure_period;
  static int Classroom_closure_delay;
  School* school;
  int age_level;
};

#endif // _FRED_CLASSROOM_H

