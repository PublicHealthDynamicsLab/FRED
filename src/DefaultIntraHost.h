/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_DEFAULT_IntraHost_H
#define _FRED_DEFAULT_IntraHost_H

#include <vector>
#include <map>

#include "Age_Map.h"
#include "Intrahost.h"

class Infection;
class Trajectory;

class DefaultIntraHost : public IntraHost {
  public:
    DefaultIntraHost();
    ~DefaultIntraHost();
    Trajectory * get_trajectory(int age);
    void setup(Disease *disease);
    int get_days_latent();
    int get_days_asymp();
    int get_days_symp();
    int get_days_susceptible();
    int will_have_symptoms(int age);
    double get_asymp_infectivity() {
      return asymp_infectivity;
    }

    double get_symp_infectivity() {
      return symp_infectivity;
    }

    double get_prob_symptomatic() {
      return prob_symptomatic;
    }

    double get_prob_symptomatic(int age) {
      if (age_specific_prob_symptomatic->is_empty()) {
	return prob_symptomatic;
      }
      else {
	return age_specific_prob_symptomatic->find_value(age);
      }
    }

    int get_infection_model() {
      return infection_model;
    }

  private:
    double asymp_infectivity;
    double symp_infectivity;
    int infection_model;
    int max_days_latent;
    int max_days_asymp;
    int max_days_symp;
    double *days_latent;
    double *days_asymp;
    double *days_symp;
    double prob_symptomatic;
    Age_Map *age_specific_prob_symptomatic;
  };

#endif
