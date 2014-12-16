/*
 This file is part of the FRED system.

 Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
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

#include <vector>
#include <bitset>
using namespace std;

#include "Infection.h"
#include "Disease.h"
#include "Global.h"
#include "Past_Infection.h"

class Person;
class Disease;
class Antiviral;
class Antivirals;
class AV_Manager;
class AV_Health;
class Vaccine;
class Vaccine_Health;
class Vaccine_Manager;
class Place;

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
// The last element should always be INSURANCE_ASSIGNMENTS.
namespace Insurance_assignment_index {
  enum e {
    PRIVATE,
    MEDICARE,
    MEDICAID,
    HIGHMARK,
    UPMC,
    UNINSURED,
    INSURANCE_ASSIGNMENTS
  };
};

class Health {

  static int nantivirals;

public:

  /**
   * Default constructor that sets the Person to which this Health object applies
   *
   * @param person a pointer to a Person object
   */
  Health(Person* person);

  ~Health();

  static const char* chronic_condition_lookup(int idx) {
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

  /**
   *
   * Perform the daily update for this object.  This method is performance-critical and
   * accounts for much of the time spent when the epidemic is otherwise 'idle' (little active
   * infection spreading).  Any changes made here should be made carefully,
   * especially w.r.t. cache performance and branch predictability.
   *
   * @param day the simulation day
   */
  void update(Person* self, int day);

  void update_face_mask_decision(Person* self, int day);

  /*
   * Separating the updates for vaccine & antivirals from the
   * infection update gives improvement for the base.  
   */
  void update_interventions(Person* self, int day);

  /**
   * Agent is susceptible to the disease
   *
   * @param disease which disease
   */
  void become_susceptible(Person* self, int disease_id);

  void become_susceptible(Person* self, Disease* disease);

  void become_susceptible_by_vaccine_waning(Person* self, Disease* disease);

  /**
   * Agent is unsusceptible to the disease
   *
   * @param disease which disease
   */
  void become_unsusceptible(Person* self, Disease* disease);

  /**
   * Agent is infectious
   *
   * @param disease pointer to the Disease object
   */
  void become_infectious(Person* self, Disease* disease);

  /**
   * Agent is symptomatic
   *
   * @param disease pointer to the Disease object
   */
  void become_symptomatic(Person* self, Disease* disease);

  /**
   * Agent is immune to the disease
   *
   * @param disease pointer to the Disease object
   */
  void become_immune(Person* self, Disease* disease);

  /**
   * Agent is removed from the susceptible population that is
   * available to a given disease
   *
   * @param disease which disease
   */
  void become_removed(Person* self, int disease_id);

  /**
   * Agent is 'At Risk' to a given Disease
   *
   * @param disease pointer to the Disease object
   */
  void declare_at_risk(Disease* disease);

  /**
   * Agent recovers from a given Disease
   *
   * @param disease pointer to the Disease object
   */
  void recover(Person* self, Disease* disease);

  /**
   * Is the agent susceptible to a given disease
   *
   * @param disease which disease
   * @return <code>true</code> if the agent is susceptible, <code>false</code> otherwise
   */
  bool is_susceptible(int disease_id) const {
    return this->susceptible.test(disease_id);
  }

  bool is_recovered(int disease_id);

  /**
   * Is the agent infectious for a given disease
   *
   * @param disease which disease
   * @return <code>true</code> if the agent is infectious, <code>false</code> otherwise
   */
  bool is_infectious(int disease_id) const {
    return this->infectious.test(disease_id);
  }

  bool is_infected(int disease_id) const {
    return this->active_infections.test(disease_id);
  }

  /**
   * Is the agent symptomatic - note that this is independent of disease
   *
   * @return <code>true</code> if the agent is symptomatic, <code>false</code> otherwise
   */
  bool is_symptomatic() const {
    return this->symptomatic.any();
  }

  bool is_symptomatic(int disease_id) {
    return this->symptomatic.test(disease_id);
  }

  int get_days_symptomatic() { 
    return this->days_symptomatic; 
  }

  /**
   * Is the agent immune to a given disease
   *
   * @param disease pointer to the disease in question
   * @return <code>true</code> if the agent is immune, <code>false</code> otherwise
   */
  bool is_immune(Disease* disease) const {
    return this->immunity.test(disease->get_id());
  }

  /**
   * Is the agent 'At Risk' to a given disease
   *
   * @param disease pointer to the disease in question
   * @return <code>true</code> if the agent is at risk, <code>false</code> otherwise
   */
  bool is_at_risk(Disease* disease) const {
    return this->at_risk.test(disease->get_id());
  }

  /**
   * Is the agent 'At Risk' to a given disease
   *
   * @param disease which disease
   * @return <code>true</code> if the agent is at risk, <code>false</code> otherwise
   */
  bool is_at_risk(int disease_id) const {
    return this->at_risk.test(disease_id);
  }

  /*
   * Advances the course of the infection by moving the exposure date
   * backwards
   */
  void advance_seed_infection(int disease_id, int days_to_advance);

  /**
   * @param disease
   * @return the simulation day that this agent became exposed to the disease
   */
  int get_exposure_date(int disease_id) const;

  /**
   * @param disease
   * @return the simulation day that this agent became infectious with the disease
   */
  int get_infectious_date(int disease_id) const;

  /**
   * @param disease
   * @return the simulation day that this agent recovered from the disease
   */
  int get_recovered_date(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return the simulation day that this agent became symptomatic to the disease
   */
  int get_symptomatic_date(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return Person who gave this person the disease
   */
  Person* get_infector(int disease_id) const;

  Place* get_infected_place(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return
   */
  int get_infected_place_id(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return the label of the infected place
   */
  char* get_infected_place_label(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return the type of place
   */
  char get_infected_place_type(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return
   */
  int get_infectees(int disease_id) const;

  /**
   * @param disease the disease to check
   * @return
   */
  double get_susceptibility(int disease_id) const;

  /**
   * @param disease the disease to check
   * @param day the simulation day
   * @return
   */
  double get_infectivity(int disease_id, int day) const;

  /**
   * @param disease the disease to check
   * @param day the simulation day
   * @return
   */
  double get_symptoms(int disease_id, int day) const;

  /**
   * @param disease the disease to check
   * @return a pointer to the Infection object
   */
  Infection* get_infection(int disease_id) const {
    return this->infection[disease_id];
  }

  /**
   * Determine whether or not the agent is on Anti-Virals for a given disease on a given day
   *
   * @param day the simulation day
   * @param disease the disease to check
   * @return <code>true</code> if the agent is on Anti-Virals, <code>false</code> otherwise
   */
  bool is_on_av_for_disease(int day, int disease_id) const;

  /**
   * Infect an agent with a disease
   *
   * @param infectee a pointer to the Person that this agent is trying to infect
   * @param disease the disease with which to infect the Person
   * @param transmission a pointer to a Transmission object
   */
  void infect(Person* self, Person* infectee, int disease_id,
      Transmission & transmission);

  /**
   * @param disease pointer to a Disease object
   * @param transmission pointer to a Transmission object
   */
  void become_exposed(Person* self, Disease* disease,
      Transmission & transmission);

  //Medication operators
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

  //Modifiers
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

  // Personal Health Behaviors
  bool is_wearing_face_mask() { 
    return this->wears_face_mask_today; 
  }

  bool is_washing_hands() { 
    return this->washes_hands; 
  }

  void start_wearing_face_mask() {
    this->wears_face_mask_today = true;
  }

  void stop_wearing_face_mask() {
    this->wears_face_mask_today = false;
  }

  double get_transmission_modifier_due_to_hygiene(int disease_id);
  double get_susceptibility_modifier_due_to_hygiene(int disease_id);

  void terminate(Person* self);

  int get_num_past_infections(int disease) {
    return this->past_infections[disease].size();
  }

  Past_Infection* get_past_infection(int disease, int i) {
    return &(this->past_infections[disease].at(i));
  }

  void clear_past_infections(int disease) {
    this->past_infections[disease].clear();
  }

  //void add_past_infection(int d, Past_Infection* pi){ past_infections[d].push_back(pi); }  
  void add_past_infection(int strain_id, int recovery_date, int age_at_exposure,
      Disease* dis) {
    this->past_infections[dis->get_id()].push_back(
        Past_Infection(strain_id, recovery_date, age_at_exposure));
  }

  void update_place_counts(Person* self, int day, int disease_id,
      Place* place);

  bool is_newly_infected(int day, int disease_id) {
    return day == get_exposure_date(disease_id);
  }

  bool is_newly_symptomatic(int day, int disease_id) {
    return day == get_symptomatic_date(disease_id);
  }

  bool is_alive() {
    return this->alive;
  }

  void die() {
    this->alive = false;
  }

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

  static double get_chronic_condition_case_fatality_prob_mult(double real_age, int cond_idx);
  static double get_chronic_condition_hospitalization_prob_mult(double real_age, int cond_idx);

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

  /*
   * Initialize any static variables needed by the Health class
   */
  static void initialize_static_variables() ;

private:

  // current chronic conditions
  std::map<int, bool> chronic_conditions_map;

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

  // The index of the person in the Population
  //int person_index;

  // living or not?
  bool alive;

  int days_symptomatic;

  // TODO (JVD): The infection vector & bitset should be combined into a little 
  // helper class to make sure that they're always synched up.
  // There would be just a little overhead in doing this but probably well worth it.
  // Until then make sure that they're always changed together.
  Infection * infection[Global::MAX_NUM_DISEASES];
  // bitset removes need to check each infection in above array to
  // find out if any are not NULL
  fred::disease_bitset active_infections;
  fred::disease_bitset immunity;
  fred::disease_bitset at_risk; // Agent is/isn't at risk for severe complications
  // Per-disease health status flags
  fred::disease_bitset susceptible;
  fred::disease_bitset infectious;
  fred::disease_bitset symptomatic;
  fred::disease_bitset recovered_today;
  fred::disease_bitset recovered;
  fred::disease_bitset evaluate_susceptibility;

  // per-disease susceptibility multiplier
  double susceptibility_multp[Global::MAX_NUM_DISEASES];

  // Antivirals.  These are all dynamically allocated to save space
  // when not in use
  typedef vector<bool> checked_for_av_type;
  checked_for_av_type* checked_for_av;
  typedef vector<AV_Health*> av_health_type;
  av_health_type* av_health;
  typedef vector<AV_Health*>::iterator av_health_itr;

  // Vaccines.  These are all dynamically allocated to save space
  // when not in use
  typedef vector<Vaccine_Health*> vaccine_health_type;
  vaccine_health_type* vaccine_health;
  typedef vector<Vaccine_Health*>::iterator vaccine_health_itr;

  // follows face mask wearing behavior pattern
  bool has_face_mask_behavior;
  bool wears_face_mask_today;
  int days_wearing_face_mask;

  // washes hands (every day)
  bool washes_hands;

  // Define a bitset type to hold health flags
  // Enumeration corresponding to positions in health
  // intervention_flags bitset is defined in implementation file
  typedef std::bitset<2> intervention_flags_type;
  intervention_flags_type intervention_flags;

  // Past_Infections used for reignition
  vector<Past_Infection> past_infections[Global::MAX_NUM_DISEASES];

  // previous infection serotype (for dengue)
  int previous_infection_serotype;

  int infectee_count[Global::MAX_NUM_DISEASES];
  int susceptible_date[Global::MAX_NUM_DISEASES];

  /*
   * @return <code>true</code> if the map contains any condition and it that condition is true,
   *  <code>false</code> otherwise
   */
  bool has_chronic_condition() {
    for(int i = 0; i < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS; i++) {
      if(has_chronic_condition(i)) {
        return true;
      }
    }
    return false;
  }

  /*
   * @return <code>true</code> if the map contains the condition and it is true,
   *  <code>false</code> otherwise
   * @param cond_idx the Chronic_condition_index to search for
   */
  bool has_chronic_condition(int cond_idx) {
    assert(cond_idx >= 0);
    assert(cond_idx < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS);
    if(this->chronic_conditions_map.find(cond_idx)
        != this->chronic_conditions_map.end()) {
      return this->chronic_conditions_map[cond_idx];
    } else {
      return false;
    }
  }

  /*
   * Set the given Chronic_medical_condtion to <code>true</code> or <code>false</code>
   * @param cond_idx the Chronic_condition_index to set
   * @param has_cond whether or not the agent has the condition
   */
  void set_has_chronic_condition(int cond_idx, bool has_cond) {
    assert(cond_idx >= 0);
    assert(cond_idx < Chronic_condition_index::CHRONIC_MEDICAL_CONDITIONS);
    this->chronic_conditions_map[cond_idx] = has_cond;
  }

protected:

  friend class Person;
  Health();
  void setup(Person* self);
};

#endif // _FRED_HEALTH_H
