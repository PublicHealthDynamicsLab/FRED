#include <vector>
#include <map>

#include "DefaultIntraHost.h"
#include "Disease.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Params.h"
#include "Random.h"

DefaultIntraHost::DefaultIntraHost() {
  prob_symptomatic = -1.0;
  asymp_infectivity = -1.0;
  symp_infectivity = -1.0;
  max_days_latent = -1;
  max_days_incubating = -1;
  max_days_asymp = -1;
  max_days_symp = -1;
  max_days = 0;
  days_latent = NULL;
  days_incubating = NULL;
  days_asymp = NULL;
  days_symp = NULL;
}

DefaultIntraHost::~DefaultIntraHost() {
  delete [] days_latent;
  delete [] days_incubating;
  delete [] days_asymp;
  delete [] days_symp;
}

void DefaultIntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);

  int id = disease->get_id();
  Params::get_indexed_param("symp",id,&prob_symptomatic);
  Params::get_indexed_param("symp_infectivity",id,&symp_infectivity);
  Params::get_indexed_param("asymp_infectivity",id,&asymp_infectivity);

  int n;
  Params::get_indexed_param("days_latent",id,&n);
  days_latent = new double [n];
  max_days_latent = Params::get_indexed_param_vector("days_latent", id, days_latent) -1;

  Params::get_indexed_param("days_incubating",id,&n);
  days_incubating = new double [n];
  max_days_incubating = Params::get_indexed_param_vector("days_incubating",id, days_incubating) - 1;

  Params::get_indexed_param("days_asymp",id,&n);
  days_asymp = new double [n];
  max_days_asymp = Params::get_indexed_param_vector("days_asymp", id, days_asymp) -1;

  Params::get_indexed_param("days_symp",id,&n);
  days_symp = new double [n];
  max_days_symp = Params::get_indexed_param_vector("days_symp", id, days_symp) -1;

  Params::get_indexed_param("infection_model", id, &infection_model);

  if (max_days_asymp > max_days_symp) {
    max_days = max_days_latent + max_days_asymp;
  } else {
    max_days = max_days_latent + max_days_symp;
  }
}

Trajectory * DefaultIntraHost::get_trajectory(Infection *infection, std :: map<int, double> *loads) {
  // TODO  take loads into account - multiple strains
  Trajectory * trajectory = new Trajectory();
  int sequential = get_infection_model();

  int will_be_symptomatic = get_symptoms();

  int days_latent = get_days_latent();
  int days_incubating;
  int days_asymptomatic = 0;
  int days_symptomatic = 0;
  double symptomatic_infectivity = get_symp_infectivity();
  double asymptomatic_infectivity = get_asymp_infectivity();
  double symptomaticity = 1.0;

  if (sequential) { // SEiIR model
    days_asymptomatic = get_days_asymp();

    if (will_be_symptomatic) {
      days_symptomatic = get_days_symp();
    }
  } else { // SEIR/SEiR model
    if (will_be_symptomatic) {
      days_symptomatic = get_days_symp();
    } else {
      days_asymptomatic = get_days_asymp();
    }
  }

  days_incubating = days_latent + days_asymptomatic;

  map<int, double> :: iterator it;

  for(it = loads->begin(); it != loads->end(); it++) {
    vector<double> infectivity_trajectory(days_latent, 0.0);
    infectivity_trajectory.insert(infectivity_trajectory.end(), days_asymptomatic, asymptomatic_infectivity);
    infectivity_trajectory.insert(infectivity_trajectory.end(), days_symptomatic, symptomatic_infectivity);
    trajectory->set_infectivity_trajectory(it->first, infectivity_trajectory);
  }

  vector<double> symptomaticity_trajectory(days_incubating, 0.0);
  symptomaticity_trajectory.insert(symptomaticity_trajectory.end(), days_symptomatic, symptomaticity);
  trajectory->set_symptomaticity_trajectory(symptomaticity_trajectory);

  return trajectory;
}

int DefaultIntraHost::get_days_latent() {
  int days = 0;
  days = draw_from_distribution(max_days_latent, days_latent);
  return days;
}

int DefaultIntraHost::get_days_incubating() {
  int days = 0;
  days = draw_from_distribution(max_days_incubating, days_incubating);
  return days;
}

int DefaultIntraHost::get_days_asymp() {
  int days = 0;
  days = draw_from_distribution(max_days_asymp, days_asymp);
  return days;
}

int DefaultIntraHost::get_days_symp() {
  int days = 0;
  days = draw_from_distribution(max_days_symp, days_symp);
  return days;
}

int DefaultIntraHost::get_days_susceptible() {
  return 0;
}

int DefaultIntraHost::get_symptoms() {
  return (RANDOM() < prob_symptomatic);
}

