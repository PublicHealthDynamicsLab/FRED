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
// File: Household.h
//
#ifndef _FRED_HOUSEHOLD_H
#define _FRED_HOUSEHOLD_H

#include "Global.h"
#include "Hospital.h"
#include "Place.h"

class Person;

/**
 * The following enum defines symbolic names for whether or not a household
 * member is currently on an extended absence from the household.
 * The last element should always be HOUSEHOLD_EXTENDED_ABSENCE.
 */
namespace Household_extended_absence_index {
  enum e {
    HAS_HOSPITALIZED,
    //HAS_IN_PRISON,
    //HAS_IN_NURSING_HOME,
    //HAS_IN_COLLEGE_DORM,
    HOUSEHOLD_EXTENDED_ABSENCE
  };
};

/**
 * The following enum defines symbolic names for places that
 * members of this household may visit.
 * The last element should always be HOUSEHOLD_VISITATION.
 */
namespace Household_visitation_place_index {
  enum e {
    HOSPITAL,
    //PRISON,
    //NURSING_HOME,
    //COLLEGE_DORM,
    HOUSEHOLD_VISITATION
  };
};


/**
 * This class represents a household location in the FRED application. It inherits from <code>Place</code>.
 * The class contains static variables that will be filled with values from the program file.
 *
 * @see Place
 */
class Household : public Place {
public:

  /**
   * Default constructor
   */
  Household();

  /**
   * Constructor with necessary properties
   */
  Household(const char* lab, char _subtype, fred::geo lon, fred::geo lat);

  ~Household() {}

  static void get_properties();

  person_vector_t get_inhabitants() {
    return this->members;
  }

  void set_household_race(int _race) {
    this->race = _race;
  }

  int get_household_race() {
    return this->race;
  }

  /**
   * Determine if the household should be open. It is dependent on the condition and simulation day.
   *
   * @property day the simulation day
   * @property condition an integer representation of the condition
   * @return whether or not the household is open on the given day for the given condition
   */
  bool should_be_open(int day, int condition) {
    return true;
  }

  void set_group_quarters_units(int units) {
    this->group_quarters_units = units;
  }

  int get_group_quarters_units() {
    return this->group_quarters_units;
  }

  void set_seeking_healthcare(bool _seeking_healthcare) {
    this->seeking_healthcare = _seeking_healthcare;
  }

  bool is_seeking_healthcare() {
    return this->seeking_healthcare;
  }

  void set_is_primary_healthcare_available(bool _primary_healthcare_available) {
    this->primary_healthcare_available = _primary_healthcare_available;
  }

  bool is_primary_healthcare_available() {
    return this->primary_healthcare_available;
  }

  void set_other_healthcare_location_that_accepts_insurance_available(bool _other_healthcare_location_that_accepts_insurance_available) {
    this->other_healthcare_location_that_accepts_insurance_available = _other_healthcare_location_that_accepts_insurance_available;
  }

  bool is_other_healthcare_location_that_accepts_insurance_available() {
    return this->other_healthcare_location_that_accepts_insurance_available;
  }

  void set_is_healthcare_available(bool _healthcare_available) {
    this->healthcare_available = _healthcare_available;
  }

  bool is_healthcare_available() {
    return this->healthcare_available;
  }

  /**
   * @return a pointer to this household's visitation hospital
   */
  Hospital* get_household_visitation_hospital() {
    return static_cast<Hospital*>(get_household_visitation_place(Household_visitation_place_index::HOSPITAL));
  }

  void set_household_visitation_hospital(Hospital* hosp) {
    set_household_visitation_place(Household_visitation_place_index::HOSPITAL, hosp);
  }

  void set_household_has_hospitalized_member(bool does_have);

  bool has_hospitalized_member() {
    return this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED];
  }

  void set_group_quarters_workplace(Place* p) {
    this->group_quarters_workplace = p;
  }

  Place* get_group_quarters_workplace() {
    return this->group_quarters_workplace;
  }

  bool has_school_aged_child();
  bool has_school_aged_child_and_unemployed_adult();

  void set_hh_schl_aged_chld_unemplyd_adlt_is_set(bool _hh_status_changed) {
    this->hh_schl_aged_chld_unemplyd_adlt_is_set = _hh_status_changed;
  }

  int get_count_seeking_hc() {
    return this->count_seeking_hc;
  }

  void set_count_seeking_hc(int _count_seeking_hc) {
    this->count_seeking_hc = _count_seeking_hc;
  }

  int get_count_primary_hc_unav() {
    return this->count_primary_hc_unav;
  }

  void set_count_primary_hc_unav(int _count_primary_hc_unav) {
    this->count_primary_hc_unav = _count_primary_hc_unav;
  }

  int get_count_hc_accept_ins_unav() {
    return this->count_hc_accept_ins_unav;
  }

  void set_count_hc_accept_ins_unav(int _count_hc_accept_ins_unav) {
    this->count_hc_accept_ins_unav = _count_hc_accept_ins_unav;
  }

  void reset_healthcare_info() {
    this->set_is_primary_healthcare_available(true);
    this->set_other_healthcare_location_that_accepts_insurance_available(true);
    this->set_is_healthcare_available(true);
    this->set_count_seeking_hc(0);
    this->set_count_primary_hc_unav(0);
    this->set_count_hc_accept_ins_unav(0);
  }

  void set_migration_admin_code(int mig_admin_code) {
    this->migration_admin_code = mig_admin_code;
  }

  void clear_migration_admin_code() {
    this->migration_admin_code = 0;  // 0 means not planning to migrate or done migrating
  }

  int get_migration_admin_code() {
    return this->migration_admin_code;
  }

  int get_orig_household_structure() {
    return this->orig_household_structure;
  }

  int get_household_structure() {
    return this->household_structure;
  }

  void set_household_structure();

  void set_orig_household_structure() {
    this->orig_household_structure = this->household_structure;
    strcpy(this->orig_household_structure_label,this->household_structure_label);
  }

  char* get_household_structure_label() {
    return this->household_structure_label;
  }

  char* get_orig_household_structure_label() {
    return this->orig_household_structure_label;
  }

  void set_household_vaccination();

  double get_household_vaccination_probability() {
    return this->vaccination_probability;
  }

  int get_household_vaccination_level() {
    return (int)(100.0*this->vaccination_probability+0.5);
  }

  int get_household_vaccination_decision() {
    return this->vaccination_decision;
  }

  bool is_in_low_vaccination_school() {
    return in_low_vaccination_school;
  }

  bool refuses_vaccines() {
    return refuse_vaccine;
  }


private:

  // household structure types
#define HTYPES 21

  enum htype_t {
    SINGLE_FEMALE,
    SINGLE_MALE,
    OPP_SEX_SIM_AGE_PAIR,
    OPP_SEX_DIF_AGE_PAIR,
    OPP_SEX_TWO_PARENTS,
    SINGLE_PARENT,
    SINGLE_PAR_MULTI_GEN_FAMILY,
    TWO_PAR_MULTI_GEN_FAMILY,
    UNATTENDED_KIDS,
    OTHER_FAMILY,
    YOUNG_ROOMIES,
    OLDER_ROOMIES,
    MIXED_ROOMIES,
    SAME_SEX_SIM_AGE_PAIR,
    SAME_SEX_DIF_AGE_PAIR,
    SAME_SEX_TWO_PARENTS,
    DORM_MATES,
    CELL_MATES,
    BARRACK_MATES,
    NURSING_HOME_MATES,
    UNKNOWN,
  };

  htype_t orig_household_structure;
  htype_t household_structure;

  char orig_household_structure_label[64];
  char household_structure_label[64];

  string htype[HTYPES] = {
    "single-female",
    "single-male",
    "opp-sex-sim-age-pair",
    "opp-sex-dif-age-pair",
    "opp-sex-two-parent-family",
    "single-parent-family",
    "single-parent-multigen-family",
    "two-parent-multigen-family",
    "unattended-minors",
    "other-family",
    "young-roomies",
    "older-roomies",
    "mixed-roomies",
    "same-sex-sim-age-pair",
    "same-sex-dif-age-pair",
    "same-sex-two-parent-family",
    "dorm-mates",
    "cell-mates",
    "barrack-mates",
    "nursing-home-mates",
    "unknown",
  };

  Place* group_quarters_workplace;
  bool primary_healthcare_available;
  bool other_healthcare_location_that_accepts_insurance_available;
  bool healthcare_available;
  bool seeking_healthcare;
  int count_seeking_hc;
  int count_primary_hc_unav;
  int count_hc_accept_ins_unav;

  bool hh_schl_aged_chld_unemplyd_adlt_is_set;
  bool hh_schl_aged_chld;
  bool hh_schl_aged_chld_unemplyd_adlt;

  int group_quarters_units;
  int race;

  double vaccination_probability;
  int vaccination_decision;
  bool in_low_vaccination_school;
  bool refuse_vaccine;

  int migration_admin_code;  //household preparing to do county-to-county migration

  // true iff a household member is at one of the places for an extended absence
  //std::bitset<Household_extended_absence_index::HOUSEHOLD_EXTENDED_ABSENCE> not_home_bitset;
  std::bitset<4> not_home_bitset;

  // Places that household members may visit
  std::map<Household_visitation_place_index::e, Place*> household_visitation_places_map;

  Place* get_household_visitation_place(Household_visitation_place_index::e i) {
    if(this->household_visitation_places_map.find(i) != this->household_visitation_places_map.end()) {
      return this->household_visitation_places_map[i];
    } else {
      return NULL;
    }
  }

  void set_household_visitation_place(Household_visitation_place_index::e i, Place* p) {
    if(p != NULL) {
      this->household_visitation_places_map[i] = p;
    } else {
      std::map<Household_visitation_place_index::e, Place*>::iterator itr;
      itr = this->household_visitation_places_map.find(i);
      if(itr != this->household_visitation_places_map.end()) {
        this->household_visitation_places_map.erase(itr);
      }
    }
  }

};

#endif // _FRED_HOUSEHOLD_H
