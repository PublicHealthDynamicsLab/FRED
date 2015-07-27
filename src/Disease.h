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
#include <map>
#include <string>
using namespace std;

#include "Epidemic.h"
#include "Global.h"

class Age_Map;
class Evolution;
class Infection;
class IntraHost;
class Person;
class Strain;
class Strain_Data;
class StrainTable;
class Trajectory;


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

  //  /**
  //   * Print out information about this object
  //   */
  //  void print();

  /**
   * @return the intrahost model's days symptomatic
   * @see IntraHost::get_days_symp()
   */
  int get_days_symp();

  /**
   * @return the days recovered
   */
  int get_days_recovered();

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

  /**
   * @param strain the strain of the disease
   * @return the strain's transmissibility
   */
  double get_transmissibility(int strain);

  /**
   * @param seasonality_value meterological condition (eg specific humidity in kg/kg)
   * @return the multiplier calculated from the seasonality condition; attenuates transmissibility
   */
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

  Trajectory* get_trajectory(int age);

  /**
   * Add a person to the Epidemic's infectious place list
   * @param p pointer to a Place
   * @see Epidemic::add_infectious_place(Place* p)
   */
  void add_infectious_place(Place* p) {
    this->epidemic->add_infectious_place(p);
  }

  void activate_place(Place* place) {
    this->epidemic->activate_place(place);
  }

  void deactivate_place(Place* place) {
    this->epidemic->deactivate_place(place);
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

  int get_num_strains();
  int get_num_strain_data_elements(int strain);
  int get_strain_data_element(int strain, int i);
  const Strain_Data &get_strain_data(int strain);
  const Strain &get_strain(int strain_id);

  void get_disease_parameters();

  void become_susceptible(Person* person) {
    this->epidemic->become_susceptible(person);
  }

  void become_unsusceptible(Person* person) {
    this->epidemic->become_unsusceptible(person);
  }

  void become_exposed(Person* person, int day) {
    this->epidemic->become_exposed(person, day);
  }

  void become_infectious(Person* person) {
    this->epidemic->become_infectious(person);
  }

  void become_uninfectious(Person* person) {
    this->epidemic->become_uninfectious(person);
  }

  void become_symptomatic(Person* person) {
    this->epidemic->become_symptomatic(person);
  }

  void become_removed(Person* person, bool susceptible, bool infectious, bool symptomatic) {
    this->epidemic->become_removed(person, susceptible, infectious, symptomatic);
  }

  void become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic) {
    this->epidemic->become_immune(person, susceptible, infectious, symptomatic);
  }

  void increment_cohort_infectee_count(int day) {
    this->epidemic->increment_cohort_infectee_count(day);
  }
 
  bool gen_immunity_infection(double real_age);

  int add_strain(Strain* child_strain, double transmissibility, int parent_strain_id);
  int add_strain(Strain* child_strain, double transmissibility);

  void add_root_strain(int num_elements);
  void print_strain(int strain_id, stringstream &out);
  std::string get_strain_data_string(int strain_id);
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
   
private:

  char disease_name[FRED_STRING_SIZE];
  int id;
  double transmissibility;

  // infectivity and symptomaticity thresholds used in Infection class to
  // determine if an agent is infectious/symptomatic at a given point in the
  // trajectory
  double infectivity_threshold;
  double symptomaticity_threshold;

  double seasonality_max, seasonality_min;
  double seasonality_Ka, seasonality_Kb;

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
  StrainTable* strain_table;
  IntraHost* ihm;
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
