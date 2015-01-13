/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Behavior.cc
//

#include "Behavior.h"
#include "Person.h"
#include "Global.h"
#include "Attitude.h"
#include "Random.h"
#include "Params.h"
#include "Place_List.h"
#include "Utils.h"
#include "Household.h"

//Private static variables that will be set by parameter lookups
int Behavior::number_of_vaccines = 0;
Behavior_params Behavior::stay_home_when_sick_params = {};
Behavior_params Behavior::take_sick_leave_params = {};
Behavior_params Behavior::keep_child_home_when_sick_params = {};
Behavior_params Behavior::accept_vaccine_params = {};
Behavior_params Behavior::accept_vaccine_dose_params = {};
Behavior_params Behavior::accept_vaccine_for_child_params = {};
Behavior_params Behavior::accept_vaccine_dose_for_child_params = {};

Behavior_survey Behavior::stay_home_when_sick_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::take_sick_leave_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::keep_child_home_when_sick_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::accept_vaccine_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::accept_vaccine_dose_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::accept_vaccine_for_child_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};
Behavior_survey Behavior::accept_vaccine_dose_for_child_survey = {NULL, NULL, 0,0,NULL, NULL, 0,0,-1};

//Private static variable to assure we only lookup parameters once
bool Behavior::parameters_are_set = false;

Behavior::Behavior(Person * person) {
  self = person;

  // initialize to null behaviors
  stay_home_when_sick = NULL;
  take_sick_leave = NULL;
  keep_child_home_when_sick = NULL;
  accept_vaccine = NULL;
  accept_vaccine_dose = NULL;
  accept_vaccine_for_child = NULL;
  accept_vaccine_dose_for_child = NULL;
  adult_decision_maker = NULL;

  // get parameters (just once)
  if (Behavior::parameters_are_set == false) {
    get_parameters();
  }

  // adults make their own decisions
  if (self->is_adult()) {
    initialize_adult_behavior(person);
  }
}


Behavior::~Behavior() {
  if (stay_home_when_sick != NULL) delete stay_home_when_sick;
  if (take_sick_leave != NULL) delete take_sick_leave;
  if (keep_child_home_when_sick != NULL) delete keep_child_home_when_sick;
  if (accept_vaccine != NULL) delete accept_vaccine;
  if (accept_vaccine_dose != NULL) delete accept_vaccine_dose;
  if (accept_vaccine_for_child != NULL) delete accept_vaccine_for_child;
  if (accept_vaccine_dose_for_child != NULL) delete accept_vaccine_dose_for_child;
}


void Behavior::initialize_adult_behavior(Person * person) {
  if (Global::Enable_Behaviors == 0) return;
  Utils::fred_verbose(1,"init adult behavior for agent %d age %d\n", self->get_id(),self->get_age());
  if (stay_home_when_sick_params.enabled) {
    stay_home_when_sick = setup(self, &stay_home_when_sick_params, &stay_home_when_sick_survey);
  }
  
  if (take_sick_leave_params.enabled) {
    take_sick_leave = setup(self, &take_sick_leave_params, &take_sick_leave_survey);
  }
  
  if (keep_child_home_when_sick_params.enabled) {
    keep_child_home_when_sick = setup(self, &keep_child_home_when_sick_params, &keep_child_home_when_sick_survey);
  }
  
  if (Behavior::number_of_vaccines > 0) {
    if (accept_vaccine_params.enabled) {
      accept_vaccine = setup(self, &accept_vaccine_params, &accept_vaccine_survey);
    }
    if (accept_vaccine_dose_params.enabled) {
      accept_vaccine_dose = setup(self, &accept_vaccine_dose_params, &accept_vaccine_dose_survey);
    }
    if (accept_vaccine_for_child_params.enabled) {
      accept_vaccine_for_child = setup(self, &accept_vaccine_for_child_params, &accept_vaccine_for_child_survey);
    }
    if (accept_vaccine_dose_for_child_params.enabled) {
      accept_vaccine_dose_for_child = setup(self, &accept_vaccine_dose_for_child_params, &accept_vaccine_dose_for_child_survey);
    }
  }

  // adult make their own decisions
  adult_decision_maker = self;
}



Attitude * Behavior::setup(Person * self, Behavior_params * params, Behavior_survey * survey) {
  double prob;
  int frequency;
  Attitude * behavior = new Attitude(self);
  int strategy_index = draw_from_distribution(params->strategy_cdf_size, params->strategy_cdf);
  params->strategy_dist[strategy_index]++;
  Behavior_strategy strategy = (Behavior_strategy) strategy_index;
  behavior->set_params(params);
  behavior->set_strategy(strategy);
  // printf("behavior %s setup strategy = %d\n", params->name, strategy);

  switch (strategy) {

  case REFUSE:
    behavior->set_strategy(REFUSE);
    behavior->set_willing(false);
    behavior->set_frequency(0);
    break;

  case ACCEPT:
    behavior->set_strategy(ACCEPT);
    behavior->set_willing(true);
    behavior->set_frequency(0);
    break;

  case FLIP:
    behavior->set_strategy(FLIP);
    prob = URAND(params->min_prob, params->max_prob); 
    behavior->set_probability(prob);
    if (params->frequency > 0) 
      frequency = IRAND(1, params->frequency);
    else
      frequency = IRAND(1, params->frequency);
    behavior->set_willing(RANDOM() < prob);
    behavior->set_frequency(frequency);
    break;

  case IMITATE_PREVALENCE:
    behavior->set_strategy(IMITATE_PREVALENCE);
  case IMITATE_CONSENSUS:
    behavior->set_strategy(IMITATE_CONSENSUS);
  case IMITATE_COUNT:
    behavior->set_strategy(IMITATE_COUNT);

    // common to all imitation strategies:
    params->imitation_enabled = 1;
    behavior->set_survey(survey);
    prob = URAND(params->min_prob, params->max_prob); 
    // printf("min_prob %f  max_prob %f  prob %f\n", params->min_prob, params->max_prob, prob);
    behavior->set_probability(prob);
    if (params->frequency > 0) 
      frequency = IRAND(1, params->frequency);
    else
      frequency = IRAND(1, params->frequency);
    behavior->set_willing(RANDOM() < prob);
    behavior->set_frequency(frequency);
    break;

  case HBM:
    behavior->set_strategy(HBM);
    prob = URAND(params->min_prob, params->max_prob); 
    behavior->set_probability(prob);
    if (params->frequency > 0) 
      frequency = IRAND(1, params->frequency);
    else
      frequency = IRAND(1, params->frequency);
    behavior->set_willing(RANDOM() < prob);
    behavior->set_frequency(frequency);
    break;
  }
  return behavior;
}


void Behavior::get_parameters() {
  if (Behavior::parameters_are_set == true) return;
  if (Global::Enable_Behaviors) {
    get_parameters_for_behavior((char *) "stay_home_when_sick", &stay_home_when_sick_params);
    get_parameters_for_behavior((char *) "take_sick_leave", &take_sick_leave_params);
    get_parameters_for_behavior((char *) "keep_child_home_when_sick", &keep_child_home_when_sick_params);
    get_parameters_for_behavior((char *) "accept_vaccine", &accept_vaccine_params);
    get_parameters_for_behavior((char *) "accept_vaccine_dose", &accept_vaccine_dose_params);
    get_parameters_for_behavior((char *) "accept_vaccine_for_child", &accept_vaccine_for_child_params);
    get_parameters_for_behavior((char *) "accept_vaccine_dose_for_child", &accept_vaccine_dose_for_child_params);
  }
  Behavior::parameters_are_set = true;
}

void Behavior::get_parameters_for_behavior(char * behavior_name, Behavior_params * params) {
  char param_str[256];

  strcpy(params->name, behavior_name);

  sprintf(param_str, "%s_enabled", behavior_name);
  Params::get_param(param_str, &(params->enabled));

  sprintf(param_str, "%s_strategy_distribution", behavior_name);
  params->strategy_cdf_size = Params::get_param_vector(param_str , params->strategy_cdf);
  
  // convert to cdf
  double stotal = 0;
  for (int i = 0; i < params->strategy_cdf_size; i++) stotal += params->strategy_cdf[i];
  if (stotal != 100.0 && stotal != 1.0) {
    Utils::fred_abort("Bad distribution %s params_str\nMust sum to 1.0 or 100.0\n", param_str);
  }
  double cumm = 0.0;
  for (int i = 0; i < params->strategy_cdf_size; i++){
    params->strategy_cdf[i] /= stotal;
    params->strategy_cdf[i] += cumm;
    cumm = params->strategy_cdf[i];
  }

  sprintf(param_str, "%s_frequency", behavior_name);
  Params::get_param(param_str, &(params->frequency));

  for (int i = 0; i < BEHAVIOR_STRATEGIES; i++)
    params->strategy_dist[i] = 0;
  params->first = 1;

  // FLIP behavior parameters

  sprintf(param_str, "%s_min_prob", behavior_name);
  Params::get_param(param_str, &(params->min_prob));

  sprintf(param_str, "%s_max_prob", behavior_name);
  Params::get_param(param_str, &(params->max_prob));

  // IMITATE PREVALENCE behavior parameters

  sprintf(param_str, "%s_imitate_prevalence_weights", behavior_name);
  Params::get_param_vector(param_str , params->imitate_prevalence_weight);

  params->imitate_prevalence_total_weight = 0.0;
  for (int i = 0; i < NUMBER_WEIGHTS; i++)
    params->imitate_prevalence_total_weight += params->imitate_prevalence_weight[i];

  sprintf(param_str, "%s_imitate_prevalence_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_prevalence_update_rate));

  // IMITATE CONSENSUS behavior parameters

  sprintf(param_str, "%s_imitate_consensus_weights", behavior_name);
  Params::get_param_vector(param_str , params->imitate_consensus_weight);

  params->imitate_consensus_total_weight = 0.0;
  for (int i = 0; i < NUMBER_WEIGHTS; i++)
    params->imitate_consensus_total_weight += params->imitate_consensus_weight[i];

  sprintf(param_str, "%s_imitate_consensus_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_consensus_update_rate));

  sprintf(param_str, "%s_imitate_consensus_threshold", behavior_name);
  Params::get_param(param_str, &(params->imitate_consensus_threshold));

  // IMITATE COUNT behavior parameters

  sprintf(param_str, "%s_imitate_count_weights", behavior_name);
  Params::get_param_vector(param_str , params->imitate_count_weight);

  params->imitate_count_total_weight = 0.0;
  for (int i = 0; i < NUMBER_WEIGHTS; i++)
    params->imitate_count_total_weight += params->imitate_count_weight[i];

  sprintf(param_str, "%s_imitate_count_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_count_update_rate));

  sprintf(param_str, "%s_imitate_count_threshold", behavior_name);
  Params::get_param(param_str, &(params->imitate_count_threshold));

  params->imitation_enabled = 0;

  // HBM parameters

  sprintf(param_str, "%s_susceptibility_threshold", behavior_name);
  int n = Params::get_param_vector(param_str, params->susceptibility_threshold_distr);
  if (n != 2) { 
    Utils::fred_abort("bad %s\n", param_str); 
  }
  
  sprintf(param_str, "%s_severity_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->severity_threshold_distr);
  if (n != 2) {  
    Utils::fred_abort("bad %s\n", param_str); 
  }
  
  sprintf(param_str, "%s_benefits_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->benefits_threshold_distr);
  if (n != 2) { 
    Utils::fred_abort("bad %s\n", param_str); 
  }
  
  sprintf(param_str, "%s_barriers_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->barriers_threshold_distr);
  if (n != 2) {  
    Utils::fred_abort("bad %s\n", param_str); 
  }
  
  sprintf(param_str, "%s_memory_decay", behavior_name);
  n = Params::get_param_vector(param_str, params->memory_decay_distr);
  if (n != 2) {  
    Utils::fred_abort("bad %s\n", param_str);  
  }
  
  sprintf(param_str, "%s_base_odds_ratio", behavior_name);
  Params::get_param(param_str, &(params->base_odds_ratio));

  sprintf(param_str, "%s_susceptibility_odds_ratio", behavior_name);
  Params::get_param(param_str, &(params->susceptibility_odds_ratio));

  sprintf(param_str, "%s_severity_odds_ratio", behavior_name);
  Params::get_param(param_str, &(params->severity_odds_ratio));

  sprintf(param_str, "%s_benefits_odds_ratio", behavior_name);
  Params::get_param(param_str, &(params->benefits_odds_ratio));

  sprintf(param_str, "%s_barriers_odds_ratio", behavior_name);
  Params::get_param(param_str, &(params->barriers_odds_ratio));

}

void Behavior::update(int day) {

  if (Global::Enable_Behaviors == 0) return;

  if (self != adult_decision_maker) return;

  if (stay_home_when_sick_params.enabled) {
    report_distribution(&stay_home_when_sick_params);
    stay_home_when_sick->update(day);
  }

  if (take_sick_leave_params.enabled) {
    report_distribution(&take_sick_leave_params);
    take_sick_leave->update(day);
  }

  if (keep_child_home_when_sick_params.enabled) {
    report_distribution(&keep_child_home_when_sick_params);
    keep_child_home_when_sick->update(day);
  }

  if (accept_vaccine_params.enabled) {
    report_distribution(&accept_vaccine_params);
    accept_vaccine->update(day);
  }

  if (accept_vaccine_dose_params.enabled) {
    report_distribution(&accept_vaccine_dose_params);
    accept_vaccine_dose->update(day);
  }

  if (accept_vaccine_for_child_params.enabled) {
    report_distribution(&accept_vaccine_for_child_params);
    accept_vaccine_for_child->update(day);
  }

  if (accept_vaccine_dose_for_child_params.enabled) {
    report_distribution(&accept_vaccine_dose_for_child_params);
    accept_vaccine_dose_for_child->update(day);
  }
}

bool Behavior::adult_is_staying_home(int day) {

  assert(Global::Enable_Behaviors > 0);

  if (stay_home_when_sick_params.enabled == false)
    return false;

  if (stay_home_when_sick == NULL) {
    printf("Help! stay_home_when_sick not defined for agent %d  age %d\n", self->get_id(), self->get_age());
  }
  assert(stay_home_when_sick != NULL);
  return stay_home_when_sick->is_willing();
}

bool Behavior::adult_is_taking_sick_leave(int day) {

  assert(Global::Enable_Behaviors > 0);

  if (self != adult_decision_maker) {
    return false;
  }

  if (take_sick_leave_params.enabled == false)
    return false;

  if (take_sick_leave == NULL) {
    printf("Help! take_sick_leave not defined for agent %d  age %d\n", self->get_id(), self->get_age());
  }
  assert(take_sick_leave != NULL);
  return take_sick_leave->is_willing();
}

bool Behavior::child_is_staying_home(int day) {
  assert(Global::Enable_Behaviors > 0);
  
  if (keep_child_home_when_sick_params.enabled == false)
    return false;

  if (self != adult_decision_maker) {
    assert(adult_decision_maker != NULL);
    return adult_decision_maker->get_behavior()->child_is_staying_home(day);
  }

  // adult deciding
  assert(keep_child_home_when_sick != NULL);
  return keep_child_home_when_sick->is_willing();
}

bool Behavior::acceptance_of_vaccine() {

  assert(Global::Enable_Behaviors > 0);

  if (accept_vaccine_params.enabled == false)
    return true;

  if (self != adult_decision_maker) {
    assert(adult_decision_maker != NULL);
    return adult_decision_maker->get_behavior()->acceptance_of_vaccine();
  }

  // adult deciding
  assert(accept_vaccine != NULL);
  return accept_vaccine->is_willing();
}

bool Behavior::acceptance_of_another_vaccine_dose() {

  assert(Global::Enable_Behaviors > 0);

  if (accept_vaccine_dose_params.enabled == false)
    return true;

  if (self != adult_decision_maker) {
    assert(adult_decision_maker != NULL);
    return adult_decision_maker->get_behavior()->acceptance_of_another_vaccine_dose();
  }

  // adult deciding
  assert(accept_vaccine_dose != NULL);
  return accept_vaccine_dose->is_willing();
}

bool Behavior::child_acceptance_of_vaccine() {

  assert(Global::Enable_Behaviors > 0);

  if (accept_vaccine_for_child_params.enabled == false)
    return false;

  if (self != adult_decision_maker) {
    assert(adult_decision_maker != NULL);
    return adult_decision_maker->get_behavior()->child_acceptance_of_vaccine();
  }

  // adult deciding
  assert(accept_vaccine_for_child != NULL);
  return accept_vaccine_for_child->is_willing();
}

bool Behavior::child_acceptance_of_another_vaccine_dose() {

  assert(Global::Enable_Behaviors > 0);

  if (accept_vaccine_dose_for_child_params.enabled == false)
    return false;

  if (self != adult_decision_maker) {
    assert(adult_decision_maker != NULL);
    return adult_decision_maker->get_behavior()->child_acceptance_of_another_vaccine_dose();
  }

  // adult deciding
  assert(accept_vaccine_dose_for_child != NULL);
  return accept_vaccine_dose_for_child->is_willing();
}

void Behavior::select_adult_decision_maker(Person *unavailable_person) {

  if (Global::Enable_Behaviors == 0) return;

  if (self->is_adult()) {
    set_adult_decision_maker(self);
  }
  else {
    int relationship = self->get_relationship();
    Household * h = (Household *) self->get_household();
    Person *person = select_adult(h, relationship, unavailable_person);
    // Anuroop
    if (person == NULL) {
      person = self;
    }
    if(person->is_adult() == false) {
      Utils::fred_verbose(0,"set_adult_decision_maker: No adult is available for child %d age %d ",
        self->get_id(), self->get_age());
      Utils::fred_verbose(0,"so new decision maker is agent %d age %d\n",
        person->get_id(), person->get_age());
      initialize_adult_behavior(person);
    }
    set_adult_decision_maker(person);
  }
}

Person * Behavior::select_adult(Household *h, int relationship, Person * unavailable_person) {
  int N = h->get_size();

  if (relationship == Global::GRANDCHILD) {

    // select first adult natural child or spouse thereof of householder parent, if any
    for (int i = 0; i < N; i++) {
      Person * person = h->get_housemate(i);
      if (person->is_adult() == false || person == unavailable_person)
	continue;
      int r = person->get_relationship();
      if (r == Global::SPOUSE || r == Global::CHILD || r == Global::SIBLING || r == Global::IN_LAW) {
	return person;
      }
    }

    // consider adult relative of householder, if any
    for (int i = 0; i < N; i++) {
      Person * person = h->get_housemate(i);
      if (person->is_adult() == false || person == unavailable_person)
	continue;
      int r = person->get_relationship();
      if (r == Global::PARENT || r == Global::OTHER_RELATIVE) {
	return person;
      }
    }
  }

  // select householder if an adult
  for (int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if (person->is_adult() == false || person == unavailable_person)
      continue;
    if (person->get_relationship() == Global::HOUSEHOLDER) {
      return person;
    }
  }
  
  // select householder's spouse if an adult
  for (int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if (person->is_adult() == false || person == unavailable_person)
      continue;
    if (person->get_relationship() == Global::SPOUSE) {
      return person;
    }
  }

  // select oldest available person
  int max_age = -1;
  Person * oldest = NULL;
  for (int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if (person->get_age() <= max_age || person == unavailable_person)
      continue;
    oldest = person;
    max_age = oldest->get_age();
  }
  return oldest;
}

void Behavior::terminate() {

  if (Global::Enable_Behaviors == 0) return;

  if (self != adult_decision_maker)
    return;

  if (Global::Verbose > 1) {
    printf("terminating behavior for agent %d age %d\n",
     self->get_id(), self->get_age());
  }

  // see if this person is the adult decision maker for any child in the household
  Household * household = (Household *) self->get_household();
  int size = household->get_size();
  for (int i = 0; i < size; i++) {
    Person * child = household->get_housemate(i);
    if (child != self && child->get_adult_decision_maker() == self) {
      if (Global::Verbose > 1) {
        printf("need new decision maker for agent %d age %d\n",
            child->get_id(), child->get_age());
      }
      child->select_adult_decision_maker(self);
      if (Global::Verbose > 1) {
        printf("new decision maker is %d age %d\n",
            child->get_adult_decision_maker()->get_id(),
            child->get_adult_decision_maker()->get_age());
        fflush(stdout);
      }
    }
  }
}
  
