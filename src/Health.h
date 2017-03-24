/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Health.h
//
#ifndef _FRED_HEALTH_H
#define _FRED_HEALTH_H

#include <vector>
using namespace std;

class Age_Map;
class Condition;
class HIV_Infection;
class Person;
class Natural_History;
class Mixing_Group;

typedef struct {
  Natural_History* natural_history;
  int state;
  int last_transition_day;
  int next_transition_day;
  int onset_day;
  int symptoms_level;
  int symptoms_start_date;

  // status flags
  bool is_fatal;
  bool is_immune;
  bool is_infected;
  bool is_recovered;

  // info related to infections
  double susceptibility;      // nominal susceptibility of current state
  double infectivity;	      // nominal transmissibility of current state
  Person* infector;
  Mixing_Group* mixing_group;
  int number_of_infectees;

  bool is_susceptibility_modified;
  bool is_transmission_modified;
  bool is_symptoms_modified;
  double* susceptibility_modifier;
  double* transmission_modifier;
  double* symptoms_modifier;

} health_condition_t;


// The following enum defines symbolic names for Insurance Company Assignment.
// The last element should always be UNSET.
namespace Insurance_assignment_index {
  enum e {
    PRIVATE,
    MEDICARE,
    MEDICAID,
    HIGHMARK,
    UPMC,
    UNINSURED,
    UNSET
  };
};

class Health {

public:

  Health();
  ~Health();
  void setup(Person* self);

  // UPDATE THE PERSON'S HEALTH CONDITIONS

  void update_condition(int day, int condition_id);
  void become_exposed(int condition_id, Person* infector, Mixing_Group* mixing_group, int day);
  void become_susceptible(int condition_id);
  void become_unsusceptible(int condition_id);
  void become_infectious(int condition_id);
  void become_noninfectious(int condition_id);
  void become_symptomatic(int condition_id);
  void resolve_symptoms(int condition_id);
  void become_case_fatality(int condition_id, int day);
  void become_immune(int condition_id);
  void become_removed(int condition_id, int day);
  void recover(int condition_id, int day);
  void infect(Person* infectee, int condition_id, Mixing_Group* mixing_group, int day);
  void update_mixing_group_counts(int day, int condition_id, Mixing_Group* mixing_group);
  void terminate(int day);

  // ACCESS METHODS TO HEALTH CONDITIONS

  void set_state(int condition_id, int state, int day);

  int get_state(int condition_id) const {
    return this->health_condition[condition_id].state;
  }

  void set_last_transition_day(int condition_id, int day) {
    this->health_condition[condition_id].last_transition_day = day;
  }

  int get_last_transition_day(int condition_id) const {
    return this->health_condition[condition_id].last_transition_day;
  }

  void set_next_transition_day(int condition_id, int day) {
    this->health_condition[condition_id].next_transition_day = day;
  }

  int get_next_transition_day(int condition_id) const {
    return this->health_condition[condition_id].next_transition_day;
  }

  void set_onset_day(int condition_id, int day) {
    this->health_condition[condition_id].onset_day = day;
  }

  int get_onset_day(int condition_id) const {
    return this->health_condition[condition_id].onset_day;
  }

  int get_exposure_date(int condition_id) const {
    return get_onset_day(condition_id);
  }

  int get_symptoms_start_date(int condition_id) const {
    return this->health_condition[condition_id].symptoms_start_date;
  }

  void set_symptoms_start_date(int condition_id, int day) {
    this->health_condition[condition_id].symptoms_start_date = day;
  }

  void set_symptoms_level(int condition_id, int level) {
    this->health_condition[condition_id].symptoms_level = level;
  }

  int get_symptoms_level(int condition_id) const {
    int nominal_level = this->health_condition[condition_id].symptoms_level;
    return (int) (nominal_level * get_symptoms_modifier(condition_id));
  }

  bool is_case_fatality(int condition_id) const {
    return this->health_condition[condition_id].is_fatal;
  }

  void set_case_fatality(int condition_id) {
    this->health_condition[condition_id].is_fatal = true;
  }

  bool is_immune(int condition_id) const {
    return this->health_condition[condition_id].is_immune;
  }

  void set_immunity(int condition_id) {
    this->health_condition[condition_id].is_immune = true;
  }

  void clear_immunity(int condition_id) {
    this->health_condition[condition_id].is_immune = false;
  }

  bool is_infected(int condition_id) const {
    return this->health_condition[condition_id].is_infected;
  }

  void set_infection(int condition_id) {
    this->health_condition[condition_id].is_infected = true;
  }

  void clear_infection(int condition_id) {
    this->health_condition[condition_id].is_infected = false;
  }

  bool is_recovered(int condition_id) const {
    return this->health_condition[condition_id].is_recovered;
  }

  void set_recovered(int condition_id) {
    this->health_condition[condition_id].is_recovered = true;
  }

  void clear_recovered(int condition_id) {
    this->health_condition[condition_id].is_recovered = false;
  }

  void set_susceptibility(int condition_id, double susceptibility) {
    this->health_condition[condition_id].susceptibility = susceptibility;
  }

  double get_susceptibility(int condition_id) const {
    return this->health_condition[condition_id].susceptibility;
  }

  void set_infectivity(int condition_id, double infectivity) {
    this->health_condition[condition_id].infectivity = infectivity;
  }

  double get_infectivity(int condition_id) const {
    return this->health_condition[condition_id].infectivity;
  }

  void set_infector(int condition_id, Person* infector) {
    this->health_condition[condition_id].infector = infector;
  }

  Person* get_infector(int condition_id) const {
    return this->health_condition[condition_id].infector;
  }

  void set_mixing_group(int condition_id, Mixing_Group* mixing_group) {
    this->health_condition[condition_id].mixing_group = mixing_group;
  }

  Mixing_Group* get_mixing_group(int condition_id) const {
    return this->health_condition[condition_id].mixing_group;
  }

  Mixing_Group* get_infected_mixing_group(int condition_id) const {
    return get_mixing_group(condition_id);
  }

  int get_mixing_group_id(int condition) const;

  char* get_mixing_group_label(int condition) const;

  char get_mixing_group_type(int condition) const;

  void increment_number_of_infectees(int condition_id) {
    this->health_condition[condition_id].number_of_infectees++;
  }

  int get_number_of_infectees(int condition_id) const {
    return this->health_condition[condition_id].number_of_infectees;
  }

  HIV_Infection* get_hiv_infection() {
    return this->hiv_infection;
  }

  // END ACCESS METHODS

  int get_days_symptomatic() { 
    return this->days_symptomatic;
  }

  double get_transmission_modifier(int condition_id) const;
  double get_susceptibility_modifier(int condition_id) const;
  double get_symptoms_modifier(int condition_id) const;

  // TESTS FOR HEALTH CONDITIONS

  bool is_susceptible(int condition_id) const {
    return get_susceptibility(condition_id) > 0.0;
  }

  bool is_infectious(int condition_id) const {
    return get_infectivity(condition_id) > 0.0;
  }

  bool is_symptomatic(int condition_id) const {
    return get_symptoms_level(condition_id) > 0;
  }

  bool is_symptomatic() const {
    for (int i = 0; i < this->number_of_conditions; ++i) {
      if (is_symptomatic(i)) {
	return true;
      }
    }
    return false;
  }

  bool is_newly_infected(int day, int condition_id) {
    return day == get_onset_day(condition_id);
  }

  bool is_alive() const {
    return this->alive;
  }

  // HEALTH INSURANCE

  Insurance_assignment_index::e get_insurance_type() const {
    return this->insurance_type;
  }

  void set_insurance_type(Insurance_assignment_index::e insurance_type) {
    this->insurance_type = insurance_type;
  }

  /////// STATIC MEMBERS

  /*
   * Initialize any static variables needed by the Health class
   */
  static void initialize_static_variables();
    
  static const char* insurance_lookup(Insurance_assignment_index::e idx) {
    assert(idx >= Insurance_assignment_index::PRIVATE);
    assert(idx <= Insurance_assignment_index::UNSET);
    switch(idx) {
    case Insurance_assignment_index::PRIVATE:
      return "Private";
    case Insurance_assignment_index::MEDICARE:
      return "Medicare";
    case Insurance_assignment_index::MEDICAID:
      return "Medicaid";
    case Insurance_assignment_index::HIGHMARK:
      return "Highmark";
    case Insurance_assignment_index::UPMC:
      return "UPMC";
    case Insurance_assignment_index::UNINSURED:
      return "Uninsured";
    case Insurance_assignment_index::UNSET:
      return "UNSET";
    default:
      Utils::fred_abort("Invalid Health Insurance Type", "");
    }
    return NULL;
  }

  static Insurance_assignment_index::e get_insurance_type_from_int(int insurance_type) {
    switch(insurance_type) {
    case 0:
      return Insurance_assignment_index::PRIVATE;
    case 1:
      return Insurance_assignment_index::MEDICARE;
    case 2:
      return Insurance_assignment_index::MEDICAID;
    case 3:
      return Insurance_assignment_index::HIGHMARK;
    case 4:
      return Insurance_assignment_index::UPMC;
    case 5:
      return Insurance_assignment_index::UNINSURED;
    default:
      return Insurance_assignment_index::UNSET;
    }
  }

  static Insurance_assignment_index::e get_health_insurance_from_distribution();

  bool ever_active(int condition_id) {
    return (get_onset_day(condition_id) >= 0);
  }

  int get_days_in_health_state(int condition_id, int day) {
    return (day - get_last_transition_day(condition_id));
  }

  int get_new_health_state(int condition_id);

private:

  int number_of_conditions;

  // link back to person
  Person* myself;

  // health condition array
  health_condition_t* health_condition;

  int days_symptomatic; 			// over all conditions

  // living or not?
  bool alive;

  //Insurance Type
  Insurance_assignment_index::e insurance_type;

  // previous infection serotype (for dengue)
  int previous_infection_serotype;

  // possible HIV_Infection
  HIV_Infection* hiv_infection;

  /////// STATIC MEMBERS

  static bool is_initialized;
  static Age_Map* pregnancy_hospitalization_prob_mult;
  static Age_Map* pregnancy_case_fatality_prob_mult;

  static double Hh_income_susc_mod_floor;

  // health insurance probabilities
  static double health_insurance_distribution[Insurance_assignment_index::UNSET];
  static int health_insurance_cdf_size;

};

#endif // _FRED_HEALTH_H
