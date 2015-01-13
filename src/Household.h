/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Household.h
//

#ifndef _FRED_HOUSEHOLD_H
#define _FRED_HOUSEHOLD_H

#include "Place.h"
#include "Person.h"
#include "Cell.h"


/**
 * This class represents a household location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Household: public Place {
public: 

  /**
   * Default constructor
   */
  Household() {}

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, fred::geo lon, fred::geo lat, Place* cont, Population *pop)
   */
  Household( const char *lab, fred::geo lon, fred::geo lat, Place *container, Population* pop );

  ~Household() {}

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Household(int loc, const char *lab, fred:geo lon, fred::geo lat, Place *container, Population* pop)</code>
   */
  void get_parameters(int diseases);

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per);

  /**
   * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
   *
   * This method returns the value from the static array <code>Household::Household_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Household_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>household_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Household::Household_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Household_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>household_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  /**
   * Add a person to the household. This method increments the number of people in
   * the household and also increments the adult or child count as appropriate.<br />
   * Overrides Place::enroll(Person * per)
   *
   * @param per a pointer to a Person object that will be added to the household
   */
  void enroll(Person * per);

  /**
   * Delete a person from the household. This method decrements the number of people in
   * the household and also decrements the adult or child count as appropriate.<br />
   * Overrides Place::enroll(Person * per)
   *
   * @param per a pointer to a Person object that will be added to the household
   */
  void unenroll(Person * per);

  /**
   * Get a person from the household.
   *
   * @param i the index of the person in the household
   * @return a pointer the person with index i in the household
   */
  Person * get_housemate(int i) { return housemate[i]; }

  /**
   * Use to get list of all people in the household.
   * @return vector of pointers to people in household.
   */
  vector <Person *> get_inhabitants() { return housemate; }

  /**
   * Get a neighborhood from the grid_cell to which this household belongs.
   *
   * @return a pointer to the neighborhood selected
   * @see Cell::select_neighborhood()
   */
  // Place * select_neighborhood() { return grid_cell->select_neighborhood(); }

  /**
   * Get the number of adults in the household.
   * @return the number of adults
   */
  int get_adults() { return adults; }

  /**
   * Get the number of children in the household.
   * @return the number of children
   */
  int get_children() { return children; }

  /**
   * Set the household income.
   */
  void set_household_income(int x) { household_income = x; }

  /**
   * Get the household income.
   * @return income
   */
  int get_household_income() { return household_income; }

  /**
   * Determine if the household should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the household is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) { return true; }

  /**
   * Record the ages in sorted order, and record the id's of the original members of the household
   */
  void record_profile();

  /*
   * @param i the index of the agent
   * @return the age of the household member with index i
   */
  int get_age_of_member(int i) { return ages[i]; }

  /**
   * @return the original count of agents in this Household
   */
  int get_orig_size() { return (int) ids.size(); }

  /*
   * @param i the index of the agent
   * @return the id of the original household member with index i
   */
  int get_orig_id(int i) { return ids[i]; }

  void spread_infection(int day, int s);

private:
  static double * Household_contacts_per_day;
  static double *** Household_contact_prob;
  static bool Household_parameters_set;

  int household_income;				// household income

  vector <Person *> housemate;
  int children;
  int adults;

  // profile of original housemates
  vector <int> ages;
  vector <int> ids;
};

#endif // _FRED_HOUSEHOLD_H

