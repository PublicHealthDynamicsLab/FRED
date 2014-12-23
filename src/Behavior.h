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
// File: Behavior.h
//

#ifndef _FRED_BEHAVIOR_H
#define _FRED_BEHAVIOR_H

#include <stdio.h>
#include "Global.h"

class Intention;
class Person;
class Household;

// the following enum defines symbolic names for
// current behaviors.  The last element should always be
// NUM_BEHAVIORS

namespace Behavior_index {
  enum e {
    TAKE_SICK_LEAVE,
    STAY_HOME_WHEN_SICK,
    KEEP_CHILD_HOME_WHEN_SICK,
    ACCEPT_VACCINE,
    ACCEPT_VACCINE_DOSE,
    ACCEPT_VACCINE_FOR_CHILD,
    ACCEPT_VACCINE_DOSE_FOR_CHILD,
    NUM_BEHAVIORS
  };
};

// the following enum defines symbolic names for
// behavior strategies.  The last element should always be
// NUM_BEHAVIOR_CHANGE_MODELS

namespace Behavior_change_model_enum {
  enum e {
    REFUSE, ACCEPT, FLIP, IMITATE_PREVALENCE,
    IMITATE_CONSENSUS, IMITATE_COUNT, HBM, NUM_BEHAVIOR_CHANGE_MODELS
  };
};


#define NUM_WEIGHTS 7

typedef struct {
  char name[32];
  int enabled;
  int frequency;
  int behavior_change_model_cdf_size;
  double behavior_change_model_cdf[Behavior_change_model_enum::NUM_BEHAVIOR_CHANGE_MODELS];
  int behavior_change_model_population[Behavior_change_model_enum::NUM_BEHAVIOR_CHANGE_MODELS];
  // FLIP
  double min_prob;
  double max_prob;
  // IMITATE params
  int imitation_enabled;
  // IMITATE PREVALENCE
  double imitate_prevalence_weight[NUM_WEIGHTS];
  double imitate_prevalence_total_weight;
  double imitate_prevalence_update_rate;
  double imitate_prevalence_threshold;
  int imitate_prevalence_count;
  // IMITATE CONSENSUS
  double imitate_consensus_weight[NUM_WEIGHTS];
  double imitate_consensus_total_weight;
  double imitate_consensus_update_rate;
  double imitate_consensus_threshold;
  int imitate_consensus_count;
  // IMITATE COUNT
  double imitate_count_weight[NUM_WEIGHTS];
  double imitate_count_total_weight;
  double imitate_count_update_rate;
  double imitate_count_threshold;
  int imitate_count_count;
  // HBM
  double susceptibility_threshold_distr[2];
  double severity_threshold_distr[2];
  double benefits_threshold_distr[2];
  double barriers_threshold_distr[2];
  double base_odds_ratio;
  double susceptibility_odds_ratio;
  double severity_odds_ratio;
  double benefits_odds_ratio;
  double barriers_odds_ratio;
} Behavior_params;


class Behavior {
public:

  ~Behavior();
  static void initialize_static_variables();
  void setup(Person * self);
  void update( Person * self, int day );
  void terminate( Person * self );

  // methods to return this person's intention to take an action:
  bool adult_is_staying_home();
  bool adult_is_taking_sick_leave();
  bool child_is_staying_home();
  bool acceptance_of_vaccine();
  bool acceptance_of_another_vaccine_dose();
  bool child_acceptance_of_vaccine();
  bool child_acceptance_of_another_vaccine_dose();

  // access functions
  Person * get_health_decision_maker() { return health_decision_maker; }
  bool is_health_decision_maker() { return health_decision_maker == NULL; }

  void set_health_decision_maker(Person * p) {
    health_decision_maker = p; 
    if (p != NULL) delete_intentions();
  }

  void become_health_decision_maker(Person * self);
  static Behavior_params * get_behavior_params(int i) { return behavior_params[i]; }
  static void print_params(int n);

private:
  // private data
  Person * health_decision_maker;
  Intention ** intention;
 
  // run-time parameters for behaviors
  static bool parameters_are_set;
  static Behavior_params ** behavior_params;

  // private methods
  void setup_intentions(Person * _self);
  static void get_parameters();
  static void get_parameters_for_behavior(char * behavior_name, int n);
  Person * select_adult(Household *h, int relationship, Person * self);
  void delete_intentions();

protected:
  friend class Person;
  Behavior();

};

#endif // _FRED_BEHAVIOR_H

