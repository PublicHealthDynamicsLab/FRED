
#include "FergEvolution.h"
#include "MSEvolution.h"
#include "Random.h"
#include "Evolution.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Global.h"
#include "IntraHost.h"
#include "Antiviral.h"
#include "Health.h"
#include "Person.h"
#include "Piecewise_Linear.h"
#include "Past_Infection.h"
#include "Transmission.h"
#include "Strain.h"
#include "Params.h"
#include <map>
#include <cmath>
#include <fstream>
#include <cfloat>
#include "Utils.h"
#include "Population.h"
#include "Geo_Utils.h"
#include "Place.h"

using namespace std;

const int REIGNITE = 0, RISING = 1, EARLY_DECLINE = 2, DECLINE = 3;
const char stateName[][20] = {"REIGNITE", "RISING", "EARLY_DECLINE", "DECLINE"};
// get from params file
const int NUM_REIGNITORS = 3;
const int RISE_THRESH = 300, DECL_THRESH = 200;
const int INIT_THRESH = 80;
const int REIGN_THRESH = 30;
bool SQUIRRELED = false;

void FergEvolution :: setup(Disease *disease)
{
  MSEvolution::setup(disease);
  char numAA_file_name[MAX_PARAM_SIZE];
  strcpy(numAA_file_name, "$FRED_HOME/input_files/evolution/numAA.txt");
  ifstream aafile;
  Utils::get_fred_file_name( numAA_file_name );
  aafile.open( numAA_file_name );
  if ( !( aafile.good() ) ) {
    Utils::fred_abort( "The file $FRED_HOME/input_files/evolution/numAA.txt (required by FergEvolution::setup) is non-existent, corrupted, empty, or otherwise not 'good()'... " );
  }
  while(!aafile.eof()){
    int n;
    aafile >> n;
    aafile >> aminoAcids[n];
  }
  aafile.close();
  Params::get_param((char *) "mutation_rate", &delta);
  Params::get_param((char *) "eps_p", &eps_p);
  Params::get_param((char *) "num_amino_acids", &num_ntides);
  num_ntides *= 3;
  //for(unsigned int i=0; i<64; i++) cout << "AA: " << i << " " << aminoAcids[i] << endl;
  base_strain = 0;
  last_strain = base_strain;

  vector <double> centers;
  Params::get_param_vector((char *) "cluster_centers", centers);
  assert(centers.size() % 2 == 0);
  for(unsigned int i=0; i<centers.size(); i+=2) {
    cluster_centers.push_back( pair<double, double> (centers.at(i), centers.at(i+1)) );
  }
  
  num_clusters = cluster_centers.size();
  num_cases_clusters.resize(num_clusters);
  new_cases_clusters.resize(num_clusters);
  ext_strain = -1;
  
  char fname[512];
  
  if ( Global::Report_Transmissions ) {
    cout << "Reporting transmissions" << endl;
  }

  //sprintf(fname, "%s/strains.txt", Global::Output_directory);
  //strains_file.open(fname);

  if ( Global::Report_Strains ) {
    cout << "Reporting Strains" << endl;
  }


  // print initial strains
  if ( Global::Report_Strains ) {
    for(int i=0; i<disease->get_num_strains(); i++) {
      fprintf( Global::Strainsfp, "-1 -1 -1 -1: " );
      std::stringstream strains_stream;
      disease->printStrain( i, strains_stream );
      fprintf( Global::Strainsfp, "%s\n", strains_stream.str().c_str() );
    }
  }

  // re-ignition
  states.resize(num_clusters);
  max_inc.resize(num_clusters);
  for(int i=0; i<num_clusters; i++) {
    states.push_back(REIGNITE);
    max_inc.push_back(0);
  }
  reignitors.resize(num_clusters);
}

FergEvolution :: ~FergEvolution()
{
  //strains_file.close();
  //infections_file.close();
}

double FergEvolution :: antigenic_distance(int strain1, int strain2)
{
  double distance = 0.0;
  int n = disease->get_num_strain_data_elements(strain1);
  for(int i=0; i<n; ++i){
    int seq1 = disease->get_strain_data_element(strain1, i);
    int seq2 = disease->get_strain_data_element(strain2, i);
    //distance += int(aminoAcids[seq1] != aminoAcids[seq2]);
    if(aminoAcids[seq1] != aminoAcids[seq2]) 
      distance += 1;
  }
  //if(strain1 != strain2 && strain1 >= 90) 
  //  cout << "COMPUTING ANTIGENIC DISTANCE: " << strain1 << " " << strain2 << " : " << distance << endl;
  return distance;
}

double FergEvolution :: genetic_distance(int strain1, int strain2) {
  double distance = 0.0;
  int n = disease->get_num_strain_data_elements(strain1);
  for(int i=0; i<n; ++i){
    int seq1 = disease->get_strain_data_element(strain1, i);
    int seq2 = disease->get_strain_data_element(strain2, i);
    if(seq1%4 != seq2%4) distance += 1;
    seq1 /= 4; seq2 /= 4;
    if(seq1%4 != seq2%4) distance += 1;
    seq1 /= 4; seq2 /= 4;
    if(seq1%4 != seq2%4) distance += 1;
  }
  distance /= (3*n);
  return distance;
}

int FergEvolution :: get_cluster(Person *pers) {
  return get_cluster(pers->get_health()->get_infection(disease->get_id()));
}

int FergEvolution :: get_cluster(Infection *infection) {
  Place *household = infection->get_host()->get_household();
  double lon = household->get_longitude();
  double lat = household->get_latitude();
  
  //cout << endl;
  int cluster = 0; double min_dist = DBL_MAX;
  for(unsigned int i=0; i<cluster_centers.size(); i++) {
    double clon = cluster_centers.at(i).first;
    double clat = cluster_centers.at(i).second;
    double dist = (lon-clon)*(lon-clon) + (lat-clat)*(lat-clat);
    if(dist < min_dist) min_dist = dist, cluster = i;
    //cout << endl << "\tCLUSTER: " << clat << " " << clon << " " << dist <<  " " << min_dist << endl;
  }
  //cout << endl << "CLUSTER: (" << lat << ", " << lon << ") " << cluster << endl;
  return cluster;
}
 
void FergEvolution :: try_to_mutate(Infection *infection, Transmission *transmission)
{
  fred::Scoped_Lock lock( mutex );

  int exp = infection->get_exposure_date();
  Trajectory *trajectory = infection->get_trajectory();
  int duration = trajectory->get_duration();
  vector<int> strains; infection->get_strains(strains);
  assert(strains.size() == 1);
  int c = get_cluster(infection);
  bool mutated = false;
  // day 0
  for(int d=0; d<duration; d++){
    if(num_cases.size() <= unsigned(exp+d)) num_cases.resize(exp+d+1);
    if(num_cases_clusters.at(c).size() <= unsigned(exp+d)) num_cases_clusters.at(c).resize(exp+d+1);
    if(d == 0) {
      if(new_cases_clusters.at(c).size() <= unsigned(exp+d)) new_cases_clusters.at(c).resize(exp+d+1);
    }
    for(unsigned int k=0; k<strains.size(); ++k){
      int s = strains[k];
      mutated = false;
      int n = disease->get_num_strain_data_elements(s);
      vector<int> newData;
      for(int i=0; i<n; ++i){
        int aa = disease->get_strain_data_element(s, i);
        int new_aa = aa;
        for(int j=0, x=1; j<3; ++j, x*=4){
          double r = RANDOM();
          if(r >= delta) continue;
          int old_ntide = (aa/x)%4; int new_ntide = IRAND(0,3);
          if(new_ntide == old_ntide) continue;
          new_aa += (new_ntide - old_ntide)*x;
          mutated = true;
        }
        newData.push_back(new_aa);
      }
      if(mutated){
        int id = disease->addStrain(newData, disease->get_transmissibility(s), s);
        if(id > last_strain) { // New strain created
          int pid = -1, date = -1;
          if(transmission->get_infected_place()) {
            pid = transmission->get_infected_place()->get_id();
            date = transmission->get_exposure_date();
          }
          int infectee_id = infection->get_host()->get_id();
          int infector_id = -1;
          if(transmission->get_infector()) infector_id = transmission->get_infector()->get_id();

          // Reporting
          if ( Global::Report_Strains ) {
            std::stringstream strains_stream;
            disease->printStrain(id, strains_stream);
            fprintf( Global::Strainsfp, "%d %d %d %d : ", pid, date, infectee_id, infector_id, strains_stream.str().c_str() );
          }

          last_strain = id;
        }
        if(new_strains.size() <= unsigned(exp+d)) new_strains.resize(exp+d+1);
        new_strains.at(exp+d).push_back(id);
        //}
        trajectory->mutate(s, id, d);
        trajectory->mutate(s, id, 0);
        if(num_cases.at(exp+d).size() <= unsigned(id)) num_cases.at(exp+d).resize(id+1);
        if(num_cases_clusters.at(c).at(exp+d).size() <= unsigned(id)) num_cases_clusters.at(c).at(exp+d).resize(id+1);
        num_cases.at(exp+d).at(id)++;
        num_cases_clusters.at(c).at(exp+d).at(id)++;
        break;
      }
      else{
        if(num_cases.at(exp+d).size() <= unsigned(s)) num_cases.at(exp+d).resize(s+1);
        if(num_cases_clusters.at(c).at(exp+d).size() <= unsigned(s)) num_cases_clusters.at(c).at(exp+d).resize(s+1);
        num_cases.at(exp+d).at(s)++;
        num_cases_clusters.at(c).at(exp+d).at(s)++;
      }
      if(d == 0){
        if(new_cases_clusters.at(c).at(exp+d).size() <= unsigned(s)) new_cases_clusters.at(c).at(exp+d).resize(s+1);
        new_cases_clusters.at(c).at(exp+d).at(s)++;
      }
    }
    if(mutated) break;
  }
  //if(mutated) infection->set_trajectory(trajectory); // ??
}

void FergEvolution :: add_failed_infection(Transmission *transmission, Person *infectee)
{
  map<int, double> *loads = transmission->get_initial_loads();
  vector<int> strains;
  map<int, double> :: iterator it;
  for(it=loads->begin(); it!=loads->end();it++){
    strains.push_back(it->first);
  }
  int day = transmission->get_exposure_date();
  int recovery_date = day - 6;
  int age_at_exposure = infectee->get_age();
  //Past_Infection *past_inf = new Past_Infection(strains, recovery_date, age_at_exposure, disease, infectee);
  //infectee->get_health()->past_infections->push_back(past_inf);
  infectee->add_past_infection( strains, recovery_date, age_at_exposure, disease, infectee );
  //cout << "FAILED TRANSMISSION: " << strains.size() << " " << strains[0] << endl;
}

double FergEvolution :: get_prob_taking(Person *infectee, int new_strain, double quantity, int day)
{
  double prob_taking = MSEvolution :: get_prob_taking(infectee, new_strain, quantity, day);
  return prob_taking;
}

Infection *FergEvolution :: transmit(Infection *infection, Transmission *transmission, Person *infectee)
{
  infection = MSEvolution::transmit(infection, transmission, infectee);
  
  if(infection == NULL){
    // immune boosting
    add_failed_infection(transmission, infectee);
    return infection;
  }

  try_to_mutate(infection, transmission);

  if ( Global::Report_Transmissions ) {
    int place_id = -1;
    if(transmission->get_infected_place()) 
      place_id = transmission->get_infected_place()->get_id();
    int infector_id = -1;
    if(transmission->get_infector()) 
      infector_id = transmission->get_infector()->get_id();
    //infections_file << infectee->get_id() << " " << infector_id << " " << transmission->get_exposure_date() << " " << place_id << " ";
    std::stringstream trj_stream;
    infection->get_trajectory()->print_alternate(trj_stream);
    fprintf( Global::Transmissionsfp, "%d %d %d %d %s\n", infectee->get_id(), infector_id, transmission->get_exposure_date(), place_id, trj_stream.str().c_str() );
    //infection->get_trajectory()->print();
  }

  return infection;
}

void FergEvolution :: antigenic_diversity(const vector<vector<int> > &num_cases, 
                  int day, double &diversity, double &diversity_nt, double &variance, long int &tot_cases) {
  tot_cases = 0; diversity = 0.0; variance = 0.0; diversity_nt = 0.0;
  if(day >= num_cases.size()) return;
  
  // precompute tot_cases to avoid overflows
  for(unsigned int s1=0; s1<num_cases.at(day).size(); s1++) {
    tot_cases += num_cases.at(day).at(s1);
  }
  
  if(tot_cases == 0) return;
  for(int s1=0; s1<num_cases.at(day).size(); s1++) {
    double cases1 = double(num_cases.at(day).at(s1)); 
    if(cases1 == 0) continue;
    for(int s2=0; s2<s1; s2++) {
      double cases2 = double(num_cases.at(day).at(s2));
      if(cases2 == 0) continue;
      double dist = antigenic_distance(s1, s2);
      double nt_dist = genetic_distance(s1, s2);
      diversity += (double(cases1)/tot_cases * double(cases2)/tot_cases * dist);
      diversity_nt += (double(cases1)/tot_cases * double(cases2)/tot_cases * nt_dist);
      variance += (double(cases1)/tot_cases * double(cases2)/tot_cases * dist * dist);
      assert(diversity >= 0);
      assert(diversity_nt >= 0);
    }
  }
  diversity *= 2.0; // * 64;
  diversity_nt *= 2.0; // * 64;
  variance *= 2.0;
  assert(diversity >= 0);
  assert(diversity_nt >= 0);
}
 
void FergEvolution :: print_stats(int day) {
  // Print Multi-strain statistics -- antigenic diversity, #substitutions, ...
  day += 1;
  // Prevalence of strains
/*  for(int s=0; s<num_cases.at(day).size(); s++) {
    int cases = num_cases.at(day).at(s);
    if(cases != 0) {
      fprintf(Global::Outfp, " P_%d %d", s, cases);
      if(Global::Verbose) {
        fprintf(Global::Statusfp, " P_%d %d", s, cases);
      }
    }
  }*/
  int disease_id = disease->get_id();

if(day % 7 == 0) {
  // Antigenic diversity
  double diversity = 0.0, diversity_nt = 0.0, variance = 0.0;
  long int tot_cases = 0;
  antigenic_diversity(num_cases, day, diversity, diversity_nt, variance, tot_cases);
  //fprintf(Global::Outfp, " div %.10lf nt_div %.10lf var %.10lf", diversity, diversity_nt, variance);
  fprintf(Global::Outfp, " div %.10lf nt_div %.10lf", diversity, diversity_nt, variance);
  if(Global::Verbose) {
    //fprintf(Global::Statusfp, " div %.10lf nt_div %.10lf var %.10lf", diversity, diversity_nt, variance);
    fprintf(Global::Statusfp, " div %.10lf nt_div %.10lf", diversity, diversity_nt, variance);
  }
  double avg_diversity = 0.0, avg_variance = 0.0;
  for(int i=0; i<cluster_centers.size(); i++) {
    continue;
    double clust_diversity = 0.0, clust_diversity_nt = 0.0, clust_variance = 0.0; long int cases = 0;
    antigenic_diversity(num_cases_clusters.at(i), day, clust_diversity, 
        clust_diversity_nt, clust_variance, cases);
    avg_diversity += double(cases)/tot_cases * clust_diversity;
    avg_variance += double(cases)/tot_cases * clust_variance;
  }
  //fprintf(Global::Outfp, " clust_var %.10lf", avg_variance);
  if(Global::Verbose) {
    //fprintf(Global::Statusfp, " clust_var %.10lf", avg_variance);
  }
  double expl_variance = 0.0;
  if(variance != 0) 
    //expl_variance = (variance - avg_variance) / variance;
    expl_variance = avg_variance / variance;
  //fprintf(Global::Outfp, " expl_var %.10lf", expl_variance);
  if(Global::Verbose) {
    //fprintf(Global::Statusfp, " expl_var %.10lf", expl_variance);
  }
 
  // Num Substitutions
  double substns = 0.0;
  int num_new_strains = 0; 
  if(day < new_strains.size()){
    num_new_strains = new_strains.at(day).size();
    for(int i = 0; i < new_strains.at(day).size(); i++){
      //substns += genetic_distance(new_strains.at(day).at(i), base_strain);
      substns += disease->getStrainSubstitutions(new_strains.at(day).at(i));
    }
    if(num_new_strains != 0){
      substns /= num_new_strains;
    }
  }
  fprintf(Global::Outfp, " sub %lf sub_num %d ", substns, num_new_strains);
  if(Global::Verbose) {
    fprintf(Global::Statusfp, " sub %lf sub_num %d ", substns, num_new_strains);
  }

  for(int c=0; c<num_clusters; c++) {
    fprintf(Global::Outfp, " C_%d %d", c, get_weekly_inc(day/7 - 1, c));
    if(Global::Verbose) {
      fprintf(Global::Statusfp, " C_%d %d", c, get_weekly_inc(day/7 - 1, c));
    }
  }
}
  // Re-ignition -- state changes
  
  // initial squirreling
  if(SQUIRRELED == false) {
    if(disease->get_epidemic()->get_num_infectious() > INIT_THRESH) {
      for(int c=0; c<num_clusters; c++) {
        vector<Person *> &reign = reignitors.at(c);
        squirrel_infectors(reign, c);
        printf("squirreled %zu people", reignitors.at(c).size());
      }
    }
  }

  if(day >= 3*7 && day % 7 == 0) {
    for(int c=0; c<num_clusters; c++) {
      bool changed = false;
      int &state = states.at(c);
      // RISING
      int week = day/7;
      int inc = get_weekly_inc(week-1, c), inc1 = get_weekly_inc(week-2, c), inc2 = get_weekly_inc(week-3, c);
      if(state == REIGNITE && inc > RISE_THRESH && inc > inc1 && inc1 > inc2) {
        state = RISING;
        changed = true;
      }
      // EARLY_DECLINE
      else if(state == RISING && inc < inc1 && inc1 < inc2) {
        state = EARLY_DECLINE;
        max_inc[c] = inc2;
        changed = true;
      }
      // DECLINE
      else if(state == EARLY_DECLINE && inc < max_inc[c]/2) {
        state = DECLINE;
        // squirrel infectors
        vector<Person *> &reign = reignitors.at(c);
        squirrel_infectors(reign, c);
        changed = true;
      }
      // REIGNITE
      else if(state == DECLINE && inc < DECL_THRESH) {
        state = REIGNITE;
        changed = true;
      }
      if(changed) {
        printf("\nstate changed for cluster %d to: %s on %d\n", c, stateName[state], day);
        if(state == DECLINE) printf("squirreled %zu people", reignitors.at(c).size());
      }
    }
  }
  // Actual Re-ignition
  int reign_count = 0;
  for(int c=0; c<num_clusters; c++) {
    //if(states.at(c) != REIGNITE || reignitors.at(c).size() == 0) continue;
    if(reignitors.at(c).size() == 0 || disease->get_epidemic()->get_num_infectious() > REIGN_THRESH) continue;
    int num = (NUM_REIGNITORS > reignitors.at(c).size()) ? reignitors.at(c).size() : NUM_REIGNITORS;
    for(int i=0; i<num; i++) {
      int index = IRAND(0, reignitors.at(c).size()-1);
      Person *reignitor = reignitors.at(c).at(index);
      if(! reignitor) // dead
        continue;
      reign_count += (int) reignite(reignitor, day);
    }
  }

  fprintf(Global::Outfp, " reign %d", reign_count);
  if(Global::Verbose) {
    fprintf(Global::Statusfp, " reign %d", reign_count);
  }
  //printf("\n");
}

void FergEvolution :: squirrel_infectors(vector<Person *> &reign, int cluster) {
  int disease_id = disease->get_id();
  reign.clear();
  vector<Person *> infectious;
  disease->get_epidemic()->get_infectious_samples(infectious, 1.0);
  for(unsigned int i=0; i<infectious.size(); i++) {
    if(get_cluster(infectious.at(i)) == cluster) {
      reign.push_back(infectious.at(i));
      Infection *inf = infectious.at(i)->get_health()->get_infection(disease_id);
      assert(inf != NULL);
      vector<int> strains;
      inf->get_strains(strains);
      assert(strains.size() > 0);
    }
  }
  SQUIRRELED = true;
}

bool FergEvolution :: reignite(Person *pers, int day) {
  int disease_id = disease->get_id();
  Infection *curr_inf = pers->get_health()->get_infection(disease_id);
  if(curr_inf != NULL) return false;
  
  // do not re-reignite people..
  pers->become_susceptible(disease);
  //if(pers->get_health()->get_infection(disease_id) != NULL) return false; 
 
  int n = pers->get_num_past_infections(this->disease->get_id());
  if(n == 0){
    printf("ERROR! No past infections!!\n");
    return false;
  }
  Past_Infection * pastInf = pers->get_past_infection(this->disease->get_id(), n-1);
  vector<int> old_strains;
  pastInf->get_strains(old_strains);
  if(old_strains.size() < 1) {
    printf("ERROR! No old strains!!\n");
    return false;
  }
  int strain = old_strains[old_strains.size()-1];
  Transmission *transmission = new Transmission(NULL, NULL, day);
  transmission->set_forcing(true); // force the transmission to succeed
  transmission->set_initial_loads(Evolution::get_primary_loads(day, strain));
  //pers->clear_past_infections(this->disease->get_id());
  pers->become_exposed(this->disease, transmission);
  //printf("Reignited person %d with strain %d\n", pers->get_id(), strain);
  return true;
}

int FergEvolution :: get_weekly_inc(int week, int cluster) {
  if(cluster >= num_clusters) return 0;
  vector< vector<int> > & cases = new_cases_clusters.at(cluster);
  int start = week*7, end = start+7;
  if(week < 0 || start >= cases.size()) return 0;
  end = (end < cases.size())?end:cases.size()-1;
  int week_cases = 0;
  for(int d=start; d<end; d++) {
    vector<int> &day_cases = cases.at(d);
    for(int i=0; i<day_cases.size(); i++) {
      week_cases += day_cases.at(i);
    }
  }
  return week_cases;
}

map<int, double> * FergEvolution::get_primary_loads(int day) {
  int strain = 0;
  return Evolution::get_primary_loads(day, strain);
}

void FergEvolution :: terminate_person(Person *p) {
  for(int c=0; c<reignitors.size(); c++) {
    for(int i=0; i<reignitors[c].size(); i++) {
      if(reignitors[c][i] == p){
        reignitors[c][i] = NULL;
      }
    }
  }
}
