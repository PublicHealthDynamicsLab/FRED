#ifndef _FRED_DEFAULT_IntraHost_H
#define _FRED_DEFAULT_IntraHost_H

#include <vector>
#include <map>

#include "IntraHost.h"
#include "Infection.h"
#include "Trajectory.h"

class Infection;
class Trajectory;

class DefaultIntraHost : public IntraHost {
    // TODO Move reqd stuff from disease to here
  public:
    DefaultIntraHost();
    ~DefaultIntraHost();

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
     * @return this intrahost model's days latent
     */
    int get_days_latent();

    /**
     * @return this intrahost model's days incubating
     */
    int get_days_incubating();

    /**
     * @return this intrahost model's days asymptomatic
     */
    int get_days_asymp();

    /**
     * @return this intrahost model's days symptomatic
     */
    int get_days_symp();

    /**
     * @return this intrahost model's days susceptible
     */
    int get_days_susceptible();

    /**
     * @return the symptoms
     */
    int get_symptoms();

    /**
     * @return the infectivity if asymptomatic
     */
    double get_asymp_infectivity() {
      return asymp_infectivity;
    }

    /**
     * @return the infectivity if symptomatic
     */
    double get_symp_infectivity() {
      return symp_infectivity;
    }

    /**
     * @return the max_days
     */
    int get_max_days() {
      return max_days;
    }

    /**
     * @return the prob_symptomatic
     */
    double get_prob_symptomatic() {
      return prob_symptomatic;
    }

    /**
     * @return the infection_model
     */
    int get_infection_model() {
      return infection_model;
    }

  private:
    double asymp_infectivity;
    double symp_infectivity;
    int infection_model;
    int max_days_latent;
    int max_days_incubating;
    int max_days_asymp;
    int max_days_symp;
    int max_days;
    double *days_latent;
    double *days_incubating;
    double *days_asymp;
    double *days_symp;
    double prob_symptomatic;
  };

#endif
