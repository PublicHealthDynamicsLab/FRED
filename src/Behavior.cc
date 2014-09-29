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
// File: Behavior.cc
//
#include "Global.h"
#include "Behavior.h"
#include "Person.h"
#include "Intention.h"
#include "Random.h"
#include "Params.h"
#include "Place_List.h"
#include "Utils.h"
#include "Household.h"

//Private static variable to assure we only lookup parameters once
bool Behavior::parameters_are_set = false;
Behavior_params ** Behavior::behavior_params = new Behavior_params *[Behavior_index::NUM_BEHAVIORS];

/// static method called in main (Fred.cc)

void Behavior::initialize_static_variables() {
  if(Behavior::parameters_are_set == false) {
    for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
      Behavior::behavior_params[i] = new Behavior_params;
    }
    get_parameters();
    for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
      print_params(i);
    }
    Behavior::parameters_are_set = true;
  }
}

Behavior::Behavior() {
  // will be initialized in setup_intentions() if needed
  this->intention = NULL;
  // will be properly initialized in setup() after all agents are created
  this->health_decision_maker = NULL;
}

Behavior::~Behavior() {
  delete_intentions();
}

void Behavior::delete_intentions() {
  if(this->intention != NULL) {
    for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
      if(this->intention[i] != NULL) {
        // printf( "%p\n",intention[i]);
        delete this->intention[i];
      }
    }
    delete[] this->intention;
    this->intention = NULL;
  }
}

void Behavior::setup(Person * self) {

  assert(Behavior::parameters_are_set == true);
  if(!Global::Enable_Behaviors) {
    return;
  }

  // setup an adult
  if(self->is_adult()) {
    // adults do not have a separate health decision maker
    FRED_VERBOSE(1, "behavior_setup for adult %d age %d -- will make own health decisions\n",
        self->get_id(), self->get_age());
    this->health_decision_maker = NULL;
    setup_intentions(self);
    return;
  }

  // setup a child
  int relationship = self->get_relationship();
  Household * h = (Household *) self->get_household();
  Person * person = select_adult(h, relationship, self);

  // child is on its own
  if(person == NULL) {
    FRED_VERBOSE(1, "behavior_setup for child %d age %d -- will make own health decisions\n",
        self->get_id(), self->get_age());
    // no separate health decision maker
    this->health_decision_maker = NULL;
    setup_intentions(self);
    return;
  }

  // an older child is available
  if(person->is_adult() == false) {
    FRED_VERBOSE(0,
        "behavior_setup for child %d age %d -- minor person %d age %d will make health decisions\n",
        self->get_id(), self->get_age(), person->get_id(), person->get_age());
    this->health_decision_maker = person;
    person->become_health_decision_maker(person);
    return;
  }

  // an adult is available
  FRED_VERBOSE(0,
      "behavior_setup for child %d age %d -- adult person %d age %d will make health decisions\n",
      self->get_id(), self->get_age(), person->get_id(), person->get_age());
  this->health_decision_maker = person; // no need to setup atitudes for adults
  return;
}

void Behavior::setup_intentions(Person * self) {
  if(!Global::Enable_Behaviors) {
    return;
  }

  assert(this->intention == NULL);

  // create array of pointers to intentions
  this->intention = new Intention *[Behavior_index::NUM_BEHAVIORS];

  // initialize to null intentions
  for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
    this->intention[i] = NULL;
  }

  if(Behavior::behavior_params[Behavior_index::STAY_HOME_WHEN_SICK]->enabled) {
    this->intention[Behavior_index::STAY_HOME_WHEN_SICK] = new Intention(self, Behavior_index::STAY_HOME_WHEN_SICK);
  }
  
  if(Behavior::behavior_params[Behavior_index::TAKE_SICK_LEAVE]->enabled) {
    this->intention[Behavior_index::TAKE_SICK_LEAVE] = new Intention(self, Behavior_index::TAKE_SICK_LEAVE);
  }
  
  if(Behavior::behavior_params[Behavior_index::KEEP_CHILD_HOME_WHEN_SICK]->enabled) {
    this->intention[Behavior_index::KEEP_CHILD_HOME_WHEN_SICK] = new Intention(self, Behavior_index::KEEP_CHILD_HOME_WHEN_SICK);
  }
  
  if(Behavior::behavior_params[Behavior_index::ACCEPT_VACCINE]->enabled) {
    this->intention[Behavior_index::ACCEPT_VACCINE] = new Intention(self, Behavior_index::ACCEPT_VACCINE);
  }
  
  if(Behavior::behavior_params[Behavior_index::ACCEPT_VACCINE_DOSE]->enabled) {
    this->intention[Behavior_index::ACCEPT_VACCINE_DOSE] = new Intention(self, Behavior_index::ACCEPT_VACCINE_DOSE);
  }
  
  if(Behavior::behavior_params[Behavior_index::ACCEPT_VACCINE_FOR_CHILD]->enabled) {
    this->intention[Behavior_index::ACCEPT_VACCINE_FOR_CHILD] = new Intention(self, Behavior_index::ACCEPT_VACCINE_FOR_CHILD);
  }
  
  if(Behavior::behavior_params[Behavior_index::ACCEPT_VACCINE_DOSE_FOR_CHILD]->enabled) {
    this->intention[Behavior_index::ACCEPT_VACCINE_DOSE_FOR_CHILD] =
        new Intention(self, Behavior_index::ACCEPT_VACCINE_DOSE_FOR_CHILD);
  }
}

void Behavior::get_parameters() {
  if(Behavior::parameters_are_set) {
    return;
  }
  
  get_parameters_for_behavior((char *) "stay_home_when_sick", Behavior_index::STAY_HOME_WHEN_SICK);
  get_parameters_for_behavior((char *) "take_sick_leave", Behavior_index::TAKE_SICK_LEAVE);
  get_parameters_for_behavior((char *) "keep_child_home_when_sick", Behavior_index::KEEP_CHILD_HOME_WHEN_SICK);
  get_parameters_for_behavior((char *) "accept_vaccine", Behavior_index::ACCEPT_VACCINE);
  get_parameters_for_behavior((char *) "accept_vaccine_dose", Behavior_index::ACCEPT_VACCINE_DOSE);
  get_parameters_for_behavior((char *) "accept_vaccine_for_child", Behavior_index::ACCEPT_VACCINE_FOR_CHILD);
  get_parameters_for_behavior((char *) "accept_vaccine_dose_for_child",
      Behavior_index::ACCEPT_VACCINE_DOSE_FOR_CHILD);
  Behavior::parameters_are_set = true;
}

void Behavior::get_parameters_for_behavior(char * behavior_name, int j) {
  Behavior_params * params = Behavior::behavior_params[j];
  strcpy(params->name, behavior_name);

  char param_str[FRED_STRING_SIZE];
  sprintf(param_str, "%s_enabled", behavior_name);
  Params::get_param(param_str, &(params->enabled));

  for(int i = 0; i < Behavior_change_model_enum::NUM_BEHAVIOR_CHANGE_MODELS; i++) {
    params->behavior_change_model_population[i] = 0;
  }

  sprintf(param_str, "%s_behavior_change_model_distribution", behavior_name);
  params->behavior_change_model_cdf_size = Params::get_param_vector(param_str,
      params->behavior_change_model_cdf);

  // convert to cdf
  double stotal = 0;
  for(int i = 0; i < params->behavior_change_model_cdf_size; i++) {
    stotal += params->behavior_change_model_cdf[i];
  }
  if(stotal != 100.0 && stotal != 1.0) {
    Utils::fred_abort("Bad distribution %s params_str\nMust sum to 1.0 or 100.0\n", param_str);
  }
  double cumm = 0.0;
  for(int i = 0; i < params->behavior_change_model_cdf_size; i++) {
    params->behavior_change_model_cdf[i] /= stotal;
    params->behavior_change_model_cdf[i] += cumm;
    cumm = params->behavior_change_model_cdf[i];
  }

  printf("BEHAVIOR %s behavior_change_model_cdf: ", params->name);
  for(int i = 0; i < Behavior_change_model_enum::NUM_BEHAVIOR_CHANGE_MODELS; i++) {
    printf("%f ", params->behavior_change_model_cdf[i]);
  }
  printf("\n");
  fflush (stdout);

  sprintf(param_str, "%s_frequency", behavior_name);
  Params::get_param(param_str, &(params->frequency));

  // FLIP behavior parameters

  sprintf(param_str, "%s_min_prob", behavior_name);
  Params::get_param(param_str, &(params->min_prob));

  sprintf(param_str, "%s_max_prob", behavior_name);
  Params::get_param(param_str, &(params->max_prob));

  // override max and min probs if prob is set
  double prob;
  sprintf(param_str, "%s_prob", behavior_name);
  Params::get_param(param_str, &prob);
  if(0.0 <= prob) {
    params->min_prob = prob;
    params->max_prob = prob;
  }

  // IMITATE PREVALENCE behavior parameters

  sprintf(param_str, "%s_imitate_prevalence_weights", behavior_name);
  Params::get_param_vector(param_str, params->imitate_prevalence_weight);

  params->imitate_prevalence_total_weight = 0.0;
  for(int i = 0; i < NUM_WEIGHTS; i++) {
    params->imitate_prevalence_total_weight += params->imitate_prevalence_weight[i];
  }
  
  sprintf(param_str, "%s_imitate_prevalence_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_prevalence_update_rate));

  // IMITATE CONSENSUS behavior parameters

  sprintf(param_str, "%s_imitate_consensus_weights", behavior_name);
  Params::get_param_vector(param_str, params->imitate_consensus_weight);

  params->imitate_consensus_total_weight = 0.0;
  for(int i = 0; i < NUM_WEIGHTS; i++)
    params->imitate_consensus_total_weight += params->imitate_consensus_weight[i];

  sprintf(param_str, "%s_imitate_consensus_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_consensus_update_rate));

  sprintf(param_str, "%s_imitate_consensus_threshold", behavior_name);
  Params::get_param(param_str, &(params->imitate_consensus_threshold));

  // IMITATE COUNT behavior parameters

  sprintf(param_str, "%s_imitate_count_weights", behavior_name);
  Params::get_param_vector(param_str, params->imitate_count_weight);

  params->imitate_count_total_weight = 0.0;
  for(int i = 0; i < NUM_WEIGHTS; i++) {
    params->imitate_count_total_weight += params->imitate_count_weight[i];
  }
  
  sprintf(param_str, "%s_imitate_count_update_rate", behavior_name);
  Params::get_param(param_str, &(params->imitate_count_update_rate));

  sprintf(param_str, "%s_imitate_count_threshold", behavior_name);
  Params::get_param(param_str, &(params->imitate_count_threshold));

  params->imitation_enabled = 0;

  // HBM parameters

  sprintf(param_str, "%s_susceptibility_threshold", behavior_name);
  int n = Params::get_param_vector(param_str, params->susceptibility_threshold_distr);
  if(n != 2) {
    Utils::fred_abort("bad %s\n", param_str);
  }

  sprintf(param_str, "%s_severity_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->severity_threshold_distr);
  if(n != 2) {
    Utils::fred_abort("bad %s\n", param_str);
  }

  sprintf(param_str, "%s_benefits_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->benefits_threshold_distr);
  if(n != 2) {
    Utils::fred_abort("bad %s\n", param_str);
  }

  sprintf(param_str, "%s_barriers_threshold", behavior_name);
  n = Params::get_param_vector(param_str, params->barriers_threshold_distr);
  if(n != 2) {
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

void Behavior::print_params(int n) {
  Behavior_params * params = Behavior::behavior_params[n];
  printf("PRINT BEHAVIOR PARAMS:\n");
  printf("name = %s\n", params->name);
  printf("enabled = %d\n", params->enabled);
  printf("frequency = %d\n", params->frequency);
  printf("behavior_change_model_population = ");
  for(int i = 0; i < Behavior_change_model_enum::NUM_BEHAVIOR_CHANGE_MODELS; i++) {
    printf("%d ", params->behavior_change_model_population[i]);
  }
  printf("\n");
  fflush (stdout);
}

void Behavior::update(Person * self, int day) {

  FRED_VERBOSE(1, "behavior::update person %d day %d\n", self->get_id(), day);

  if(day == -1 && self->get_id() == 0) {
    for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
      print_params(i);
    }
  }

  if(!Global::Enable_Behaviors) {
    return;
  }
  
  if(this->health_decision_maker != NULL) {
    return;
  }

  for(int i = 0; i < Behavior_index::NUM_BEHAVIORS; i++) {
    Behavior_params * params = Behavior::behavior_params[i];
    if(params->enabled) {
      FRED_VERBOSE(1, "behavior::update -- update intention[%d]\n", i);
      assert(this->intention[i] != NULL);
      this->intention[i]->update(day);
    }
  }
  FRED_VERBOSE(1, "behavior::update complete person %d day %d\n", self->get_id(), day);
}

bool Behavior::adult_is_staying_home() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::STAY_HOME_WHEN_SICK;
  Intention *my_intention = intention[n];
  if(my_intention == NULL) {
    return false;
  }
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  return my_intention->agree();
}

bool Behavior::adult_is_taking_sick_leave() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::TAKE_SICK_LEAVE;
  Intention *my_intention = this->intention[n];
  if(my_intention == NULL) {
    return false;
  }
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  return my_intention->agree();
}

bool Behavior::child_is_staying_home() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::KEEP_CHILD_HOME_WHEN_SICK;
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  if(this->health_decision_maker != NULL) {
    return this->health_decision_maker->child_is_staying_home();
  }
  // I am the health decision maker
  Intention *my_intention = this->intention[n];
  assert(my_intention != NULL);
  return my_intention->agree();
}

bool Behavior::acceptance_of_vaccine() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::ACCEPT_VACCINE;
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  if(this->health_decision_maker != NULL) {
    // printf("Asking health_decision_maker %d \n", health_decision_maker->get_id());
    return this->health_decision_maker->acceptance_of_vaccine();
  }
  // I am the health decision maker
  Intention *my_intention = this->intention[n];
  assert(my_intention != NULL);
  // printf("My answer is:\n");
  return my_intention->agree();
}

bool Behavior::acceptance_of_another_vaccine_dose() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::ACCEPT_VACCINE_DOSE;
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  if(this->health_decision_maker != NULL) {
    return this->health_decision_maker->acceptance_of_another_vaccine_dose();
  }
  // I am the health decision maker
  Intention *my_intention = this->intention[n];
  assert(my_intention != NULL);
  return my_intention->agree();
}

bool Behavior::child_acceptance_of_vaccine() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::ACCEPT_VACCINE_FOR_CHILD;
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  if(this->health_decision_maker != NULL) {
    printf("Asking health_decision_maker %d \n", this->health_decision_maker->get_id());
    return this->health_decision_maker->child_acceptance_of_vaccine();
  }
  // I am the health decision maker
  Intention *my_intention = this->intention[n];
  assert(my_intention != NULL);
  printf("My answer is:\n");
  return my_intention->agree();
}

bool Behavior::child_acceptance_of_another_vaccine_dose() {
  assert(Global::Enable_Behaviors == true);
  int n = Behavior_index::ACCEPT_VACCINE_DOSE_FOR_CHILD;
  Behavior_params * params = Behavior::behavior_params[n];
  if(params->enabled == false) {
    return false;
  }
  if(this->health_decision_maker != NULL) {
    return this->health_decision_maker->child_acceptance_of_another_vaccine_dose();
  }
  // I am the health decision maker
  Intention *my_intention = this->intention[n];
  assert(my_intention != NULL);
  return my_intention->agree();
}

Person * Behavior::select_adult(Household *h, int relationship, Person * self) {

  int N = h->get_size();

  if(relationship == Global::GRANDCHILD) {

    // select first adult natural child or spouse thereof of householder parent, if any
    for(int i = 0; i < N; i++) {
      Person * person = h->get_housemate(i);
      if(person->is_adult() == false || person == self) {
        continue;
      }
      int r = person->get_relationship();
      if(r == Global::SPOUSE || r == Global::CHILD || r == Global::SIBLING || r == Global::IN_LAW) {
        return person;
      }
    }

    // consider adult relative of householder, if any
    for(int i = 0; i < N; i++) {
      Person * person = h->get_housemate(i);
      if(person->is_adult() == false || person == self) {
        continue;
      }
      int r = person->get_relationship();
      if(r == Global::PARENT || r == Global::OTHER_RELATIVE) {
        return person;
      }
    }
  }

  // select householder if an adult
  for(int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if(person->is_adult() == false || person == self) {
      continue;
    }
    if(person->get_relationship() == Global::HOUSEHOLDER) {
      return person;
    }
  }

  // select householder's spouse if an adult
  for(int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if(person->is_adult() == false || person == self) {
      continue;
    }
    if(person->get_relationship() == Global::SPOUSE) {
      return person;
    }
  }

  // select oldest available person
  int max_age = self->get_age();

  Person * oldest = NULL;
  for(int i = 0; i < N; i++) {
    Person * person = h->get_housemate(i);
    if(person->get_age() <= max_age || person == self) {
      continue;
    }
    oldest = person;
    max_age = oldest->get_age();
  }
  return oldest;
}

void Behavior::terminate(Person * self) {
  if(!Global::Enable_Behaviors) {
    return;
  }
  if(this->health_decision_maker != NULL) {
    return;
  }
  if(Global::Verbose > 1) {
    printf("terminating behavior for agent %d age %d\n", self->get_id(), self->get_age());
  }

  // see if this person is the adult decision maker for any child in the household
  Household * household = (Household *) self->get_household();
  int size = household->get_size();
  for(int i = 0; i < size; i++) {
    Person * child = household->get_housemate(i);
    if(child != self && child->get_health_decision_maker() == self) {
      if(Global::Verbose > 1) {
        printf("need new decision maker for agent %d age %d\n", child->get_id(), child->get_age());
      }
      child->setup_behavior();
      if(Global::Verbose > 1) {
        printf("new decision maker is %d age %d\n", child->get_health_decision_maker()->get_id(),
            child->get_health_decision_maker()->get_age());
        fflush (stdout);
      }
    }
  }
}

void Behavior::become_health_decision_maker(Person * self) {
  if(this->health_decision_maker != NULL) {
    this->health_decision_maker = NULL;
    delete_intentions();
    setup_intentions(self);
  }
}

