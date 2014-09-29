/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_FIXED_IntraHost_H
#define _FRED_FIXED_IntraHost_H

#include <vector>
#include <map>

#include "IntraHost.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Transmission.h"

class Infection;
class Trajectory;

class FixedIntraHost : public IntraHost {
  public:

    /**
     * Get the infection Trajectory
     *
     * @param infection
     * @param loads
     * @return a pointer to a Trajectory object
     */
    Trajectory * get_trajectory( Infection *infection, Transmission::Loads * loads );

    /**
     * Set the attributes for the IntraHost
     *
     * @param dis the disease to which this IntraHost model is associated
     */
    void setup(Disease *disease);

    /**
     * @return this intrahost model's days symptomatic
     */
    virtual int get_days_symp();

  private:
    // library of trajectories and cdf over them
    std::vector< std::vector<double> > infLibrary;
    std::vector< std::vector<double> > sympLibrary;
    std::vector<double> probabilities;
};

#endif
