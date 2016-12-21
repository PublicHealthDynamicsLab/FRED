/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_FIXED_IntraHost_H
#define _FRED_FIXED_IntraHost_H

#include <vector>
#include "IntraHost.h"

using namespace std;

class Condition;
class Trajectory;

class FixedIntraHost : public IntraHost {
public:

  /**
   * Get the infection Trajectory
   *
   * @return a pointer to a Trajectory object
   */
  Trajectory* get_trajectory( );

  /**
   * Set the attributes for the IntraHost
   *
   * @param dis the condition to which this IntraHost model is associated
   */
  void setup(Condition *condition);

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
