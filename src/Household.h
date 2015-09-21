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
// File: Household.h
//
#ifndef _FRED_HOUSEHOLD_H
#define _FRED_HOUSEHOLD_H

#include <map>
#include <bitset>
#include <set>

#include "Global.h"
#include "Hospital.h"
#include "Place.h"

class Person;

class HH_Adult_Sickleave_Data;

// The following enum defines symbolic names for Household Income. These categories are defined by the RTI Synthetic
// Population (see Quickstart Guide https://www.epimodels.org/midas/Rpubsyntdata1.do for description)
// Note that these are just for classification purposes. We leave it to researchers
// to define the income that actually corresponds to the income level.
// The last element should always be UNCLASSIFIED.
namespace Household_income_level_code {
  enum e {
    CAT_I,
    CAT_II,
    CAT_III,
    CAT_IV,
    CAT_V,
    CAT_VI,
    CAT_VII,
    UNCLASSIFIED
  };
};

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
 * The class contains static variables that will be filled with values from the parameter file.
 *
 * @see Place
 */
class Household : public Place {
public:

  static std::map<int, int> count_inhabitants_by_household_income_level_map;
  static std::map<int, int> count_children_by_household_income_level_map;
  static std::map<long int, int> count_inhabitants_by_census_tract_map;
  static std::map<long int, int> count_children_by_census_tract_map;
  static std::set<long int> census_tract_set;

  static const char* household_income_level_lookup(int idx) {
    assert(idx >= 0);
    assert(idx <= Household_income_level_code::UNCLASSIFIED);
    switch(idx) {
      case Household_income_level_code::CAT_I:
        return "cat_I";
      case Household_income_level_code::CAT_II:
        return "cat_II";
      case Household_income_level_code::CAT_III:
        return "cat_III";
      case Household_income_level_code::CAT_IV:
        return "cat_IV";
      case Household_income_level_code::CAT_V:
        return "cat_V";
      case Household_income_level_code::CAT_VI:
        return "cat_VI";
      case Household_income_level_code::CAT_VII:
        return "cat_VII";
      case Household_income_level_code::UNCLASSIFIED:
        return "Unclassified";
      default:
        Utils::fred_abort("Invalid Household Income Level Code", "");
    }
    return NULL;
  }

  static int get_household_income_level_code_from_income(int income) {

    if(income < 0) {
      return Household_income_level_code::UNCLASSIFIED;
    } else if(income < Household::Cat_I_Max_Income) {
      return Household_income_level_code::CAT_I;
    } else if(income < Household::Cat_II_Max_Income) {
      return Household_income_level_code::CAT_II;
    } else if(income < Household::Cat_III_Max_Income) {
      return Household_income_level_code::CAT_III;
    } else if(income < Household::Cat_IV_Max_Income) {
      return Household_income_level_code::CAT_IV;
    } else if(income < Household::Cat_V_Max_Income) {
      return Household_income_level_code::CAT_V;
    } else if(income < Household::Cat_VI_Max_Income) {
      return Household_income_level_code::CAT_VI;
    } else {
      return Household_income_level_code::CAT_VII;
    }
  }

  Household();

  /**
   * Convenience constructor that sets most of the values by calling Place::setup
   *
   * @see Place::setup(const char* lab, fred::geo lon, fred::geo lat)
   */
  Household(const char* lab, fred::place_subtype subtype, fred::geo lon, fred::geo lat);

  ~Household() {}

  static void get_parameters();

  /**
   * @see Place::get_group(int disease, Person* per)
   */
  int get_group(int disease, Person* per);

  /**
   * @see Place::get_transmission_prob(int disease, Person* i, Person* s)
   *
   * This method returns the value from the static array <code>Household::Household_contact_prob</code> that
   * corresponds to a particular age-related value for each person.<br />
   * The static array <code>Household_contact_prob</code> will be filled with values from the parameter
   * file for the key <code>household_prob[]</code>.
   */
  double get_transmission_prob(int disease, Person* i, Person* s);

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
   * Use to get list of all people in the household.
   * @return vector of pointers to people in household.
   */
  vector<Person*> get_inhabitants() {
    return this->enrollees;
  }

  /**
   * Set the household income.
   */
  void set_household_income(int x) {
    this->household_income = x;
    this->set_household_income_code(static_cast<int>(Household::get_household_income_level_code_from_income(x)));
    if(x != -1) {
      if(x >= 0 && x < Household::Min_hh_income) {
        Household::Min_hh_income = x;
        //Utils::fred_log("HH_INCOME: Min[%i]\n", Household::Min_hh_income);
      }
      if(x > Household::Max_hh_income) {
        Household::Max_hh_income = x;
        //Utils::fred_log("HH_INCOME: Max[%i]\n", Household::Max_hh_income);
      }
    }
  }

  /**
   * Get the household income.
   * @return income
   */
  int get_household_income() {
    return this->household_income;
  }

  /**
   * Determine if the household should be open. It is dependent on the disease and simulation day.
   *
   * @param day the simulation day
   * @param disease an integer representation of the disease
   * @return whether or not the household is open on the given day for the given disease
   */
  bool should_be_open(int day, int disease) {
    return true;
  }

  /**
   * Record the ages and the id's of the original members of the household
   */
  void record_profile();

  /**
   * @return the original count of agents in this Household
   */
  int get_orig_size() {
    return static_cast<int>(this->ids.size());
  }

  /*
   * @param i the index of the agent
   * @return the id of the original household member with index i
   */
  int get_orig_id(int i) {
    return this->ids[i];
  }

  void set_deme_id(unsigned char _deme_id) {
    this->deme_id = _deme_id;
  }

  unsigned char get_deme_id() {
    return this->deme_id;
  }

  void set_group_quarters_units(int units) {
    this->group_quarters_units = units;
  }

  int get_group_quarters_units() {
    return this->group_quarters_units;
  }

  void set_shelter(bool _sheltering) {
    this->sheltering = _sheltering;
  }

  bool is_sheltering() {
    return this->sheltering;
  }

  bool is_sheltering_today(int day) {
    return (this->shelter_start_day <= day && day < this->shelter_end_day);
  }

  void set_shelter_start_day(int start_day) {
    this->shelter_start_day = start_day;
  }

  void set_shelter_end_day(int end_day) {
    this->shelter_end_day = end_day;
  }

  int get_shelter_start_day() {
    return this->shelter_start_day;
  }

  int get_shelter_end_day() {
    return this->shelter_end_day;
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

  int get_household_income_code() const {
    return this->household_income_code;
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

  void set_sympt_child(bool _hh_sympt_childl) {
    this->hh_sympt_child = _hh_sympt_childl;
  }

  bool has_sympt_child() {
    return this->hh_sympt_child;
  }

  void set_hh_schl_aged_chld_unemplyd_adlt_chng(bool _hh_status_changed) {
    this->hh_schl_aged_chld_unemplyd_adlt_chng = _hh_status_changed;
  }

  void set_working_adult_using_sick_leave(bool _is_using_sl) {
    this->hh_working_adult_using_sick_leave = _is_using_sl;
  }

  bool has_working_adult_using_sick_leave() {
    return this->hh_working_adult_using_sick_leave;
  }

  void prepare_person_childcare_sickleave_map();

  bool have_working_adult_use_sickleave_for_child(Person* adult, Person* child);

  void set_income_quartile(int _income_quartile) {
    assert(_income_quartile >= Global::Q1 && _income_quartile <= Global::Q4);
    this->income_quartile = _income_quartile;
  }

  int get_income_quartile() {
    return this->income_quartile;
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

  static int get_min_hh_income() {
    return Household::Min_hh_income;
  }

  static int get_max_hh_income() {
    return Household::Max_hh_income;
  }

  static int get_min_hh_income_90_pct() {
    return Household::Min_hh_income_90_pct;
  }

  static void set_min_hh_income_90_pct(int _hh_income) {
    Household::Min_hh_income_90_pct = _hh_income;
  }

private:

  static double contacts_per_day;
  static double** prob_transmission_per_contact;

  //Income Limits for classification
  static int Cat_I_Max_Income;
  static int Cat_II_Max_Income;
  static int Cat_III_Max_Income;
  static int Cat_IV_Max_Income;
  static int Cat_V_Max_Income;
  static int Cat_VI_Max_Income;

  static int Min_hh_income;
  static int Max_hh_income;
  static int Min_hh_income_90_pct;

  Place* group_quarters_workplace;
  bool sheltering;
  bool primary_healthcare_available;
  bool other_healthcare_location_that_accepts_insurance_available;
  bool healthcare_available;
  bool seeking_healthcare;
  int count_seeking_hc;
  int count_primary_hc_unav;
  int count_hc_accept_ins_unav;

  bool hh_schl_aged_chld_unemplyd_adlt_chng;
  bool hh_schl_aged_chld;
  bool hh_schl_aged_chld_unemplyd_adlt;
  bool hh_sympt_child;
  bool hh_working_adult_using_sick_leave;

  unsigned char deme_id;	      // deme == synthetic population id
  int group_quarters_units;
  int shelter_start_day;
  int shelter_end_day;
  int household_income;
  int household_income_code;
  int income_quartile;

  // true iff a household member is at one of the places for an extended absence
  //std::bitset<Household_extended_absence_index::HOUSEHOLD_EXTENDED_ABSENCE> not_home_bitset;
  std::bitset<4> not_home_bitset;

  // Places that household members may visit
  std::map<Household_visitation_place_index::e, Place*> household_visitation_places_map;

  // profile of original housemates
  std::vector<unsigned char> ages;
  std::vector<int> ids;

  // Sicktime available to watch children for adult housemates
  std::map<Person*, HH_Adult_Sickleave_Data> adult_childcare_sickleave_map;

  void set_household_income_code(int _household_income_code) {
    if(_household_income_code >= 0 ||
       _household_income_code <= Household_income_level_code::UNCLASSIFIED) {
      this->household_income_code = _household_income_code;
    } else {
      Utils::fred_abort("Invalid Household Income Level Code", "");
      this->household_income_code = Household_income_level_code::UNCLASSIFIED;
    }
  }

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


/**
 * This is a helper class that is used to store information about the children in the household
 * The adults in the household who work will each be mapped to one of these objects to keep
 * track of whether or not they have already used a sick day for a given child
 */
class HH_Adult_Sickleave_Data {
public:
  HH_Adult_Sickleave_Data() { }

  void add_child_to_maps(Person* child) {
    std::pair<std::map<Person*, bool>::iterator, bool> ret;
    ret = this->stayed_home_for_child_map.insert(std::pair<Person*, bool>(child, false));
    assert(ret.second); // Shouldn't insert the child twice
  }

  bool stay_home_with_child(Person* child) {
    std::map<Person*, bool>::iterator itr;

    bool ret_val = false;

    itr = this->stayed_home_for_child_map.find(child);
    if(itr != this->stayed_home_for_child_map.end()) {
      if(!itr->second) { //didn't already stay home with this child
        itr->second = true;
        ret_val = true;
      }
    }
    return ret_val;
  }

private:
  std::map<Person*, bool> stayed_home_for_child_map;

};

#endif // _FRED_HOUSEHOLD_H
