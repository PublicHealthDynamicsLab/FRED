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
// File: Perceptions.h
//

#ifndef _FRED_PERCEPTIONS_H
#define _FRED_PERCEPTIONS_H

#include "Behavior.h"
class Epidemic;
class Person;

class Perceptions {
public:

  Perceptions(Person * self);
  void get_parameters();
  void update(int day);
  double get_perceived_severity(int disease_id) {
    return this->perceived_severity[disease_id];
  };

  double get_perceived_susceptibility(int disease_id) {
    return this->perceived_susceptibility[disease_id];
  };

  double get_perceived_benefits(int behavior_id, int disease_id) {
    return this->perceived_benefits[behavior_id][disease_id];
  };

  double get_perceived_barriers(int behavior_id, int disease_id) {
    return this->perceived_barriers[behavior_id][disease_id];
  };

  // Memory
  static double memory_decay_distr[2];
  static bool parameters_set;

private:
  Person * self;
  Epidemic * epidemic;

  // Perceptions
  double * perceived_susceptibility;		// per disease
  double * perceived_severity;			// per disease
  double * perceived_benefits[Behavior_index::NUM_BEHAVIORS];	// per behavior, per disease
  double * perceived_barriers[Behavior_index::NUM_BEHAVIORS];	// per behavior. per disease
  double memory_decay;

  // update methods
  void update_perceived_severity(int day);
  void update_perceived_susceptibility(int day);
  void update_perceived_benefits(int day);
  void update_perceived_barriers(int day);

protected:
  Perceptions() {}
  ~Perceptions() {}
};

#endif // _FRED_PERCEPTIONS_H

