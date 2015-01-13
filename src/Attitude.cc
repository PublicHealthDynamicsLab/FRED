/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Attitude.cc
//

#include "Attitude.h"
#include "Global.h"
#include "Place_List.h"
#include "Place.h"
#include "Person.h"
#include "Utils.h"

Attitude::Attitude(Person * _person) {
  self = _person;
  strategy = REFUSE;
  probability = 0.0;
  frequency = 0;
  expiration = 0;
  willing = false;
}

void Attitude::update(int day) {

  // reset the survey if needed (once per day)
  if (params->imitation_enabled && survey->last_update < day) {
    reset_survey(day);
  }

  if (frequency > 0 && expiration <= day) {
    if (params->imitation_enabled && day > 0)
      imitate();
    if (strategy == HBM && day > 0)
      update_hbm(day);
    double r = RANDOM();
    willing = (r < probability);
    expiration = day + frequency;
  }

  // respond to survey if any agent is using imitation
  if (params->imitation_enabled) {
    update_survey();
  }

  // printf("ATTITUDE day %d behavior %s person %d strategy %d willing %d\n",
  // day, params->name, self->get_id(), strategy, willing?1:0); 
}


////// CHANGE ATTITUDE BY IMITATION

void Attitude::reset_survey(int day) {
  int places = Global::Places.get_number_of_places();

  // allocate space for survey if needed
  if (survey->yes_responses == NULL) {
    printf("SURVEY for %s allocated on day %d for %d places\n",
     params->name,day,places); fflush(stdout);
    survey->yes_responses = new int [places];
    assert (survey->yes_responses != NULL);
    survey->total_responses = new int [places];
    assert (survey->total_responses != NULL);
    survey->previous_yes_responses = new int [places];
    assert (survey->previous_yes_responses != NULL);
    survey->previous_total_responses = new int [places];
    assert (survey->previous_total_responses != NULL);
    printf("SURVEY allocated on day %d for %d places\n", day,places); fflush(stdout);
  }

  // swap previous and current day's surveys
  int * tmp = survey->yes_responses;
  survey->yes_responses = survey->previous_yes_responses;
  survey->previous_yes_responses = tmp;
  survey->previous_yes = survey->yes;
  survey->previous_total = survey->total;
  
  if (day > 0) {
    printf("SURVEY of %s for day %d YES %d TOT %d PREVALENCE %f\n",
     params->name,day, survey->yes, survey->total,
     survey->total?(survey->yes/(double)survey->total):0.0);
  }

  // clear out today's survey
  for (int i = 0; i < places; i++) {
    // printf("SURVEY resetting for day %d index %d\n", day,i); fflush(stdout);
    survey->yes_responses[i] = 0;
    survey->total_responses[i] = 0;
  }
  survey->yes = 0;
  survey->total = 0;

  // mark this day's survey as reset
  survey->last_update = day;
  // printf("SURVEY RESET for day %d\n", day); fflush(stdout);
}

void Attitude::update_survey() {
  if (weight[HOUSEHOLD_WT] != 0.0) {
    record_survey_response(self->get_household());
  }
  if (weight[NEIGHBORHOOD_WT] != 0.0) {
    record_survey_response(self->get_neighborhood());
  }
  if (weight[SCHOOL_WT] != 0.0) {
    record_survey_response(self->get_school());
  }
  if (weight[WORK_WT] != 0.0) {
    record_survey_response(self->get_workplace());
  }
  if (weight[ALL_WT] != 0.0) {
    record_survey_response(NULL);
  }
}

void Attitude::record_survey_response(Place * place) {
  if (place != NULL) {
    int place_id = place->get_id();
    if (willing) {
      survey->yes_responses[place_id]++;
    }
    survey->total_responses[place_id]++;
  }
  else {
    if (willing) {
      survey->yes++;
    }
    survey->total++;
  }
}

void Attitude::imitate() {
  Place * place;
  int place_id;
  int * yes_responses = survey->previous_yes_responses;
  int * total_responses = survey->previous_total_responses;
  double weighted_sum = 0.0;
  
  // process household preferences
  if (weight[HOUSEHOLD_WT] != 0.0) {
    place = self->get_household();
    if (place != NULL) {
      int place_id = place->get_id();
      if (total_responses[place_id] > 0) {
  double contribution = weight[HOUSEHOLD_WT] * yes_responses[place_id];
  if (strategy != IMITATE_COUNT)
    contribution /= total_responses[place_id];
  weighted_sum += contribution;
      }
    }
  }

  // process neighborhood preferences
  if (weight[NEIGHBORHOOD_WT] != 0.0) {
    place = self->get_neighborhood();
    if (place != NULL) {
      place_id = place->get_id();
      if (total_responses[place_id] > 0) {
  double contribution = weight[NEIGHBORHOOD_WT] * yes_responses[place_id];
  if (strategy != IMITATE_COUNT)
    contribution /= total_responses[place_id];
  weighted_sum += contribution;
      }
    }
  }

  // process school preferences
  if (weight[SCHOOL_WT] != 0.0) {
    place = self->get_school();
    if (place != NULL) {
      place_id = place->get_id();
      if (total_responses[place_id] > 0) {
  double contribution = weight[SCHOOL_WT] * yes_responses[place_id];
  if (strategy != IMITATE_COUNT)
    contribution /= total_responses[place_id];
  weighted_sum += contribution;
      }
    }
  }

  // process workplace preferences
  if (weight[WORK_WT] != 0.0) {
    place = self->get_workplace();
    if (place != NULL) {
      place_id = place->get_id();
      if (total_responses[place_id] > 0) {
  double contribution = weight[WORK_WT] * yes_responses[place_id];
  if (strategy != IMITATE_COUNT)
    contribution /= total_responses[place_id];
  weighted_sum += contribution;
      }
    }
  }

  // process community preferences
  if (weight[ALL_WT] != 0.0) {
    if (strategy == IMITATE_COUNT)
      weighted_sum += weight[ALL_WT] * survey->previous_yes;
    else
      weighted_sum += weight[ALL_WT] *
  (double) survey->previous_yes / (double) survey->previous_total;
  }

  double prevalence = weighted_sum / total_weight;

  /*
  printf("imitate person %d yes %d  tot %d ws = %f  w: %f %f %f %f %f ",
   self->get_id(), survey->yes, survey->total, weighted_sum,
   weight[HOUSEHOLD_WT],weight[NEIGHBORHOOD_WT],weight[SCHOOL_WT],
   weight[WORK_WT],weight[ALL_WT]);

  printf("strategy %d update_rate %f prevalance %f prob before %f ",
   strategy, update_rate, prevalence, probability);

  */

  probability *= (1.0 - update_rate);
  if (strategy == IMITATE_PREVALENCE) {
    probability += update_rate * prevalence;
  }
  else {
    if (prevalence > threshold) {
      probability += update_rate;
    }
  }
  // printf(" prob after %f\n", probability);
}


////// HEALTH BELIEF MODEL

#include "Global.h"
#include "Perceptions.h"

void Attitude::update_hbm(int day) {
  /*
  // NOTE: HBM currently applies only to disease 0

  int disease_id = 0;

  // each update is specific to current behavior
  update_perceived_severity(day, disease_id);
  update_perceived_susceptibility(day, disease_id);
  update_perceived_benefits(day, disease_id);
  update_perceived_barriers(day, disease_id);

    // perceptions of current state of epidemic
    int current_cases = perceptions->get_global_cases(disease_id);
    int total_cases = Global::Pop.get_disease(disease_id)->get_epidemic()->get_total_incidents();
    double current_deaths = Global::Pop.get_disease(disease_id)->get_mortality_rate()*total_cases;
    double current_incidence = (double) current_cases / (double) Global::Pop.get_pop_size();
    total_deaths += current_deaths;
    
    // update memory
    if (day > 0) {
      cumm_susceptibility[disease_id] = day*cumm_susceptibility[disease_id] + current_incidence;
      cumm_susceptibility[disease_id] /= day;
    }
    else {
      cumm_susceptibility[disease_id] = (1.0 - memory_decay) * cumm_susceptibility[disease_id] +
  memory_decay * current_incidence;
    }
    cumm_severity[disease_id] = (1.0 - memory_decay) * cumm_severity[disease_id] + memory_decay * current_deaths;

    // update HBM constructs
    
    // perceived susceptibility
    if (susceptibility_threshold <= cumm_susceptibility[disease_id])
      perceived_susceptibility[disease_id] = 1;
    else
      perceived_susceptibility[disease_id] = 0;
    
    // perceived severity
    if (total_cases > 0 && severity_threshold <= total_deaths/total_cases)
      perceived_severity[disease_id] = 1;
    else
      perceived_severity[disease_id] = 0;
    odds_ratio[disease_id] = compare_to_odds_ratio(disease_id);

    perceived_benefits[disease_id] = 1.0;
    perceived_barriers[disease_id] = 0.0;
  }
  */
}


void Attitude::update_perceived_severity(int day, int disease_id) {
}

void Attitude::update_perceived_susceptibility(int day, int disease_id) {
}

void Attitude::update_perceived_benefits(int day, int disease_id) {
}

void Attitude::update_perceived_barriers(int day, int disease_id) {
}


void Attitude::setup_hbm() {

  perceptions = new Perceptions(self);

  perceived_susceptibility = new (nothrow) double [Global::Diseases];
  if (perceived_susceptibility == NULL) {
    Utils::fred_abort("Help! sus allocation failure\n");
  }
  
  perceived_severity = new (nothrow) double [Global::Diseases];
  if (perceived_severity == NULL) {
    Utils::fred_abort("Help! sev allocation failure\n"); 
  }
  
  perceived_benefits = new (nothrow) double [Global::Diseases];
  if (perceived_benefits == NULL) {
    Utils::fred_abort("Help! benefits allocation failure\n"); 
  }
  
  perceived_barriers = new (nothrow) double [Global::Diseases];
  if (perceived_barriers == NULL) {
    Utils::fred_abort("Help! barrier allocation failure\n"); 
  }

  cumm_susceptibility = new (nothrow) double [Global::Diseases];
  if (cumm_susceptibility == NULL) {
    Utils::fred_abort("Help! cumm_susceptibility allocation failure\n"); 
  }

  cumm_severity = new (nothrow) double [Global::Diseases];
  if (cumm_severity == NULL) {
    Utils::fred_abort("Help! cumm_severity allocation failure\n"); 
  }

  // individual differences:
  memory_decay = draw_normal(params->memory_decay_distr[0],params->memory_decay_distr[1]);
  if (memory_decay < 0.001) memory_decay = 0.0001;

  susceptibility_threshold = draw_normal(params->susceptibility_threshold_distr[0],params->susceptibility_threshold_distr[1]);
  severity_threshold = draw_normal(params->severity_threshold_distr[0],params->severity_threshold_distr[1]);
  benefits_threshold = draw_normal(params->benefits_threshold_distr[0],params->benefits_threshold_distr[1]);
  barriers_threshold = draw_normal(params->barriers_threshold_distr[0],params->barriers_threshold_distr[1]);

  for (int d = 0; d < Global::Diseases; d++) {
    cumm_susceptibility[d] = 0.0;
    cumm_severity[d] = 0.0;
  }
  total_deaths = 0;
}

bool Attitude::compare_to_odds_ratio(int disease_id) {
  double Odds;
  Odds = params->base_odds_ratio;
  if (perceived_susceptibility[disease_id] == 1)
    Odds *= params->susceptibility_odds_ratio;
  if (perceived_severity[disease_id] == 1)
    Odds *= params->severity_odds_ratio;
  if (perceived_benefits[disease_id] == 1)
    Odds *= params->benefits_odds_ratio;
  if (perceived_barriers[disease_id] == 1)
    Odds *= params->barriers_odds_ratio;
  return (Odds > 1.0);
}
