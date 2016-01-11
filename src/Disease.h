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

#include <vector>
#include <map>
#include <string>

using namespace std;

class Age_Map;
class Epidemic;
class Infection;
class Natural_History;
class Person;
class Transmission;

class Disease {
public:

  /**
   * Default constructor
   */
  Disease();
  virtual ~Disease();

  void get_parameters(int disease, string name);

  /**
   * Set all of the attributes for the Disease
   */
  void setup();

  void prepare();

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
   * @param day the simulation day
   * @see Epidemic::print_stats(day);
   */
  void print_stats(int day);

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

  void increment_cohort_infectee_count(int day);
 
  void update(int day);

  void terminate(Person* person, int day);

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

  double get_min_symptoms_for_seek_healthcare() {
    return this->min_symptoms_for_seek_healthcare;
  }

  double get_hospitalization_prob(Person* per);

  double get_outpatient_healthcare_prob(Person* per);

  char* get_disease_name() {
    return this->disease_name;
  }

  void read_residual_immunity_by_FIPS();
  
  vector<double> get_residual_immunity_values_by_FIPS(int FIPS_string);
   
  char* get_natural_history_model() {
    return this->natural_history_model;
  }

  Natural_History* get_natural_history() {
    return this->natural_history;
  }

  // case fatality

  virtual bool is_case_fatality_enabled();

  virtual bool is_fatal(double real_age, double symptoms, int days_symptomatic);

  virtual bool is_fatal(Person* per, double symptoms, int days_symptomatic);

  // transmission mode

  char* get_transmission_mode() {
    return this->transmission_mode;
  }

  Transmission* get_transmission() {
    return this->transmission;
  }

  void become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic);

  bool assume_susceptible() {
    return this->make_all_susceptible;
  }

  void end_of_run();

private:

  // disease identifiers
  int id;
  char disease_name[20];

  // the course of infection within a host
  char natural_history_model[20];
  Natural_History* natural_history;
  bool make_all_susceptible;

  // how the disease spreads
  char transmission_mode[20];
  Transmission* transmission;

  // contagiousness
  double transmissibility;
  double R0;
  double R0_a;
  double R0_b;
  
  // the course of infection at the population level
  Epidemic* epidemic;
  Age_Map* at_risk;
  Age_Map* residual_immunity;
  std::map<int, vector<double> > residual_immunity_by_FIPS;

  // variation over time of year
  double seasonality_max, seasonality_min;
  double seasonality_Ka, seasonality_Kb;

  // behavioral intervention efficacies
  int enable_face_mask_usage;
  double face_mask_transmission_efficacy;
  double face_mask_susceptibility_efficacy;

  int enable_hand_washing;
  double hand_washing_transmission_efficacy;
  double hand_washing_susceptibility_efficacy;
  double face_mask_plus_hand_washing_transmission_efficacy;
  double face_mask_plus_hand_washing_susceptibility_efficacy;

  // hospitalization and outpatient healthcare parameters
  double min_symptoms_for_seek_healthcare;
  Age_Map* hospitalization_prob;
  Age_Map* outpatient_healthcare_prob;

};

#endif // _FRED_Disease_H
