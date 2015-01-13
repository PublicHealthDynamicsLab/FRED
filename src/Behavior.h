/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Behavior.h
//

#ifndef _FRED_BEHAVIOR_H
#define _FRED_BEHAVIOR_H

#include <stdio.h>
#include <vector>

class Person;
class Attitude;
class Household;

enum Behavior_strategy {REFUSE, ACCEPT, FLIP, IMITATE_PREVALENCE, IMITATE_CONSENSUS, IMITATE_COUNT, HBM};
#define BEHAVIOR_STRATEGIES 7
#define NUMBER_WEIGHTS 7

typedef struct {
  // COMMON
  char name[32];
  int first;
  int enabled;
  int frequency;
  int strategy_cdf_size;
  double strategy_cdf[BEHAVIOR_STRATEGIES];
  int strategy_dist[BEHAVIOR_STRATEGIES];
  // FLIP
  double min_prob;
  double max_prob;
  // IMITATE params
  int imitation_enabled;
  // IMITATE PREVALENCE
  double imitate_prevalence_weight[NUMBER_WEIGHTS];
  double imitate_prevalence_total_weight;
  double imitate_prevalence_update_rate;
  double imitate_prevalence_threshold;
  int imitate_prevalence_count;
  // IMITATE CONSENSUS
  double imitate_consensus_weight[NUMBER_WEIGHTS];
  double imitate_consensus_total_weight;
  double imitate_consensus_update_rate;
  double imitate_consensus_threshold;
  int imitate_consensus_count;
  // IMITATE COUNT
  double imitate_count_weight[NUMBER_WEIGHTS];
  double imitate_count_total_weight;
  double imitate_count_update_rate;
  double imitate_count_threshold;
  int imitate_count_count;
  // HBM
  double susceptibility_threshold_distr[2];
  double severity_threshold_distr[2];
  double benefits_threshold_distr[2];
  double barriers_threshold_distr[2];
  double memory_decay_distr[2];
  double base_odds_ratio;
  double susceptibility_odds_ratio;
  double severity_odds_ratio;
  double benefits_odds_ratio;
  double barriers_odds_ratio;
} Behavior_params;

typedef struct {
  int * yes_responses;
  int * total_responses;
  int yes;
  int total;
  int * previous_yes_responses;
  int * previous_total_responses;
  int previous_yes;
  int previous_total;
  int last_update;
} Behavior_survey;


class Behavior {

public:

  /**
   * Constructor
   * @param p a pointer to the agent who will exhibit this behavior
   */
  Behavior(Person *p);
  ~Behavior();
  void initialize_adult_behavior(Person * person);

  /**
    * Perform the daily update for this object
    *
    * @param day the simulation day
    */
  void update(int day);

  bool adult_is_staying_home(int day);
  bool adult_is_taking_sick_leave(int day);
  bool child_is_staying_home(int day);
  bool acceptance_of_vaccine();
  bool acceptance_of_another_vaccine_dose();
  bool child_acceptance_of_vaccine();
  bool child_acceptance_of_another_vaccine_dose();
  void set_adult_decision_maker(Person * person) { adult_decision_maker = person; }
  Person * get_adult_decision_maker() { return adult_decision_maker; }
  void select_adult_decision_maker(Person *unavailable_person);
  void terminate();
  //void set_dependent(Person *person) { dependent_children.insert(person); }
  //void remove_dependent(Person *person) { dependent_children.erase(person); }

private:
  Person * self;
  Person * adult_decision_maker;
  //set<Person *> dependent_children;
  void get_parameters();
  void get_parameters_for_behavior(char * behavior_name, Behavior_params * par);
  Attitude * setup(Person * self, Behavior_params * params, Behavior_survey * survey);
  void report_distribution(Behavior_params * params);
  Person * select_adult(Household *h, int relationship, Person * unavailable_person);
  
  Attitude * take_sick_leave;
  Attitude * stay_home_when_sick;
  Attitude * keep_child_home_when_sick;
  Attitude * accept_vaccine;
  Attitude * accept_vaccine_dose;
  Attitude * accept_vaccine_for_child;
  Attitude * accept_vaccine_dose_for_child;

  static bool parameters_are_set;
  static int number_of_vaccines;

  // run-time parameters for behaviors
  static Behavior_params stay_home_when_sick_params;
  static Behavior_params take_sick_leave_params;
  static Behavior_params keep_child_home_when_sick_params;
  static Behavior_params accept_vaccine_params;
  static Behavior_params accept_vaccine_dose_params;
  static Behavior_params accept_vaccine_for_child_params;
  static Behavior_params accept_vaccine_dose_for_child_params;
  
  static Behavior_survey stay_home_when_sick_survey;
  static Behavior_survey take_sick_leave_survey;
  static Behavior_survey keep_child_home_when_sick_survey;
  static Behavior_survey accept_vaccine_survey;
  static Behavior_survey accept_vaccine_dose_survey;
  static Behavior_survey accept_vaccine_for_child_survey;
  static Behavior_survey accept_vaccine_dose_for_child_survey;

protected:
  friend class Person;
  Behavior() { };
};

inline
void Behavior::report_distribution(Behavior_params * params) {
  if (params->first) {
    printf("BEHAVIOR %s dist: ", params->name);
    for (int i = 0; i < BEHAVIOR_STRATEGIES; i++) {
      printf("%d ", params->strategy_dist[i]);
    }
    printf("\n"); fflush(stdout);
    params->first = 0;
  }
}

#endif // _FRED_BEHAVIOR_H

