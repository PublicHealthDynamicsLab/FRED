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
// File: Intention.cc
//
#include "Intention.h"
#include "Global.h"
#include "Random.h"
#include "Utils.h"
#include "Person.h"
#include "Behavior.h"
#include "Perceptions.h"

Intention::Intention(Person * _self, int _index) {
  this->index = _index;
  this->self = _self;
  this->expiration = 0;
  this->params = Behavior::get_behavior_params(this->index);

  this->perceptions = NULL;

  // pick a behavior_change_model for this individual based on the population market shares
  this->behavior_change_model = draw_from_distribution(this->params->behavior_change_model_cdf_size,
      this->params->behavior_change_model_cdf);

  // set the other intention parameters based on the behavior_change_model
  switch(this->behavior_change_model) {

    case Behavior_change_model_enum::REFUSE:
      this->intention = false;
      this->probability = 0.0;
      this->frequency = 0;
      break;

    case Behavior_change_model_enum::ACCEPT:
      this->intention = true;
      this->probability = 1.0;
      this->frequency = 0;
      break;

    case Behavior_change_model_enum::FLIP:
      this->probability = URAND(this->params->min_prob, this->params->max_prob);
      this->intention = (RANDOM()<= this->probability);
      this->frequency = this->params->frequency;
      break;

    case Behavior_change_model_enum::HBM:
      this->probability = URAND(this->params->min_prob, this->params->max_prob);
      this->intention = (RANDOM() <= this->probability);
      this->frequency = this->params->frequency;
      setup_hbm();
      break;

    default: // REFUSE
      this->intention = false;
      this->probability = 0.0;
      this->frequency = 0;
      break;
    }

  FRED_VERBOSE(1,
      "created INTENTION %d name %d behavior_change_model %d freq %d expir %d probability %f\n",
      this->index, this->params->name, this->behavior_change_model, this->frequency,
      this->expiration, this->probability);
}

void Intention::update(int day) {
  FRED_VERBOSE(1,
      "update INTENTION %s day %d behavior_change_model %d freq %d expir %d probability %f\n",
      this->params->name, day, this->behavior_change_model, this->frequency,
      this->expiration, this->probability);

  if(this->frequency > 0 && this->expiration <= day) {
    if(this->behavior_change_model == Behavior_change_model_enum::HBM && day > 0) {
      this->probability = update_hbm(day);
    }
    double r = RANDOM();
    this->intention = (r < this->probability);
    this->expiration = day + this->frequency;
  }

  FRED_VERBOSE(1, "updated INTENTION %s = %d  expir = %d\n", this->params->name,
      (this->intention ? 1 : 0),
      this->expiration);

}

////// HEALTH BELIEF MODEL

double Intention::update_hbm(int day) {
  int disease_id = 0;

  FRED_VERBOSE(1, "update_hbm entered: thresholds: sus= %f sev= %f  ben= %f bar = %f\n",
      this->susceptibility_threshold, this->severity_threshold,
      this->benefits_threshold, this->barriers_threshold);

  // update perceptions.
  this->perceptions->update(day);

  // each update is specific to current behavior
  double perceived_severity = (this->perceptions->get_perceived_severity(disease_id) > this->severity_threshold);

  double perceived_susceptibility = (this->perceptions->get_perceived_susceptibility(disease_id)
      > this->susceptibility_threshold);

  double perceived_benefits = 1.0;
  double perceived_barriers = 0.0;

  // decide whether to act or not
  double odds;
  odds = this->params->base_odds_ratio;

  if(perceived_susceptibility == 1)
    odds *= this->params->susceptibility_odds_ratio;

  if(perceived_severity == 1)
    odds *= this->params->severity_odds_ratio;

  if(perceived_benefits == 1)
    odds *= this->params->benefits_odds_ratio;

  if(perceived_barriers == 1)
    odds *= this->params->barriers_odds_ratio;


  return (odds > 1.0);
}

void Intention::setup_hbm() {
  this->perceptions = new Perceptions(self);
  this->susceptibility_threshold = draw_uniform(this->params->susceptibility_threshold_distr[0],
      this->params->susceptibility_threshold_distr[1]);
  this->severity_threshold = draw_uniform(this->params->severity_threshold_distr[0],
      this->params->severity_threshold_distr[1]);
  this->benefits_threshold = draw_uniform(this->params->benefits_threshold_distr[0],
      this->params->benefits_threshold_distr[1]);
  this->barriers_threshold = draw_uniform(this->params->barriers_threshold_distr[0],
      this->params->barriers_threshold_distr[1]);
  FRED_VERBOSE(1, "setup_hbm: thresholds: sus= %f sev= %f  ben= %f bar = %f\n",
      this->susceptibility_threshold, this->severity_threshold,
      this->benefits_threshold, this->barriers_threshold);
}

