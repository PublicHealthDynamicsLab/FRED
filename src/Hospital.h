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
// File: Hospital.h
//

#ifndef _FRED_HOSPITAL_H
#define _FRED_HOSPITAL_H

#include "Place.h"
#include "Health.h"

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
  Hospital();
  ~Hospital() {
    if(Global::Enable_HAZEL && this->checked_open_day_vec != NULL) {
      delete this->checked_open_day_vec;
    }
  }

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup(const char* lab, double lon, double lat, Place* cont, Population* pop)
   */
  Hospital(const char* lab, fred::place_subtype _subtype, double lon, double lat, Place* container, Population* pop);

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Hospital(int,const char*, double, double, Place*, Population*)</code>
   */
  void get_parameters(int diseases);

  /**
   * @see Place::get_group(int disease, Person* per)
   */
  int get_group(int disease, Person* per);

  /**
   * @see Place::get_transmission_prob(int disease, Person* i, Person* s)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Hospital_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>hospital_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person* i, Person* s);

  /**
   * @see Place::get_contacts_per_day(int disease)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contacts_per_day</code>
   * that corresponds to a particular disease.<br />
   * The static array <code>Hospital_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>hospital_contacts[]</code>.
   */
  double get_contacts_per_day(int disease);


  // If we don't really care about the disease
  bool should_be_open(int day);

  /**
   * @see Place::should_be_open(int day, int disease)
   *
   * Determine if the Hospital should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the hospital is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease);

  void set_accepts_insurance(Insurance_assignment_index::e insr, bool does_accept);
  void set_accepts_insurance(int insr_indx, bool does_accept);

  bool accepts_insurance(Insurance_assignment_index::e insr) {
    return this->accepted_insurance_bitset[static_cast<int>(insr)];
  }

  int get_bed_count() {
    return this->bed_count;
  }

  void set_bed_count(int _bed_count) {
    this->bed_count = _bed_count;
  }

  int get_capacity() {
    return this->capacity;
  }

  void set_capacity(int _capacity) {
    this->capacity = _capacity;
  }

private:
  static double* Hospital_contacts_per_day;
  static double*** Hospital_contact_prob;
  static std::vector<double> Hospital_health_insurance_prob;
  static bool Hospital_parameters_set;
  static int HAZEL_disaster_start_day;
  static int HAZEL_disaster_end_day;
  static double HAZEL_disaster_max_bed_usage_multiplier;
  static std::vector<double> HAZEL_reopening_CDF;
  int bed_count;
  int capacity;
  bool is_open;
  std::vector<bool>* checked_open_day_vec;
  // true iff a the hospital accepts the indexed Insurance Coverage
  std::bitset<static_cast<size_t>(Insurance_assignment_index::UNSET)> accepted_insurance_bitset;
};

#endif // _FRED_HOSPITAL_H

