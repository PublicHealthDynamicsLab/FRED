
#include "ODEIntraHost.h"
#include "Infection.h"
#include "Trajectory.h"
#include "ODE.h"
#include "Random.h"
#include "Params.h"
#include <map>

using namespace std;

void ODEIntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);

  // TODO use disease
  get_param((char *) "viral_titer_scaling", & viral_titer_scaling);
  get_param((char *) "viral_titer_latent_threshold", & viral_titer_latent_threshold);
  get_param((char *) "interferon_scaling", & interferon_scaling);
  get_param((char *) "interferon_threshold", & interferon_threshold);

  max_days = MAX_LENGTH;
  }

double ODEIntraHost :: get_inoculum_particles (double infectivity) {
  double sigma = 0.005;
  double inoculum_particles_max = 0.5 ;
  double inoculum_particles_min = 0.005;
  double v_max = 100.00;

  double mu = max (inoculum_particles_min, ( ( infectivity * viral_titer_scaling/ v_max ) * inoculum_particles_max ) );
  double inoculum_particles = draw_normal(mu,sigma);
  return inoculum_particles;
  }

vector<double> ODEIntraHost::getInfectivities(double *viralTiter, int duration) {
  vector<double> infectivity;

  for(int day = 0; day < duration; day++) {
    if (viralTiter[day] <= viral_titer_latent_threshold) {
      break;
      }
    else {
      infectivity.push_back(viralTiter[day] / viral_titer_scaling);
      }
    }

  return infectivity;
  }

vector<double> ODEIntraHost::get_symptomaticity(double *interferon, int duration) {
  vector<double> symptomaticity;
  bool flag = false;

  for(int day = 0; day < duration; day++) {
    if (interferon[day] > interferon_threshold || ! flag) {
      symptomaticity.push_back(interferon[day] / interferon_scaling);
      flag = true;
      }
    else break;
    }

  return symptomaticity;
  }

Trajectory *ODEIntraHost :: get_trajectory(Infection *infection, map<int, double> *loads) {
  /*  if(! initialized){
      initialize();
      initialized = true;
    }
  */
  int numStrains = loads->size();
  ODE *ebm = new ODE(numStrains);

  // set indices to strains
  int indices[numStrains];

  map<int, double> :: iterator it = loads->begin();

  for(int s = 0; s < numStrains; s++, it++) {
    indices[s] = it->first;
    double inoculum_particles = get_inoculum_particles(it->second);
    ebm->set_V(inoculum_particles, s);
    }

  // TODO set reqd params


  // Use ODE
  ebm->setup();

  Trajectory *trajectory = new Trajectory;

  // Infectivity Trajectories
  for(int s = 0; s < numStrains; s++, it++) {
    double *vt = ebm->get_viral_titer_data(s);
    vector<double> it = getInfectivities(vt, ebm->get_duration());
    it.insert(it.begin(), 0.0); // TODO
    trajectory->set_infectivity_trajectory(indices[s], it);
    }

  // Symptomaticity Trajectory
  double *ft = ebm->get_interferon_data();
  vector<double> st = get_symptomaticity(ft, ebm->get_duration());
  trajectory->set_symptomaticity_trajectory(st);

  delete ebm;
  return trajectory;
  }

