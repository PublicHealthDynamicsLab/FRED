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
// File: Condition.h
//

#ifndef _FRED_Condition_H
#define _FRED_Condition_H

#include <vector>
#include <map>
#include <string>

using namespace std;

class Age_Map;
class Epidemic;
class Natural_History;
class Person;
class Transmission;

class Condition {
public:

  /**
   * Default constructor
   */
  Condition();
  virtual ~Condition();

  void get_parameters(int condition, string name);

  /**
   * Set all of the attributes for the Condition
   */
  void setup();

  void prepare();

  /**
   * @return this Condition's id
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

  void report(int day);

  /**
   * @return the epidemic with which this Condition is associated
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

  void get_condition_parameters();

  void increment_cohort_infectee_count(int day);
 
  void update(int day);

  virtual void terminate_person(Person* person, int day);

  double get_min_symptoms_for_seek_healthcare() {
    return this->min_symptoms_for_seek_healthcare;
  }

  double get_hospitalization_prob(Person* per);

  double get_outpatient_healthcare_prob(Person* per);

  char* get_condition_name() {
    return this->condition_name;
  }

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

  int get_number_of_states();

  string get_state_name(int i);

  int get_state_from_name(string state_name);

  void end_of_run();

  bool is_hiv() {
    return is_hiv_infection;
  }

  int get_total_symptomatic_cases();
  int get_total_severe_symptomatic_cases();
  bool is_household_confinement_enabled(Person *person);
  bool is_confined_to_household(Person *person);
  bool get_daily_household_confinement(Person *person);

  // double get_transmission_modifier(int condition_id);
  // double get_susceptibility_modifier(int condition_id);

private:

  // condition identifiers
  int id;
  char condition_name[256];

  // the course of infection within a host
  char natural_history_model[256];
  Natural_History* natural_history;

  // how the condition spreads
  char transmission_mode[256];
  Transmission* transmission;

  // contagiousness
  double transmissibility;
  
  // the course of infection at the population level
  Epidemic* epidemic;

  // variation over time of year
  double seasonality_max, seasonality_min;
  double seasonality_Ka, seasonality_Kb;

  // hospitalization and outpatient healthcare parameters
  double min_symptoms_for_seek_healthcare;
  Age_Map* hospitalization_prob;
  Age_Map* outpatient_healthcare_prob;

  // condition is HIV
  bool is_hiv_infection;
};

#endif // _FRED_Condition_H
