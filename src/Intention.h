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
// File: Intention.h
//

#ifndef _FRED_INTENTION_H
#define _FRED_INTENTION_H

#include "Global.h"
#include "Random.h"
#include "Behavior.h"
class Perceptions;
class Person;

class Intention {
public:
  /**
   * Default constructor
   */
  Intention(Person * _self, int _index);

  ~Intention() {}

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  void update(int day);

  // access functions
  void set_behavior_change_model(int bcm) {
    this->behavior_change_model = bcm;
  }

  void set_probability(double prob) {
    this->probability = prob;
  }

  void set_frequency(int freq) {
    this->frequency = freq;
  }

  void set_intention(bool decision) {
    this->intention = decision;
  }

  int get_behavior_change_model() {
    return this->behavior_change_model;
  }

  double get_probability () {
    return this->probability;
  }

  double get_frequency () {
    return this->frequency;
  }

  bool agree() {
    return this->intention;
  }

private:
  Person * self;
  Behavior_params * params;
  int index;
  int behavior_change_model;
  double probability;
  int frequency;
  int expiration;
  bool intention;

  // Health Belief Model

  // where the agent computes perceived values
  Perceptions * perceptions;

  // Thresholds for dichotomous variables
  double susceptibility_threshold;
  double severity_threshold;
  double benefits_threshold;
  double barriers_threshold;

  // HBM methods
  void setup_hbm();
  double update_hbm(int day);
};

#endif // _FRED_INTENTION_H

