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
