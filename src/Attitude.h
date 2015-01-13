/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Attitude.h
//

#ifndef _FRED_ATTITUDE_H
#define _FRED_ATTITUDE_H

#include "Random.h"
#include "Behavior.h"

class Person;
class Perceptions;
class Place;

enum IMITATION_WEIGHTS {HOUSEHOLD_WT, NEIGHBORHOOD_WT, SCHOOL_WT, WORK_WT, ALL_WT};

class Attitude {
 public:
  /**
   * Default constructor
   */
  Attitude(Person * self);
  ~Attitude() {}

  /**
    * Perform the daily update for this object
    *
    * @param day the simulation day
    */
  void update(int day);

  // access functions
  void set_params(Behavior_params * _params) { params = _params; }
  void set_strategy(Behavior_strategy t) {
    strategy = t;
    switch (strategy) {
    case IMITATE_PREVALENCE:
      weight = params->imitate_prevalence_weight;
      threshold = -1.0;
      total_weight = params->imitate_prevalence_total_weight;
      update_rate = params->imitate_prevalence_update_rate;
      break;
    case IMITATE_CONSENSUS:
      weight = params->imitate_consensus_weight;
      threshold = params->imitate_consensus_threshold;
      total_weight = params->imitate_consensus_total_weight;
      update_rate = params->imitate_consensus_update_rate;
      break;
    case IMITATE_COUNT:
      weight = params->imitate_count_weight;
      threshold = params->imitate_count_threshold;
      total_weight = params->imitate_count_total_weight;
      update_rate = params->imitate_count_update_rate;
      break;
    case HBM:
      setup_hbm();
      break;
    default:
      break;
    }
  }
  void set_probability(double p) { probability = p; }
  void set_frequency(int _frequency) { frequency = _frequency; }
  void set_willing(bool decision) { willing = decision; }
  void set_survey(Behavior_survey * _survey) { survey = _survey; }
  int get_strategy() { return strategy; }
  double get_probability () { return probability; }
  double get_frequency () { return frequency; }
  bool is_willing() { return willing; }

 private:
  // private methods
  void reset_survey(int day);
  void update_survey();
  void record_survey_response(Place * place);
  void imitate();
  void setup_hbm();
  void update_hbm(int day);
  bool compare_to_odds_ratio(int disease_id);
  void update_perceived_severity(int day, int disease_id);
  void update_perceived_susceptibility(int day, int disease_id);
  void update_perceived_benefits(int day, int disease_id);
  void update_perceived_barriers(int day, int disease_id);

  // private data
  Person * self;
  Behavior_strategy strategy;
  Behavior_params * params;
  double probability;
  int frequency;
  int expiration;
  bool willing;

  // Imitation data 
  Behavior_survey * survey;
  double * weight;
  double threshold;
  double total_weight;
  double update_rate;
  
  // Health Belief Model

  // Memory
  double * cumm_susceptibility;        // per disease
  double * cumm_severity;        // per disease
  double memory_decay;
  
  // Perceptions
  int total_deaths;
  Perceptions * perceptions;
  double * perceived_susceptibility;    // per disease
  double * perceived_severity;      // per disease
  double * perceived_benefits;      // per disease
  double * perceived_barriers;      // per disease

  // Thresholds for dichotomous variables
  double susceptibility_threshold;
  double severity_threshold;
  double benefits_threshold;
  double barriers_threshold;

};

#endif // _FRED_ATTITUDE_H

