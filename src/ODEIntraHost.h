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
// File: ODEIntraHost.h
//

#ifndef _FRED_ODEIntraHost_H
#define _FRED_ODEIntraHost_H

#include <map>
#include <vector>

#include "IntraHost.h"

class Infection;
class Trajectory;

using namespace std;

class ODEIntraHost : public IntraHost {
    // TODO set params from params file
    // TODO set all initial values

  public:
    Trajectory *get_trajectory(Infection *infection, map<int, double> *loads);
    void setup(Disease *disease);
    int get_days_symp() {
      return 1;  // TODO
      }

  private:
    double get_inoculum_particles (double infector_particles);
    vector<double> getInfectivities(double *viralTiter, int duration);
    vector<double> get_symptomaticity(double *interferon, int duration);

    static const int MAX_LENGTH = 10;

    double viral_titer_scaling;
    double viral_titer_latent_threshold;
    double interferon_scaling;
    double interferon_threshold;
  };


#endif
