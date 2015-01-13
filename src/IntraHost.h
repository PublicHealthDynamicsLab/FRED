#ifndef _FRED_IntraHost_H
#define _FRED_IntraHost_H

#include <vector>
#include <map>

class Infection;
class Trajectory;
class Disease;

class IntraHost {
  public:

    /**
     * This static factory method is used to get an instance of a specific IntraHost Model.
     * Depending on the type parameter, it will create a specific IntraHost Model and return
     * a pointer to it.
     *
     * @param the specific IntraHost model type
     * @return a pointer to a specific IntraHost model
     */
    static IntraHost * newIntraHost(int type);

    /**
     * Set the attributes for the IntraHost
     *
     * @param dis the disease to which this IntraHost model is associated
     */
    virtual void setup(Disease *dis);

    /**
     * An interface for a specific IntraHost model to implement to return its infection Trajectory
     *
     * @param infection
     * @param loads
     * @return a pointer to a Trajectory object
     */
    virtual Trajectory * get_trajectory(Infection *infection, std::map<int, double> *loads) = 0;

    /**
     * @return this IntraHost's max_days
     */
    int get_max_days() {
      return max_days;
    }

    /**
     * @return the days symptomatic
     */
    virtual int get_days_symp();
  protected:
    Disease *disease;
    int max_days;
};

#endif
