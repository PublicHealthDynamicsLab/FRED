/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "FixedIntraHost.h"
#include "Condition.h"
#include "IntraHost.h"
#include "Params.h"
#include "Trajectory.h"
#include "Random.h"

using namespace std;

void FixedIntraHost::setup(Condition *condition) {
  IntraHost::setup(condition);

  char s[80];
  char condition_name[80];
  int numProfiles;
  strcpy(condition_name, condition->get_condition_name());

  sprintf(s, "%s_infectivity_profile_probabilities", condition_name);
  Params::get_param(s, &numProfiles);
  Params::get_param_vector(s, probabilities);

  for(int i = 0; i < numProfiles; i++) {
    vector<double> infProfile;
    sprintf(s, "%s_fixed_infectivity_profile[%d]", condition_name, i);
    Params::get_param_vector(s, infProfile);
    infLibrary.push_back(infProfile);

    vector<double> sympProfile;
    sprintf(s, "%s_fixed_symptomaticity_profile[%d]", condition_name, i);
    Params::get_param_vector(s, sympProfile);
    sympLibrary.push_back(sympProfile);
  }
}

Trajectory * FixedIntraHost::get_trajectory() {
  double r = Random::draw_random();
  int index = 0;
  vector<double> :: iterator it;

  for(it = probabilities.begin(); it != probabilities.end(); it++, index++) {
    if (r <= *it) break;
  }

  Loads* loads;
  loads = new Loads;
  loads->clear();
  loads->insert( pair<int, double> (1, 1.0) );

  Trajectory * trajectory = new Trajectory();

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
  int idx = Random::draw_random_int(0, num);
  int days = 0;
  vector<double>& traj = sympLibrary[idx];

  for(int i=0; i < (int) traj.size(); i++) {
    if(traj[i] != 0) days+= 1;
  }

  return days;
}


