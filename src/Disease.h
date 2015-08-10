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
// File: Disease.h
//

#ifndef _FRED_Disease_H
#define _FRED_Disease_H

#include <fstream>
#include <vector>
#include <map>
#include <string>
using namespace std;

#include "Epidemic.h"
#include "Global.h"

class Age_Map;
class Evolution;
class Infection;
class Natural_History;
class Person;

class Disease {
public:

  /**
   * Default constructor
   */
  Disease();
  ~Disease();

  void initialize_static_variables();
  void get_parameters(int disease, string name);

  /**
   * Set all of the attributes for the Disease
   */
  void setup();

  // called by Infection:
  int get_latent_period(Person* host);
  int get_duration_of_infectiousness(Person* host);
  int get_incubation_period(Person* host);
  int get_duration_of_symptoms(Person* host);
  int get_duration_of_immunity(Person* host);


  /**
   * @return this Disease's id
   */
  int get_id() {
    return this->id;
  }

  /**
   * @return the transmissibility
   */
  double get_transmissibility() {
    return this->transmissibility;
  }

  double calculate_climate_multiplier(double seasonality_value);

  /**
   * @return the Epidemic's attack ratio
   * @see Epidemic::get_attack_rate()
   */
  double get_attack_rate();

  /**
   * @return the Epidemic's symptomatic attack ratio
   * @see Epidemic::get_attack_rate()
   */
  double get_symptomatic_attack_rate();

  /**
   * @return a pointer to this Disease's residual_immunity Age_Map
   */
  Age_Map* get_residual_immunity() const {
    return this->residual_immunity;
  }

  /**
   * @return a pointer to this Disease's at_risk Age_Map
   */
  Age_Map* get_at_risk() const {
    return this->at_risk;
  }

  /**
   * @return a pointer to this Disease's Evolution attribute
   */
  Evolution* get_evolution() {
    return this->evol;
  }

  /**
   * @param day the simulation day
   * @see Epidemic::print_stats(day);
   */
  void print_stats(int day) {
    this->epidemic->print_stats(day);
  }

  /**
   * @return the epidemic with which this Disease is associated
   */
  Epidemic* get_epidemic() {
    return this->epidemic;
  }

  /**
   * @return the probability that agent's will stay home
   */
  static double get_prob_stay_home();

  /**
   * @param the new probability that agent's will stay home
   */
  static void set_prob_stay_home(double);

  void get_disease_parameters();

  void increment_cohort_infectee_count(int day) {
    this->epidemic->increment_cohort_infectee_count(day);
  }
 
  bool gen_immunity_infection(double real_age);

  void initialize_evolution_reporting_grid(Regional_Layer* grid);

  double get_infectivity_threshold() {
    return this->infectivity_threshold;
  }
  
  double get_symptomaticity_threshold() {
    return this->symptomaticity_threshold;
  }

  void init_prior_immunity();

  void update(int day) {
    this->epidemic->update(day);
  }

  double get_face_mask_transmission_efficacy() {
    return this->face_mask_transmission_efficacy;
  }

  double get_face_mask_susceptibility_efficacy() {
    return this->face_mask_susceptibility_efficacy;
  }

  double get_hand_washing_transmission_efficacy() {
    return this->hand_washing_transmission_efficacy;
  }

  double get_hand_washing_susceptibility_efficacy() {
    return this->hand_washing_susceptibility_efficacy;
  }

  double get_face_mask_plus_hand_washing_transmission_efficacy() {
    return this->face_mask_plus_hand_washing_transmission_efficacy;
  }

  double get_face_mask_plus_hand_washing_susceptibility_efficacy() {
    return this->face_mask_plus_hand_washing_susceptibility_efficacy;
  }

  bool is_fatal(double real_age, double symptoms, int days_symptomatic);
  bool is_fatal(Person* per, double symptoms, int days_symptomatic);

  double get_min_symptoms_for_seek_healthcare() {
    return this->min_symptoms_for_seek_healthcare;
  }

  double get_hospitalization_prob(Person* per);

  double get_outpatient_healthcare_prob(Person* per);

  bool is_case_fatality_enabled() {
    return this->enable_case_fatality;
  }

  char* get_disease_name() {
    return this->disease_name;
  }

  void read_residual_immunity_by_FIPS();
  
  vector<double> get_residual_immunity_values_by_FIPS(int FIPS_string);
   
  char * get_natural_history_model() {
    return this->natural_history_model;
  }

private:

  int id;
  char disease_name[20];
  char natural_history_model[20];
  double transmissibility;

  // infectivity and symptomaticity thresholds used in Infection class to
  // determine if an agent is infectious/symptomatic at a given point in the
  // trajectory
  double infectivity_threshold;
  double symptomaticity_threshold;

  double seasonality_max, seasonality_min;
  double seasonality_Ka, seasonality_Kb;

  // moved from natural_history
  double asymp_infectivity;
  double symp_infectivity;
  int infection_model;
  int max_days_latent;
  int max_days_asymp;
  int max_days_symp;
  double *days_latent;
  double *days_asymp;
  double *days_symp;
  double prob_symptomatic;
  Age_Map *age_specific_prob_symptomatic;

  // case fatality parameters
  int enable_case_fatality;
  double min_symptoms_for_case_fatality;
  Age_Map* case_fatality_age_factor;
  double* case_fatality_prob_by_day;
  int max_days_case_fatality_prob;

  //Hospitalization and outpatient healthcare parameters
  double min_symptoms_for_seek_healthcare;
  Age_Map* hospitalization_prob;
  Age_Map* outpatient_healthcare_prob;

  double immunity_loss_rate;
  Epidemic* epidemic;
  Age_Map* residual_immunity;
  Age_Map* at_risk;
  Age_Map* infection_immunity_prob;
  Natural_History* natural_history;
  Evolution* evol;

  // intervention efficacies
  int enable_face_mask_usage;
  double face_mask_transmission_efficacy;
  double face_mask_susceptibility_efficacy;

  int enable_hand_washing;
  double hand_washing_transmission_efficacy;
  double hand_washing_susceptibility_efficacy;
  double face_mask_plus_hand_washing_transmission_efficacy;
  double face_mask_plus_hand_washing_susceptibility_efficacy;

  double R0;
  double R0_a;
  double R0_b;
  
  /// added for residual_immunity_by FIPS
  std::map<int, vector<double> > residual_immunity_by_FIPS;
  /// end added

};

#endif // _FRED_Disease_H
