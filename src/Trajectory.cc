#include "Trajectory.h"
#include <vector>
#include <iostream>
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include <string>
#include <sstream>

using namespace std;

Trajectory::Trajectory() {
  duration = 0;
}

Trajectory::Trajectory(map< int, trajectory_t > infectivity_copy, trajectory_t symptomaticity_copy) {
  duration = 0;
  infectivity = infectivity_copy;
  symptomaticity = symptomaticity_copy;

  for (map< int, trajectory_t >::iterator strain_iterator = infectivity.begin(); strain_iterator != infectivity.end(); ++strain_iterator) {
    if ( (int) strain_iterator->second.size() > duration) {
      duration = strain_iterator->second.size();
    }
  }

  if ( (int) symptomaticity.size() > duration) {
    duration = symptomaticity.size();
  }
}

Trajectory * Trajectory::clone() {
  Trajectory * cloned_trajectory = new Trajectory(infectivity, symptomaticity);
  return cloned_trajectory;
}

bool Trajectory::contains(int strain) {
  return ( infectivity.find(strain) != infectivity.end() );
}

void Trajectory::set_infectivities(map<int, vector<double> > inf) {
  infectivity = inf;
}

trajectory_t Trajectory::get_infectivity_trajectory(int strain) {
  return infectivity[strain];
}

trajectory_t Trajectory::get_symptomaticity_trajectory() {
  return symptomaticity;
}

void Trajectory::set_infectivity_trajectory(int strain, trajectory_t inf) {
  map< int, trajectory_t >::iterator it = infectivity.find(strain);

  if (it != infectivity.end()) {
    infectivity.erase(it);
  }

  infectivity[strain] = inf;

  if (duration < (int) infectivity[strain].size()) {
    duration = infectivity[strain].size();
  }
}

void Trajectory::set_symptomaticity_trajectory(trajectory_t symt) {
  symptomaticity = symt;

  if (duration < (int) symptomaticity.size()) {
    duration = (int) symptomaticity.size();
  }
}

Trajectory::point Trajectory::get_data_point(int t) {
  double point_infectivity = 0.0;
  double point_symptomaticity = 0.0;

  if (duration > t) {
    map< int, trajectory_t >::iterator strain_iterator;

    for (strain_iterator = infectivity.begin(); strain_iterator != infectivity.end(); ++strain_iterator) {
      if ( (int) strain_iterator->second.size() > t) {
        point_infectivity += strain_iterator->second[t];
      }
    }

    if ( (int) symptomaticity.size() > t) {
      point_symptomaticity += symptomaticity[t];
    }
  }

  return Trajectory::point(point_infectivity, point_symptomaticity);
}

void Trajectory::calculate_aggregate_infectivity() {
  // not currently used anywhere, could be used in the trajectory iterator
  aggregate_infectivity.assign(duration,0.0);
  map< int, trajectory_t >::iterator strain_iterator;
  trajectory_t::iterator strain_infectivity_iterator;

  for (strain_iterator = infectivity.begin(); strain_iterator != infectivity.end(); ++strain_iterator) {
    int t = 0;

    for (strain_infectivity_iterator = strain_iterator->second.begin(); strain_infectivity_iterator != strain_iterator->second.end(); ++strain_infectivity_iterator) {
      cout << t << "  " << *strain_infectivity_iterator << "  " << duration << endl;
      aggregate_infectivity[t] += *strain_infectivity_iterator;
      t++;
    }
  }
}

string Trajectory::to_string() {
  ostringstream os;
  os << "Infection Trajectories:";
  map< int, trajectory_t >::iterator map;
  trajectory_t::iterator vec;

  for (map = infectivity.begin(); map != infectivity.end(); ++map) {
    os << endl << " Strain " << map->first << ":";

    for (vec = map->second.begin(); vec != map->second.end(); ++vec) {
      os << " " << *vec;
    }
  }

  /*
  os << endl << "Symptomaticity Trajectories:" << endl;

  for (vec = symptomaticity.begin(); vec != symptomaticity.end(); ++vec) {
    os << " " << *vec;
  }
*/
  return os.str();
}

void Trajectory::print() {
  cout << to_string() << endl;
}

void Trajectory::print_alternate(stringstream &out) {
  //out << "Strains: ";
  //print();
  if(infectivity.size() == 1) { 
    map<int, trajectory_t>::iterator it = infectivity.begin();
    out << it->first << " " << it->second.size();
  }
  else {
    map<int, trajectory_t>::iterator it1 = infectivity.begin();
    map<int, trajectory_t>::iterator it2 = infectivity.end();
    int mut_day = 0;
    for(unsigned int i=0; i<it2->second.size(); i++) {
      if(it2->second.at(i) != 0) break;
      mut_day++;
    }
    if(mut_day != 0) {
      out << it1->first << " " << mut_day << " ";
    }
    out << it2->first << " " << it2->second.size()-mut_day+1;
  }
}

map<int, double> *Trajectory::get_current_loads(int day) {
  map<int, double> *infectivities = new map<int, double>;

  map< int, trajectory_t > :: iterator it;

  for(it = infectivity.begin(); it != infectivity.end(); it++) {
    pair<int, double> p = pair<int, double> (it->first, (it->second)[day]);
    infectivities->insert(p);
  }

  return infectivities;
}

map<int, double> *Trajectory::getInoculum(int day) {
  map<int, double> *infectivities = new map<int, double>;

  map< int, trajectory_t > :: iterator it;

  for(it = infectivity.begin(); it != infectivity.end(); it++) {
    pair<int, double> p = pair<int, double> (it->first, (it->second)[day]);
    infectivities->insert(p);
  }

  return infectivities;
}

void Trajectory::modify_symp_period(int startDate, int days_left) {
  // symptomaticty and infectivity trajectories become 0 after startDate + days_left
  int end_date = startDate + days_left;

  if(end_date > (int) symptomaticity.size()) return;

  symptomaticity.resize(end_date, 1);
  map< int, trajectory_t > :: iterator it;

  for(it = infectivity.begin(); it != infectivity.end(); it++) {
    (it->second).resize(end_date, 1);
  }
}

void Trajectory::modify_asymp_period(int startDate, int days_left, int sympDate) {
  int end_date = startDate + days_left;

  // if decreasing the asymp period
  if(end_date < sympDate) {
    for(int i = startDate, j = sympDate ; i < (int) symptomaticity.size(); i++, j++) {
      symptomaticity[i] = symptomaticity[j];
    }

    symptomaticity.resize(symptomaticity.size() - sympDate + days_left, 0);
    map< int, trajectory_t > :: iterator it;

    for(it = infectivity.begin(); it != infectivity.end(); it++) {
      trajectory_t &inf = it->second;

      for(int i = startDate, j = sympDate ; i < (int) inf.size(); i++, j++) {
        inf[i] = inf[j];
      }

      inf.resize(symptomaticity.size() - sympDate + days_left, 0);
    }
  }
  // if increasing the asymp period
  else {
    int days_extended = end_date - sympDate;
    trajectory_t :: iterator it = symptomaticity.begin();

    for(int i = 0; i < sympDate; i++) it++;

    symptomaticity.insert(it, days_extended, 0.0);

    map< int, trajectory_t > :: iterator inf_it;

    for(inf_it = infectivity.begin(); inf_it != infectivity.end(); inf_it++) {
      // infectivity value for new period = infectivity just before becoming symptomatic
      //  == asymp_infectivity for FixedIntraHost
      trajectory_t &inf = inf_it->second;
      it = inf.begin();

      for(int i = 0; i < sympDate; i++) it++;

      inf.insert(it, days_extended, inf[sympDate-1]);
    }
  }
}

void Trajectory :: modify_develops_symp(int sympDate, int sympPeriod) {
  int end_date = sympDate + sympPeriod;

  if(end_date < (int) symptomaticity.size()) {
    symptomaticity.resize(end_date);
  }
  else {
    symptomaticity.resize(end_date, 1);
    map< int, trajectory_t > :: iterator inf_it;

    for(inf_it = infectivity.begin(); inf_it != infectivity.end(); inf_it++) {
      trajectory_t &inf = inf_it->second;
      trajectory_t :: iterator it = inf.end();
      it--;
      inf.insert(it, end_date - symptomaticity.size(), inf[end_date-1]);
    }
  }
}

void Trajectory :: get_all_strains(vector<int>& strains) {
  strains.clear();
  map< int, trajectory_t > :: iterator inf_it;

  for(inf_it = infectivity.begin(); inf_it != infectivity.end(); inf_it++) {
    strains.push_back(inf_it->first);
  }
}

void Trajectory :: mutate(int old_strain, int new_strain, unsigned int day) {
  if(infectivity.find(new_strain) != infectivity.end()) return; // HACK
  if(infectivity.find(old_strain) == infectivity.end()){
    cout << "Strain Not Found: " << old_strain << endl;
    print();
  }
  assert(infectivity.find(old_strain) != infectivity.end());

  vector<double> &old_inf = infectivity[old_strain];
  vector<double> new_inf;
  if(day > old_inf.size()) return;
  for(unsigned int d=0; d < day; ++d) new_inf.push_back(0);
  for(unsigned int d=day; d < old_inf.size(); ++d){
    new_inf.push_back(old_inf[d]);
    old_inf[d] = 0;
  }
  infectivity.insert( pair<int, vector<double> > (new_strain, new_inf) );
  //infectivity.insert( pair<int, vector<double> > (old_strain, old_inf) );
  
  //cout << "Mutated from " << old_strain << " to " << new_strain << " on day " << day << endl;
}

