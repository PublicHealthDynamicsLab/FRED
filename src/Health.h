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
// File: Health.h
//
#ifndef _FRED_HEALTH_H
#define _FRED_HEALTH_H

#include <bitset>
#include <vector>
using namespace std;

#include "Age_Map.h"
#include "Disease.h"
#include "Global.h"
#include "Infection.h"
#include "Past_Infection.h"

class Antiviral;
class Antivirals;
class AV_Health;
class AV_Manager;
class Disease;
class Person;
class Mixing_Group;
class Vaccine;
class Vaccine_Health;
class Vaccine_Manager;

// The following enum defines symbolic names for Chronic Medical Conditions.
// The last element should always be CHRONIC_MEDICAL_CONDITIONS.
namespace Chronic_condition_index {
  enum e {
    ASTHMA,
    COPD, //Chronic Obstructive Pulmonary Disease
    CHRONIC_RENAL_DISEASE,
    DIABETES,
    HEART_DISEASE,
    HYPERTENSION,
    HYPERCHOLESTROLEMIA,
    CHRONIC_MEDICAL_CONDITIONS
  };
};

// Enumerate flags corresponding to positions in
// health intervention_flags bitset
namespace Intervention_flag {
  enum e {
    TAKES_VACCINE,
    TAKES_AV
  };
};

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

  void update_infection(int day, int disease_id);
  void update_face_mask_decision(Person* self, int day);
  void update_interventions(Person* self, int day);
  void become_exposed(Person* self, int disease_id, Person* infector, Mixing_Group* mixing_group, int day);
  void become_susceptible(Person* self, int disease_id);
  void become_susceptible(Person* self, Disease* disease);
  void become_susceptible_by_vaccine_waning(Person* self, int disease_id);
  void become_unsusceptible(Person* self, Disease* disease);
  void become_unsusceptible(Person* self, int disease_id);
  void become_infectious(Person* self, Disease* disease);
  void become_symptomatic(Person* self, Disease* disease);
  void resolve_symptoms(Person* self, Disease* disease);
  void become_immune(Person* self, Disease* disease);
  void become_removed(Person* self, int disease_id);
  void declare_at_risk(Disease* disease);
  void recover(Person* self, Disease* disease);
  void advance_seed_infection(int disease_id, int days_to_advance);
  void infect(Person* self, Person* infectee, int disease_id, Mixing_Group* mixing_group, int day);
  //  void increment_infectee_count(Person* self, int disease_id, Person* infectee, Mixing_Group* mixing_group, int day);
  void start_wearing_face_mask() {
    this->wears_face_mask_today = true;
  }
  void stop_wearing_face_mask() {
    this->wears_face_mask_today = false;
  }
  void terminate(Person* self);
  void clear_past_infections(int disease_id) {
    this->past_infections[disease_id].clear();
  }
  void add_past_infection(int strain_id, int recovery_date, int age_at_exposure, Disease* dis) {
    this->past_infections[dis->get_id()].push_back(
						   Past_Infection(strain_id, recovery_date, age_at_exposure));
  }
  void update_mixing_group_counts(Person* self, int day, int disease_id, Mixing_Group* mixing_group);


  // ACCESS TO HEALTH CONDITIONS

  int get_days_symptomatic() { 
    return this->days_symptomatic;
  }
  int get_exposure_date(int disease_id) const;
  int get_infectious_start_date(int disease_id) const;
  int get_infectious_end_date(int disease_id) const;
  int get_symptoms_start_date(int disease_id) const;
  int get_symptoms_end_date(int disease_id) const;
  int get_immunity_end_date(int disease_id) const;
  int get_infector_id(int disease_id) const;
  Person* get_infector(int disease_id) const;
  Mixing_Group* get_infected_mixing_group(int disease_id) const;
  int get_infected_mixing_group_id(int disease_id) const;
  char* get_infected_mixing_group_label(int disease_id) const;
  char get_infected_mixing_group_type(int disease_id) const;
  int get_infectees(int disease_id) const;
  double get_susceptibility(int disease_id) const;
  double get_infectivity(int disease_id, int day) const;
  double get_symptoms(int disease_id, int day) const;
  Infection* get_infection(int disease_id) const {
    return this->infection[disease_id];
  }
  double get_transmission_modifier_due_to_hygiene(int disease_id);
  double get_susceptibility_modifier_due_to_hygiene(int disease_id);
  double get_susceptibility_modifier_due_to_household_income(int disease_id);
  int get_num_past_infections(int disease) {
    return this->past_infections[disease].size();
  }
  Past_Infection* get_past_infection(int disease, int i) {
    return &(this->past_infections[disease].at(i));
  }


  // TESTS FOR HEALTH CONDITIONS

  bool is_case_fatality(int disease_id) {
    return this->case_fatality.test(disease_id);
  }

  bool is_susceptible(int disease_id) const {
    return this->susceptible.test(disease_id);
  }

  bool is_infectious(int disease_id) const {
    return this->infectious.test(disease_id);
  }

  bool is_infected(int disease_id) const {
    return this->infection[disease_id] != NULL;
  }

  bool is_symptomatic() const {
    return this->symptomatic.any();
  }

  bool is_symptomatic(int disease_id) {
    return this->symptomatic.test(disease_id);
  }

  bool is_recovered(int disease_id);

  bool is_immune(int disease_id) const {
    return this->immunity.test(disease_id);
  }

  bool is_at_risk(int disease_id) const {
    return this->at_risk.test(disease_id);
  }

  bool is_on_av_for_disease(int day, int disease_id) const;

  // Personal Health Behaviors
  bool is_wearing_face_mask() { 
    return this->wears_face_mask_today; 
  }

  bool is_washing_hands() { 
    return this->washes_hands; 
  }

  bool is_newly_infected(int day, int disease_id) {
    return day == get_exposure_date(disease_id);
  }

  bool is_newly_symptomatic(int day, int disease_id) {
    return day == get_symptoms_start_date(disease_id);
  }

  bool is_alive() const {
    return this->alive;
  }


  // MEDICATION OPERATORS

  /**
   * Agent will take a vaccine
   * @param vacc pointer to the Vaccine to take
   * @param day the simulation day
   * @param vm a pointer to the Manager of the Vaccinations
   */
  void take_vaccine(Person* self, Vaccine* vacc, int day, Vaccine_Manager* vm);

  /**
   * Agent will take an antiviral
   * @param av pointer to the Antiviral to take
   * @param day the simulation day
   */
  void take(Antiviral* av, int day);

  /**
   * @return a count of the antivirals this agent has already taken
   */
  int get_number_av_taken() const {
    if(this->av_health) {
      return this->av_health->size();
    } else {
      return 0;
    }
  }

  /**
   * @param s the index of the av to check
   * @return the checked_for_av with the given index
   */
  int get_checked_for_av(int s) const {
    assert(this->checked_for_av != NULL);
    return (*this->checked_for_av)[s];
  }

  /**
   * Set the checked_for_av value at the given index to 1
   * @param s the index of the av to set
   */
  void flip_checked_for_av(int s) {
    if(this->checked_for_av == NULL) {
      this->checked_for_av = new checked_for_av_type();
      this->checked_for_av->assign(Health::nantivirals, false);
    }
    (*this->checked_for_av)[s] = 1;
  }

  /**
   * @return <code>true</code> if the agent is vaccinated, <code>false</code> if not
   */
  bool is_vaccinated() const {
    if(this->vaccine_health) {
      return this->vaccine_health->size();
    } else {
      return 0;
    }
  }

  /**
   * @return the number of vaccines this agent has taken
   */
  int get_number_vaccines_taken() const {
    if(this->vaccine_health) {
      return this->vaccine_health->size();
    } else {
      return 0;
    }
  }

  /**
   * @return a pointer to this instance's AV_Health object
   */
  AV_Health* get_av_health(int i) const {
    assert(this->av_health != NULL);
    return (*this->av_health)[i];
  }

  /**
   * @return this instance's av_start day
   */
  int get_av_start_day(int i) const;

  /**
   * @return a pointer to this instance's Vaccine_Health object
   */
  Vaccine_Health* get_vaccine_health(int i) const {
    if(this->vaccine_health) {
      return (*this->vaccine_health)[i];
    } else {
      return NULL;
    }
  }

  // MODIFIERS

  /**
   * Alter the susceptibility of the agent to the given disease by a multiplier
   * @param disease the disease to which the agent is suceptible
   * @param multp the multiplier to apply
   */
  void modify_susceptibility(int disease_id, double multp);

  /**
   * Alter the infectivity of the agent to the given disease by a multiplier
   * @param disease the disease with which the agent is infectious
   * @param multp the multiplier to apply
   */
  void modify_infectivity(int disease_id, double multp);

  /**
   * Alter the infectious period of the agent for the given disease by a multiplier.
   * Modifying infectious period is equivalent to modifying symptomatic and asymptomatic
   * periods by the same amount. Current day is needed to modify infectious period, because we can't cause this
   * infection to recover in the past.
   *
   * @param disease the disease with which the agent is infectious
   * @param multp the multiplier to apply
   * @param cur_day the simulation day
   */
  void modify_infectious_period(int disease_id, double multp, int cur_day);

  /**
   * Alter the symptomatic period of the agent for the given disease by a multiplier.
   * Current day is needed to modify symptomatic period, because we can't cause this
   * infection to recover in the past.
   *
   * @param disease the disease with which the agent is symptomatic
   * @param multp the multiplier to apply
   * @param cur_day the simulation day
   */
  void modify_symptomatic_period(int disease_id, double multp, int cur_day);

  /**
   * Alter the asymptomatic period of the agent for the given disease by a multiplier.
   * Current day is needed to modify symptomatic period, because we can't cause this
   * infection to recover in the past.
   *
   * @param disease the disease with which the agent is asymptomatic
   * @param multp the multiplier to apply
   * @param cur_day the simulation day
   */
  void modify_asymptomatic_period(int disease_id, double multp, int cur_day);

  /**
   * Alter the whether or not the agent will develop symptoms.
   * Can't change develops_symptoms if this person is not asymptomatic ('i' or 'E')
   * Current day is needed to modify symptomatic period, because we can't change symptomaticity that
   * is in the past.
   *
   * @param disease the disease with which the agent is asymptomatic
   * @param symptoms whether or not the agent is showing symptoms
   * @param cur_day the simulation day
   */
  void modify_develops_symptoms(int disease_id, bool symptoms, int cur_day);


  // CHRONIC CONDITIONS

  /**
   * @return <code>true</code> if agent is asthmatic, <code>false</code> otherwise
   */
  bool is_asthmatic() {
    return has_chronic_condition(Chronic_condition_index::ASTHMA);
  }

  /**
   * Sets whether or not agent is asthmatic
   * @param has_cond whether or not the agent is asthmatic
   */
  void set_is_asthmatic(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::ASTHMA, has_cond);
  }

  /**
   * @return <code>true</code> if agent has COPD (Chronic Obstructive Pulmonary
   * disease), <code>false</code> otherwise
   */
  bool has_COPD() {
    return has_chronic_condition(Chronic_condition_index::COPD);
  }

  /**
   * Sets whether or not the agent has COPD (Chronic Obstructive Pulmonary
   * disease)
   * @param has_cond whether or not the agent has COPD
   */
  void set_has_COPD(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::COPD, has_cond);
  }

  /**
   * @return <code>true</code> if agent has chronic renal disease, <code>false</code> otherwise
   */
  bool has_chronic_renal_disease() {
    return has_chronic_condition(Chronic_condition_index::CHRONIC_RENAL_DISEASE);
  }

  /**
   * Sets whether or not the agent has chronic renal disease
   * @param has_cond whether or not the agent has chronic renal disease
   */
  void set_has_chronic_renal_disease(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::CHRONIC_RENAL_DISEASE,
			      has_cond);
  }

  /**
   * @return <code>true</code> if agent is diabetic, <code>false</code> otherwise
   */
  bool is_diabetic() {
    return has_chronic_condition(Chronic_condition_index::DIABETES);
  }

  /**
   * Sets whether or not the agent is diabetic
   * @param has_cond whether or not the agent is diabetic
   */
  void set_is_diabetic(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::DIABETES, has_cond);
  }

  /**
   * @return <code>true</code> if agent has heart disease, <code>false</code> otherwise
   */
  bool has_heart_disease() {
    return has_chronic_condition(Chronic_condition_index::HEART_DISEASE);
  }

  /**
   * Sets whether or not the agent has heart disease
   * @param has_cond whether or not the agent has heart disease
   */
  void set_has_heart_disease(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::HEART_DISEASE, has_cond);
  }
    
  /**
   * @return <code>true</code> if agent has hypertension, <code>false</code> otherwise
   */
  bool has_hypertension() {
    return has_chronic_condition(Chronic_condition_index::HYPERTENSION);
  }
    
  /**
   * Sets whether or not the agent has hypertension
   * @param has_cond whether or not the agent has hypertension
   */
  void set_has_hypertension(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::HYPERTENSION, has_cond);
  }

  /**
   * @return <code>true</code> if agent has hypercholestrolemia, <code>false</code> otherwise
   */
  bool has_hypercholestrolemia() {
    return has_chronic_condition(Chronic_condition_index::HYPERCHOLESTROLEMIA);
  }

  /**
   * Sets whether or not the agent has hypercholestrolemia 
   * @param has_cond whether or not the agent has hypercholestrolemia 
   */
  void set_has_hypercholestrolemia(bool has_cond) {
    set_has_chronic_condition(Chronic_condition_index::HYPERCHOLESTROLEMIA, has_cond);
  }

  /*
   * @return <code>true</code> if the map contains the condition and it is true,
   *  <code>false</code> otherwise
   * @param cond_idx the Chronic_condition_index to search for
   */
  bool has_chronic_condition(int cond_idx) {
    assert(cond_idx >= static_cast<int>(Chronic_condition_index::ASTHMA));
    assert(cond_idx < static_cast<int>(Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS));
    if(this->chronic_conditions_map.find(static_cast<Chronic_condition_index::e>(cond_idx))
       != this->chronic_conditions_map.end()) {
      return this->chronic_conditions_map[static_cast<Chronic_condition_index::e>(cond_idx)];
    } else {
      return false;
    }
  }

  /*
   * Set the given Chronic_medical_condtion to <code>true</code> or <code>false</code>
   * @param cond_idx the Chronic_condition_index to set
   * @param has_cond whether or not the agent has the condition
   */
  void set_has_chronic_condition(Chronic_condition_index::e cond_idx, bool has_cond) {
    assert(cond_idx >= Chronic_condition_index::ASTHMA);
    assert(cond_idx < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS);
    this->chronic_conditions_map[cond_idx] = has_cond;
  }

  /*
   * @return <code>true</code> if the map contains any condition and it that condition is true,
   *  <code>false</code> otherwise
   */
  bool has_chronic_condition() {
    for(int i = 0; i < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS; ++i) {
      if(has_chronic_condition(i)) {
        return true;
      }
    }
    return false;
  }

  // HEALTH INSURANCE

  Insurance_assignment_index::e get_insurance_type() const {
    return this->insurance_type;
  }

  void set_insurance_type(Insurance_assignment_index::e insurance_type) {
    this->insurance_type = insurance_type;
  }

  /////// STATIC MEMBERS

  static int nantivirals;

  /*
   * Initialize any static variables needed by the Health class
   */
  static void initialize_static_variables();
    
  static const char* chronic_condition_lookup(Chronic_condition_index::e idx) {
    assert(idx >= 0);
    assert(idx < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS);
    switch(idx) {
    case Chronic_condition_index::ASTHMA:
      return "Asthma";
    case Chronic_condition_index::COPD:
      return "COPD";
    case Chronic_condition_index::CHRONIC_RENAL_DISEASE:
      return "Chronic Renal Disease";
    case Chronic_condition_index::DIABETES:
      return "Diabetes";
    case Chronic_condition_index::HEART_DISEASE:
      return "Heart Disease";
    case Chronic_condition_index::HYPERTENSION:
      return "Hypertension";
    case Chronic_condition_index::HYPERCHOLESTROLEMIA:
      return "Hypercholestrolemia";
    default:
      Utils::fred_abort("Invalid Chronic Condition Type", "");
    }
    return NULL;
  }

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

  static double get_chronic_condition_case_fatality_prob_mult(double real_age, Chronic_condition_index::e cond_idx);
  static double get_chronic_condition_hospitalization_prob_mult(double real_age, Chronic_condition_index::e cond_idx);

  static double get_pregnancy_case_fatality_prob_mult(double real_age) {
    if(Global::Enable_Chronic_Condition && Health::is_initialized) {
      return Health::pregnancy_case_fatality_prob_mult->find_value(real_age);
    }
    return 1.0;
  }

  static double get_pregnancy_hospitalization_prob_mult(double real_age) {
    if(Global::Enable_Chronic_Condition && Health::is_initialized) {
      return Health::pregnancy_hospitalization_prob_mult->find_value(real_age);
    }
    return 1.0;
  }

  static bool Enable_hh_income_based_susc_mod;


private:

  // link back to person
  Person * myself;

  // active infections (NULL if not infected)
  Infection** infection;

  // persistent infection data (kept after infection clears)
  double* susceptibility_multp;
  int* infectee_count;
  int* immunity_end_date;
  int* exposure_date;
  int* infector_id;
  Mixing_Group** infected_in_mixing_group;
  int days_symptomatic; 			// over all diseases

  // living or not?
  bool alive;

  // bitset removes need to check each infection in above array to
  // find out if any are not NULL
  fred::disease_bitset immunity;
  fred::disease_bitset at_risk; // Agent is/isn't at risk for severe complications

  // Per-disease health status flags
  fred::disease_bitset susceptible;
  fred::disease_bitset infectious;
  fred::disease_bitset symptomatic;
  fred::disease_bitset recovered_today;
  fred::disease_bitset recovered;
  fred::disease_bitset case_fatality;

  // Define a bitset type to hold health flags
  // Enumeration corresponding to positions in health
  typedef std::bitset<2> intervention_flags_type;
  intervention_flags_type intervention_flags;

  // Antivirals.  These are all dynamically allocated to save space
  // when not in use
  typedef std::vector<bool> checked_for_av_type;
  checked_for_av_type* checked_for_av;
  typedef std::vector<AV_Health*> av_health_type;
  av_health_type* av_health;
  typedef std::vector<AV_Health*>::iterator av_health_itr;

  // Vaccines.  These are all dynamically allocated to save space
  // when not in use
  typedef std::vector<Vaccine_Health*> vaccine_health_type;
  vaccine_health_type* vaccine_health;
  typedef std::vector<Vaccine_Health*>::iterator vaccine_health_itr;

  // health behaviors
  bool has_face_mask_behavior;
  bool wears_face_mask_today;
  int days_wearing_face_mask;
  bool washes_hands;				// every day

  // current chronic conditions
  std::map<Chronic_condition_index::e, bool> chronic_conditions_map;

  //Insurance Type
  Insurance_assignment_index::e insurance_type;

  // Past_Infections used for reignition
  typedef std::vector<Past_Infection> past_infections_type;
  past_infections_type* past_infections;

  // previous infection serotype (for dengue)
  int previous_infection_serotype;

  /////// STATIC MEMBERS

  static bool is_initialized;
  static Age_Map* asthma_prob;
  static Age_Map* COPD_prob;
  static Age_Map* chronic_renal_disease_prob;
  static Age_Map* diabetes_prob;
  static Age_Map* heart_disease_prob;
  static Age_Map* hypertension_prob;
  static Age_Map* hypercholestrolemia_prob;
    
  static Age_Map* asthma_hospitalization_prob_mult;
  static Age_Map* COPD_hospitalization_prob_mult;
  static Age_Map* chronic_renal_disease_hospitalization_prob_mult;
  static Age_Map* diabetes_hospitalization_prob_mult;
  static Age_Map* heart_disease_hospitalization_prob_mult;
  static Age_Map* hypertension_hospitalization_prob_mult;
  static Age_Map* hypercholestrolemia_hospitalization_prob_mult;

  static Age_Map* asthma_case_fatality_prob_mult;
  static Age_Map* COPD_case_fatality_prob_mult;
  static Age_Map* chronic_renal_disease_case_fatality_prob_mult;
  static Age_Map* diabetes_case_fatality_prob_mult;
  static Age_Map* heart_disease_case_fatality_prob_mult;
  static Age_Map* hypertension_case_fatality_prob_mult;
  static Age_Map* hypercholestrolemia_case_fatality_prob_mult;

  static Age_Map* pregnancy_hospitalization_prob_mult;
  static Age_Map* pregnancy_case_fatality_prob_mult;

  // health protective behavior parameters
  static int Days_to_wear_face_masks;
  static double Face_mask_compliance;
  static double Hand_washing_compliance;

  static double Hh_income_susc_mod_floor;

  // health insurance probabilities
  static double health_insurance_distribution[Insurance_assignment_index::UNSET];
  static int health_insurance_cdf_size;

};

#endif // _FRED_HEALTH_H
