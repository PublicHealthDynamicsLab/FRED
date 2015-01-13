/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Workplace.h
//

#ifndef _FRED_WORKPLACE_H
#define _FRED_WORKPLACE_H

#include "Place.h"
#include <vector>

class Office;

class Workplace: public Place {
public: 

  /**
   * Default constructor
   */
  Workplace() { offices.clear(); next_office = 0; }
  ~Workplace() {}

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup( const char *lab, double lon, double lat, Place* cont, Population *pop)
   */
  Workplace( const char *,double,double,Place *, Population *pop );

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Workplace(int loc, const char *lab, double lon, double lat, Place *container, Population* pop)</code>
   */
  void get_parameters(int diseases);

  /**
   * Initialize the workplace and its offices
   */
  void prepare();

  /**
   * @see Place::get_group(int disease, Person * per)
   */
  int get_group(int disease, Person * per) { return 0; }

  /**
    * @see Place::get_transmission_prob(int disease, Person * i, Person * s)
    *
    * This method returns the value from the static array <code>Workplace::Workplace_contact_prob</code> that
    * corresponds to a particular age-related value for each person.<br />
    * The static array <code>Workplace_contact_prob</code> will be filled with values from the parameter
    * file for the key <code>workplace_prob[]</code>.
    */
  double get_transmission_prob(int disease, Person * i, Person * s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Workplace::Workplace_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Workplace_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>workplace_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);

  int get_number_of_rooms();
  /**
   * Setup the offices within this Workplace
   */
  void setup_offices( Allocator< Office > & office_allocator );

  /**
   * Assign a person to a particular Office
   * @param per the Person to assign
   * @return the Office to which the Person was assigned
   */
  Place * assign_office(Person *per);

  /**
   * Determine if the Workplace should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the workplace is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) { return true; }

  static int get_max_office_size() { return Office_size; }

  static int get_total_workers() { return total_workers; }
  static int get_workers_in_small_workplaces() { return workers_in_small_workplaces; }
  static int get_workers_in_medium_workplaces() { return workers_in_medium_workplaces; }
  static int get_workers_in_large_workplaces() { return workers_in_large_workplaces; }
  static int get_workers_in_xlarge_workplaces() { return workers_in_xlarge_workplaces; }
  static int get_small_workplace_size() { return Small_workplace_size; }
  static int get_medium_workplace_size() { return Medium_workplace_size; }
  static int get_large_workplace_size() { return Large_workplace_size; }

 private:
  static double * Workplace_contacts_per_day;
  static double *** Workplace_contact_prob;
  static bool Workplace_parameters_set;
  static int Office_size;
  static int Small_workplace_size;
  static int Medium_workplace_size;
  static int Large_workplace_size;
  static int total_workers;
  static int workers_in_small_workplaces;
  static int workers_in_medium_workplaces;
  static int workers_in_large_workplaces;
  static int workers_in_xlarge_workplaces;

  vector <Place *> offices;
  int next_office;
};

#endif // _FRED_WORKPLACE_H

