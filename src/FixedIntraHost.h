#ifndef _FRED_FIXED_IntraHost_H
#define _FRED_FIXED_IntraHost_H

#include <vector>
#include <map>

#include "IntraHost.h"
#include "Infection.h"
#include "Trajectory.h"

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
    Trajectory * get_trajectory(Infection *infection, std :: map<int, double> *loads);

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
