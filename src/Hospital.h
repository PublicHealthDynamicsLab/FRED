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

#include "Health.h"
#include "Mixing_Group.h"
#include "Place.h"

class HAZEL_Hospital_Init_Data;

/**
 * This class represents a hospital location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Hospital : public Place {

  typedef std::map<std::string, HAZEL_Hospital_Init_Data> HospitalInitMapT;

public: 
  /**
   * Default constructor
   * Note: really only used by Allocator
   */
  Hospital();

  /**
   * Constructor with necessary parameters
   */
  Hospital(const char* lab, char _subtype, fred::geo lon, fred::geo lat);
  ~Hospital() { }

  static void get_parameters();

  /**
   * @see Place::get_group(int condition, Person* per)
   */
  int get_group(int condition, Person* per);

  /**
   * @see Mixing_Group::get_transmission_prob(int condition, Person* i, Person* s)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Hospital_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>hospital_prob[]</code>.
   */
  double get_transmission_prob(int condition, Person* i, Person* s);

  /**
   * @see Place::get_contacts_per_day(int condition)
   *
   * This method returns the value from the static array <code>Hospital::Hospital_contacts_per_day</code>
   * that corresponds to a particular condition.<br />
   * The static array <code>Hospital_contacts_per_day</code> will be filled with values from the parameter
   * file for the key <code>hospital_contacts[]</code>.
   */
  double get_contacts_per_day(int condition);

  bool is_open(int sim_day);

  /**
   * @see Place::should_be_open(int day)
   *
   * Determine if the Hospital should be open. This is independent of any condition.
   *
   * @param sim_day the simulation day
   * @return whether or not the hospital is open on the given day for the given condition
   */
  bool should_be_open(int sim_day);

  /**
   * @see Place::should_be_open(int day, int condition)
   *
   * Determine if the Hospital should be open. It is dependent on the condition and simulation day.
   *
   * @param sim_day the simulation day
   * @param condition an integer representation of the condition
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

  void have_HAZEL_closure_dates_been_set(bool is_set) {
    this->HAZEL_closure_dates_have_been_set = is_set;
  }

  std::string to_string();

  static int get_HAZEL_mobile_van_open_delay() {
    return Hospital::HAZEL_mobile_van_open_delay;
  }

  static int get_HAZEL_mobile_van_closure_day() {
    return Hospital::HAZEL_mobile_van_closure_day;
  }

private:
  static double contacts_per_day;
  static double** prob_transmission_per_contact;
  static std::vector<double> Hospital_health_insurance_prob;
  static double HAZEL_disaster_capacity_multiplier;
  static int HAZEL_mobile_van_open_delay;
  static int HAZEL_mobile_van_closure_day;
  static HospitalInitMapT HAZEL_hospital_init_map;
  static bool HAZEL_hospital_init_map_file_exists;
  static std::vector<double> HAZEL_reopening_CDF;

  int bed_count;
  int occupied_bed_count;
  int daily_patient_capacity;
  int current_daily_patient_count;
  bool add_capacity;
  bool HAZEL_closure_dates_have_been_set;

  // true iff a the hospital accepts the indexed Insurance Coverage
  std::bitset<static_cast<size_t>(Insurance_assignment_index::UNSET)> accepted_insurance_bitset;

  void apply_individual_HAZEL_closure_policy();
};

struct HAZEL_Hospital_Init_Data {
  int panel_week;
  bool accpt_private;
  bool accpt_medicare;
  bool accpt_medicaid;
  bool accpt_highmark;
  bool accpt_upmc;
  bool accpt_uninsured; // Person has no insurance and pays out of pocket
  int reopen_after_days;
  bool is_mobile;
  bool add_capacity;

  void setup(const char* _panel_week, const char* _accpt_private, const char* _accpt_medicare,
	     const char* _accpt_medicaid, const char* _accpt_highmark, const char* _accpt_upmc,
	     const char* _accpt_uninsured, const char* _reopen_after_days, const char* _is_mobile,
	     const char* _add_capacity) {

    string accpt_priv_str = string(_accpt_private);
    string accpt_medicr_str = string(_accpt_medicare);
    string accpt_medicd_str = string(_accpt_medicaid);
    string accpt_hghmrk_str = string(_accpt_highmark);
    string accpt_upmc_str = string(_accpt_medicare);
    string accpt_uninsrd_str = string(_accpt_uninsured);
    string is_mobile_str = string(_is_mobile);
    string add_capacity_str = string(_add_capacity);

    sscanf(_panel_week, "%d", &panel_week);
    this->accpt_private = Utils::to_bool(accpt_priv_str);
    this->accpt_medicare = Utils::to_bool(accpt_medicr_str);
    this->accpt_medicaid = Utils::to_bool(accpt_medicd_str);
    this->accpt_highmark = Utils::to_bool(accpt_hghmrk_str);
    this->accpt_upmc = Utils::to_bool(accpt_upmc_str);
    this->accpt_uninsured = Utils::to_bool(accpt_uninsrd_str);
    this->is_mobile = Utils::to_bool(is_mobile_str);
    this->add_capacity = Utils::to_bool(add_capacity_str);
    sscanf(_reopen_after_days, "%d", &reopen_after_days);
  }

  HAZEL_Hospital_Init_Data(const char* _panel_week, const char* _accpt_private, const char* _accpt_medicare,
			   const char* _accpt_medicaid, const char* _accpt_highmark, const char* _accpt_upmc,
			   const char* _accpt_uninsured, const char* _reopen_after_days, const char* _is_mobile, const char* _add_capacity) {
    setup(_panel_week, _accpt_private, _accpt_medicare,
	  _accpt_medicaid, _accpt_highmark, _accpt_upmc,
	  _accpt_uninsured, _reopen_after_days, _is_mobile, _add_capacity);
  }

};

#endif // _FRED_HOSPITAL_H

