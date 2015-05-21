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
   */
  Hospital();
  ~Hospital() { }

  static bool HAZEL_hospital_init_map_file_exists;

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

  /**
   * @see Place::should_be_open(int day)
   *
   * Determine if the Hospital should be open. This is independent of any disease.
   *
   * @param day the simulation day
   * @return whether or not the hospital is open on the given day for the given disease
   */
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

  int get_daily_patient_capacity() {
    return this->daily_patient_capacity;
  }

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


private:
  static double* Hospital_contacts_per_day;
  static double*** Hospital_contact_prob;
  static std::vector<double> Hospital_health_insurance_prob;
  static bool Hospital_parameters_set;
  static double HAZEL_disaster_max_bed_usage_multiplier;
  static HospitalInitMapT HAZEL_hospital_init_map;
  static int HAZEL_disaster_start_day;
  static int HAZEL_disaster_end_day;
  static std::vector<double> HAZEL_reopening_CDF;

  int bed_count;
  int occupied_bed_count;
  int daily_patient_capacity;
  int current_daily_patient_count;
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

  void setup(const char* _panel_week, const char* _accpt_private, const char* _accpt_medicare,
      const char* _accpt_medicaid, const char* _accpt_highmark, const char* _accpt_upmc,
      const char* _accpt_uninsured, const char* _reopen_after_days) {

    string accpt_priv_str = string(_accpt_private);
    string accpt_medicr_str = string(_accpt_medicare);
    string accpt_medicd_str = string(_accpt_medicaid);
    string accpt_hghmrk_str = string(_accpt_highmark);
    string accpt_upmc_str = string(_accpt_medicare);
    string accpt_uninsrd_str = string(_accpt_uninsured);

    sscanf(_panel_week, "%d", &panel_week);
    accpt_private = Utils::to_bool(accpt_priv_str);
    accpt_medicare = Utils::to_bool(accpt_medicr_str);
    accpt_medicaid = Utils::to_bool(accpt_medicd_str);
    accpt_highmark = Utils::to_bool(accpt_hghmrk_str);
    accpt_upmc = Utils::to_bool(accpt_upmc_str);
    accpt_uninsured = Utils::to_bool(accpt_uninsrd_str);
    sscanf(_reopen_after_days, "%d", &reopen_after_days);
  }

  HAZEL_Hospital_Init_Data(const char* _panel_week, const char* _accpt_private, const char* _accpt_medicare,
      const char* _accpt_medicaid, const char* _accpt_highmark, const char* _accpt_upmc,
      const char* _accpt_uninsured, const char* _reopen_after_days) {
     setup(_panel_week, _accpt_private, _accpt_medicare,
           _accpt_medicaid, _accpt_highmark, _accpt_upmc,
           _accpt_uninsured, _reopen_after_days);
  }

};

#endif // _FRED_HOSPITAL_H

