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

#include "Place.h"
#include "Hospital.h"
#include "Person.h"
#include "Neighborhood_Patch.h"
#include "Random.h"

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
   * @see Place::setup(const char* lab, fred::geo lon, fred::geo lat, Place* cont, Population* pop)
   */
  Household(const char* lab, fred::place_subtype subtype, fred::geo lon, fred::geo lat, Place* container, Population* pop);

  ~Household() {}

  /**
   * @see Place::get_parameters(int diseases)
   *
   * This method is called by the constructor
   * <code>Household(int loc, const char* lab, fred:geo lon, fred::geo lat, Place* container, Population* pop)</code>
   */
  void get_parameters(int diseases);

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
   * Get a person from the household.
   *
   * @param i the index of the person in the household
   * @return a pointer the person with index i in the household
   */
  Person* get_housemate(int i) {
    return this->enrollees[i];
  }

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
    this->set_household_income_code(Household::get_household_income_level_code_from_income(x));
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
   * Record the ages in sorted order, and record the id's of the original members of the household
   */
  void record_profile();

  /*
   * @param i the index of the agent
   * @return the age of the household member with index i
   */
  int get_age_of_member(int i) {
    return this->ages[i];
  }

  /**
   * @return the original count of agents in this Household
   */
  int get_orig_size() {
    return (int)this->ids.size();
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

  void set_county(int _county_index) {
    this->county_index = _county_index;
  }

  int get_county() {
    return this->county_index;
  }

  int get_census_tract_index() {
    return this->census_tract_index;
  }

  void set_census_tract_index(int _census_tract_index) {
    this->census_tract_index = _census_tract_index;
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

  int get_size() {
    return this->N;
  }

  vector<unsigned char> get_original_ages() {
    return this->ages;
  }

  void set_group_quarters_workplace(Place* p) {
    this->group_quarters_workplace = p;
  }

  Place* get_group_quarters_workplace() {
    return this->group_quarters_workplace;
  }

private:

  static double* Household_contacts_per_day;
  static double*** Household_contact_prob;
  static bool Household_parameters_set;

  //Income Limits for classification
  static int Cat_I_Max_Income;
  static int Cat_II_Max_Income;
  static int Cat_III_Max_Income;
  static int Cat_IV_Max_Income;
  static int Cat_V_Max_Income;
  static int Cat_VI_Max_Income;

  Place* group_quarters_workplace;
  bool sheltering;
  unsigned char deme_id;	      // deme == synthetic population id
  int group_quarters_units;
  int shelter_start_day;
  int shelter_end_day;
  int household_income;		      // household income
  int household_income_code;
  int county_index;
  int census_tract_index;

  // true iff a household member is at one of the places for an extended absence
  //std::bitset<Household_extended_absence_index::HOUSEHOLD_EXTENDED_ABSENCE> not_home_bitset;
  std::bitset<4> not_home_bitset;

  // Places that household members may visit
  std::map<Household_visitation_place_index::e, Place*> household_visitation_places_map;

  // profile of original housemates
  vector<unsigned char> ages;
  vector<int> ids;

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

#endif // _FRED_HOUSEHOLD_H
