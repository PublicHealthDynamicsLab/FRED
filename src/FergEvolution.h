#ifndef _FRED_FERGEVOLUTION_H
#define _FRED_FERGEVOLUTION_H

#include <map>
#include <vector>
#include <string>
#include <fstream>


#include "Evolution.h"
#include "MSEvolution.h"
#include "Age_Map.h"
#include "Global.h"

class Infection;
class AV_Health;
class Antiviral;
class Infection;
class Health;
class Disease;
class Trajectory;
class Piecewise_Linear;

using namespace std;

class FergEvolution : public MSEvolution 
{
 public:
  virtual ~FergEvolution();
  void setup(Disease *disease);
  Infection *transmit(Infection *infection, Transmission *transmission, Person *infectee);
  void try_to_mutate(Infection *infection, Transmission *transmission);
  double get_prob_taking(Person *infectee, int new_strain, double quantity, int day);
  void add_failed_infection(Transmission *transmission, Person *infectee);
  double antigenic_distance(int strain1, int strain2);
  double genetic_distance(int strain1, int strain2);
  void print_stats(int day);
  int get_cluster(Infection *infection);
  int get_cluster(Person *pers);
  void antigenic_diversity(const vector<vector<int> > &num_cases, int day, 
      double &diversity, double &diversity_nt, double &variance, long int &tot_cases);
  map<int, double> * get_primary_loads(int day);
  int create_strain(int start_strain, int gen_dist, int day);
  void terminate_person(Person *p);

 private:
  int get_weekly_inc(int week, int cluster);
  bool reignite(Person *pers, int day);
  void squirrel_infectors(vector<Person *> &reign, int cluster);

  fred::Mutex mutex;

 protected:
  int aminoAcids[64];
  double delta;
  double eps_p;
  vector<vector<int> > num_cases; // per day per strain
  vector<vector<vector<int> > > num_cases_clusters; // per cluster per day per strain (prev)
  vector<vector<vector<int> > > new_cases_clusters; // per cluster per day per strain (inc)
  vector<pair<double, double> > cluster_centers; // means of the clusters
  int base_strain;
  int last_strain;
  vector<vector<int> > new_strains; // newly created strains per day
  int num_ntides;
  int ext_strain;
  vector<int> states, max_inc;
  vector< vector<Person *> >reignitors; // per cluster
  int num_clusters;
};

#endif

