
#include <vector>
#include <map>

#include "FixedIntraHost.h"

#include "Disease.h"
#include "Params.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Random.h"

using namespace std;

void FixedIntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);

  char s[80];
  int numProfiles;
  int id = disease->get_id();

  sprintf(s, "infectivity_profile_probabilities[%d]", id);
  Params::get_param(s, &numProfiles);
  Params::get_param_vector(s, probabilities);

  max_days = 0;

  for(int i = 0; i < numProfiles; i++) {
    vector<double> infProfile;
    sprintf(s, "fixed_infectivity_profile[%d][%d]", id, i);
    Params::get_param_vector(s, infProfile);
    infLibrary.push_back(infProfile);

    vector<double> sympProfile;
    sprintf(s, "fixed_symptomaticity_profile[%d][%d]", id, i);
    Params::get_param_vector(s, sympProfile);
    sympLibrary.push_back(sympProfile);

    if( (int) infProfile.size() > max_days) max_days = infProfile.size();
  }
}

Trajectory * FixedIntraHost::get_trajectory(Infection *infection, map<int, double> *loads) {
  double r = RANDOM();
  int index = 0;
  vector<double> :: iterator it;

  for(it = probabilities.begin(); it != probabilities.end(); it++, index++) {
    if (r <= *it) break;
  }

  map<int, vector<double> > infectivities;
  map<int, double> :: iterator lit;

  // Weight infectivity values of each strain by their corresponding load
  for(lit = loads->begin(); lit != loads->end(); lit++) {
    int strain = lit->first;
    double load = lit->second;

    vector<double> infectivity;
    vector<double> :: iterator infIt;

    for(infIt = infLibrary[index].begin(); infIt != infLibrary[index].end(); infIt++) {
      infectivity.push_back( (*infIt) * load );
    }

    infectivities.insert( pair<int, vector<double> > (strain, infectivity) );
  }

  vector<double> symptomaticity = sympLibrary[index];

  return new Trajectory(infectivities, symptomaticity);
}

int FixedIntraHost::get_days_symp() {
  // Change implementation to take exact trajectory values...
  int num = sympLibrary.size();
  int idx = IRAND(0, num);
  int days = 0;
  vector<double>& traj = sympLibrary[idx];

  for(int i=0; i < (int) traj.size(); i++) {
    if(traj[i] != 0) days+= 1;
  }

  return days;
}


