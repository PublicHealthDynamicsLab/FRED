/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_HIV_IntraHost_H
#define _FRED_HIV_IntraHost_H

#include <vector>
#include <map>

#include "IntraHost.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Transmission.h"

class Infection;
class Trajectory;

class HIV_IntraHost : public IntraHost {
  public:
    HIV_IntraHost();
    ~HIV_IntraHost();

    void setup(Disease *disease);
    Trajectory * get_trajectory( Infection *infection, Transmission::Loads * loads ) {
      return NULL;
    }
    int get_days_latent() { return 0; }
    int get_days_asymp() { return 0; }
    int get_days_symp() { return 0; }
    int get_days_susceptible() { return 0; }
    int get_symptoms() { return 0; }
    double get_asymp_infectivity() {
      return 0.0;
    }
    double get_symp_infectivity() {
      return 0.0;
    }
    double get_prob_symptomatic() {
      return 0.0;
    }
    int get_infection_model() {
      return 0.0;
    }

  private:
  };

#endif
