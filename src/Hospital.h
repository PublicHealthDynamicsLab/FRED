/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Hospital.h
//

#ifndef _FRED_HOSPITAL_H
#define _FRED_HOSPITAL_H

#include "Place.h"
#include "Person.h"

/**
 * This class represents a hospital location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the program file.
 *
 * @see Place
 */
class Hospital : public Place {

public: 
  /**
   * Default constructor
   */
  Hospital();

  /**
   * Constructor with necessary properties
   */
  Hospital(const char* lab, char _subtype, fred::geo lon, fred::geo lat);
  ~Hospital() { }

  static void get_properties();

  bool is_open(int sim_day);

  /**
   *
   * Determine if the Hospital should be open. This is independent of any condition.
   *
   * @property sim_day the simulation day
   * @return whether or not the hospital is open on the given day for the given condition
   */
  bool should_be_open(int sim_day);

  /**
   * @see Place::should_be_open(int day, int condition)
   *
   * Determine if the Hospital should be open. It is dependent on the condition and simulation day.
   *
   * @property sim_day the simulation day
   * @property condition an integer representation of the condition
   * @return whether or not the hospital is open on the given day for the given condition
   */
  bool should_be_open(int sim_day, int condition);

  void set_accepts_insurance(Insurance_assignment_index::e insr, bool does_accept);
  void set_accepts_insurance(int insr_indx, bool does_accept);

  bool accepts_insurance(Insurance_assignment_index::e insr) {
    return this->accepted_insurance_bitset[static_cast<int>(insr)];
  }

  int get_bed_count(int sim_day);

  void set_bed_count(int _bed_count) {
    this->bed_count = _bed_count;
  }

  int get_employee_count() {
    return this->employee_count;
  }

  void set_employee_count(int _employee_count) {
    this->employee_count = _employee_count;
  }

  int get_physician_count() {
    return this->physician_count;
  }

  void set_physician_count(int _physician_count) {
    this->physician_count = _physician_count;
  }

  int get_daily_patient_capacity(int sim_day);

  void set_daily_patient_capacity(int _capacity) {
    this->daily_patient_capacity = _capacity;
  }

  int get_current_daily_patient_count() {
    return this->current_daily_patient_count;
  }

  void increment_current_daily_patient_count() {
    this->current_daily_patient_count++;
  }

  void reset_current_daily_patient_count() {
    this->current_daily_patient_count = 0;
  }

  int get_occupied_bed_count() {
    return this->occupied_bed_count;
  }

  void increment_occupied_bed_count() {
    this->occupied_bed_count++;
  }

  void decrement_occupied_bed_count() {
    this->occupied_bed_count--;
  }

  void reset_occupied_bed_count() {
    this->occupied_bed_count = 0;
  }

  std::string to_string();


private:
  static std::vector<double> Hospital_health_insurance_prob;

  int bed_count;
  int occupied_bed_count;
  int daily_patient_capacity;
  int current_daily_patient_count;

  int employee_count;
  int physician_count;

  bool add_capacity;
  int open_day;
  int close_day;

  // true iff a the hospital accepts the indexed Insurance Coverage
  std::bitset<static_cast<size_t>(Insurance_assignment_index::UNSET)> accepted_insurance_bitset;

};

#endif // _FRED_HOSPITAL_H

