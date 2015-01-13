
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
#include "Age_Map.h"
#include "Params.h"
#include <map>
#include <cmath>
#include <fstream>

using namespace std;

MSEvolution :: MSEvolution()
{ 
  //char fname[256];
  //get_param((char *)"outfile", fname);
  //file.open(fname);
  
  halflife_inf = NULL;
  halflife_vac = NULL;
  init_prot_inf = 0.0;
  init_prot_vac = 0.0;
  sat_quantity = 0.0;
  protection = NULL;
}

void MSEvolution :: setup(Disease *disease)
{
  Evolution::setup(disease);
  halflife_inf = new Age_Map("Infection Protection Half Life");
  halflife_inf->read_from_input("half_life_inf", disease->get_id());
  halflife_vac = new Age_Map("Vaccination Protection Half Life");
  halflife_vac->read_from_input("half_life_vac", disease->get_id());
  Params::get_param((char *) "init_protection_inf", &init_prot_inf);
  Params::get_param((char *) "init_protection_vac", &init_prot_vac);
  Params::get_param((char *) "saturation_quantity", &sat_quantity);
  protection = new Piecewise_Linear;
  protection->setup("strain_dependent_protection", disease);
}

MSEvolution :: ~MSEvolution()
{
  //file.close();
  delete halflife_inf;
  delete halflife_vac;
  delete protection;
}

inline double MSEvolution::residual_immunity(Person *person, int challenge_strain, int day) {
  double probTaking = 1-Evolution::residual_immunity(person, challenge_strain, day);

  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(person, challenge_strain, day);

  // Pr(Taking | Protective Vaccinations)
  //probTaking *= prob_past_vaccinations(person, challenge_strain, day);

  return (1-probTaking);
}

double MSEvolution :: prob_inoc(double quantity)
{
  static double norm = 1 - exp(-1);
  double prob = (1.0 - exp(- quantity / sat_quantity)) / norm;
  return (prob < 1.0) ? prob:1.0;
}

double MSEvolution :: antigenic_distance(int strain1, int strain2)
{
  //if(strain1 == 0 || strain2 == 0) return 10;
  //if(strain1 == strain2) return 0.0;
  //else return 10.0;
  int diff = strain1 - strain2;
  //cout << "STRAINS: " << strain1 << " " << strain2 << " " << diff << endl;
  if(diff*diff == 0) return 0.0;
  else if(diff*diff == 1) return 1.0;
  else return 10.0;
}

double MSEvolution :: prob_inf_blocking(int old_strain, int new_strain, int time, int age){
  return prob_blocking(old_strain, new_strain, time, halflife_inf->find_value(age), init_prot_inf ); 
}

double MSEvolution :: prob_vac_blocking(int old_strain, int new_strain, int time, int age){
  return prob_blocking(old_strain, new_strain, time, halflife_vac->find_value(age), init_prot_vac ); 
}

double MSEvolution :: prob_blocking(int old_strain, int new_strain, int time, double halflife, double init_prot){
  double prob_block = 1.0;
  // Generalized
  //cout << "TIME: " << time << " " << halflife << " " << exp(-time/ (halflife/0.693)) << endl;
  // if(time == 0) return 1.0;
  //if(old_strain == 0) { // past infection (from past_infections file)
  //  prob_block *= (1 - init_prot * exp( - time / (halflife*0.693) ));
  //}
  
  //prob_block *= (1 - init_prot * exp( - time / (halflife/0.693) ));
  //double ad = antigenic_distance(old_strain, new_strain);
  //prob_block = protection->get_prob(ad);
  //prob_block *= (1 - protection->get_prob(ad));
  //prob_block = 1 - prob_block;
  //if(ad != 0) cout << "Blocking: " << ad << " " << prob_block << endl;
  
  
  prob_block *= (1-init_prot * exp( - time / (halflife/0.693) ));

  // Strain-dep
  double ad = antigenic_distance(old_strain, new_strain);
  prob_block *= (1-protection->get_prob(ad));
  
  //cout << "ANT_DIST " << ad << " " << prob_block;

  assert(prob_block >= 0.0 && prob_block <= 1.0);
  return (1-prob_block);
}

double MSEvolution :: prob_past_infections(Person *infectee, int new_strain, int day){
  int disease_id = disease->get_id();
  double probTaking = 1.0;
  int n = infectee->get_num_past_infections(disease_id);
  Past_Infection *pastInf;
  vector<int> old_strains;
  for(int i=0; i<n; i++){
    pastInf = infectee->get_past_infection(disease_id, i);
    pastInf->get_strains(old_strains);
    for(unsigned int j=0; j<old_strains.size(); j++){
      //printf("DATES: %d %d\n", day, pastInf->get_recovery_date()); 
      probTaking *= (1 - prob_inf_blocking(old_strains[j], new_strain, 
            day - pastInf->get_recovery_date(), pastInf->get_age_at_exposure()));
    }
  }
  return probTaking;
}

double MSEvolution :: prob_past_vaccinations(Person *infectee, int new_strain, int day){
  double probTaking = 1.0;

  // TODO Handle getting past vaccinations through person instead of infection
/*  int n = infection->get_num_past_vaccinations();
  cout << "VACC " << n << endl;
  Infection *pastInf;
  vector<int> old_strains; 
  for(int i=0; i<n; i++){
    pastInf = infection->get_past_vaccination(i);
    if(! pastInf->provides_immunity()) continue;
    else{
      pastInf->get_strains(old_strains);
      for(unsigned int i=0; i<old_strains.size(); i++){
        probTaking *= (1 - prob_vac_blocking(old_strains[i], new_strain, 
              day - pastInf->get_exposure_date(), pastInf->get_age_at_exposure()));
      }
    }
  }*/
  return probTaking;
}

double MSEvolution :: get_prob_taking(Person *infectee, int new_strain, double quantity, int day)
{
  double probTaking = 1.0;

  // Pr(Taking | quantity)
  probTaking *= prob_inoc(quantity);

  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(infectee, new_strain, day);

  // Pr(Taking | Protective Vaccinations)
  probTaking *= prob_past_vaccinations(infectee, new_strain, day);

  return probTaking;
}

Infection *MSEvolution :: transmit(Infection *infection, Transmission *transmission, Person *infectee)
{
  // Transmissibility
  int day = transmission->get_exposure_date();
  map<int, double> *loads = transmission->get_initial_loads();
  map<int, double> *failed_loads = new map<int, double>;
  bool force = transmission->get_forcing();
  map<int, double> :: iterator it;
  bool failed = true;
  for(it=loads->begin(); it!=loads->end();){
    double trans;
    if(force) {
     trans = 1.0;
    } else{
     trans = get_prob_taking(infectee, it->first, it->second, day);
    }
    //if(trans < 0.0001) trans = 0;
    //assert(trans <= 1.0 && trans >= 0.0);
    double r = RANDOM();
    //if(it->second < 0.01) trans = 0;
    if(r <= trans){
      failed = false;
      it++;
    }
    else{
      failed_loads->insert( pair<int, double> (it->first, it->second) );
      loads->erase( it++ );
    }
  }
  if(!failed){
    transmission->set_initial_loads(loads);
    infection = Evolution::transmit(infection, transmission, infectee);
    //infection->set_susceptibility_period(1);
    return infection;
  }
  else{
    transmission->set_initial_loads(failed_loads);
    return NULL;
  }
}

