/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <cmath>

#include "Age_Map.h"
#include "MSEvolution.h"
#include "Params.h"
#include "Past_Infection.h"
#include "Person.h"
#include "Piecewise_Linear.h"


MSEvolution::MSEvolution() { 
  this->halflife_inf = NULL;
  this->halflife_vac = NULL;
  this->init_prot_inf = 0.0;
  this->init_prot_vac = 0.0;
  this->sat_quantity = 0.0;
  this->protection = NULL;
  this->prob_inoc_norm = 0.0;
}

void MSEvolution::setup( Disease * disease ) {
  Evolution::setup(disease);
  this->halflife_inf = new Age_Map("Infection Protection Half Life");
  this->halflife_inf->read_from_input("half_life_inf", disease->get_id());

  this->halflife_vac = new Age_Map("Vaccination Protection Half Life");
  this->halflife_vac->read_from_input("half_life_vac", disease->get_id());
  
  Params::get_param((char*) "init_protection_inf", &this->init_prot_inf);
  Params::get_param((char*) "init_protection_vac", &this->init_prot_vac);
  Params::get_param((char*) "saturation_quantity", &this->sat_quantity);
  
  this->protection = new Piecewise_Linear;
  this->protection->setup("strain_dependent_protection", disease);

  this->prob_inoc_norm = 1 - exp(-1);
 
}

MSEvolution::~MSEvolution() {
  delete this->halflife_inf;
  delete this->halflife_vac;
  delete this->protection;
}

inline double MSEvolution::residual_immunity(Person* person, int challenge_strain, int day)  {
  double probTaking = 1 - Evolution::residual_immunity(person, challenge_strain, day);
  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(person, challenge_strain, day);
  // Pr(Taking | Protective Vaccinations)
  //probTaking *= prob_past_vaccinations(person, challenge_strain, day);
  return (1 - probTaking);
}

double MSEvolution::prob_inoc( double quantity ) {
  //static double norm = 1 - exp( -1 );
  double prob = ( 1.0 - exp((0 - quantity ) / this->sat_quantity)) / this->prob_inoc_norm;
  return (prob < 1.0) ? prob : 1.0;
}

double MSEvolution::antigenic_distance( int strain1, int strain2 ) {
  int diff = strain1 - strain2;
  if(diff * diff == 0) {
    return 0.0;
  } else if(diff * diff == 1) {
    return 1.0;
  } else {
    return 10.0;
  }
}

double MSEvolution::prob_inf_blocking(int old_strain, int new_strain, int time, double real_age) {
  FRED_VERBOSE( 3, "Prob Blocking %f old strain %d new strain %d time %d halflife %f age %.2f init prot inf %f\n",
		prob_blocking( old_strain, new_strain, time, halflife_inf->find_value( real_age ), init_prot_inf ),
		old_strain, new_strain, time, halflife_inf->find_value( real_age ), real_age, init_prot_inf );
  return prob_blocking( old_strain, new_strain, time, this->halflife_inf->find_value( real_age ), this->init_prot_inf );
}

double MSEvolution::prob_vac_blocking(int old_strain, int new_strain, int time, double real_age) {
  return prob_blocking( old_strain, new_strain, time, this->halflife_vac->find_value( real_age ), this->init_prot_vac );
}

double MSEvolution::prob_blocking(int old_strain, int new_strain, int time, double halflife, double init_prot) {
  double prob_block = 1.0;
  // Generalized Immunity
  prob_block *= (1 - (init_prot * exp((0 - time) / (halflife / 0.693))));
  // Strain Dependent Immunity 
  double ad = antigenic_distance( old_strain, new_strain);
  prob_block *= (1 - this->protection->get_prob(ad));
  // Make sure that it's a valid probability 
  assert(prob_block >= 0.0 && prob_block <= 1.0);
  return (1 - prob_block);
}

double MSEvolution::prob_past_infections(Person* infectee, int new_strain, int day) {
  int disease_id = this->disease->get_id();
  double probTaking = 1.0;
  int n = infectee->get_num_past_infections(disease_id);
  for(int i = 0; i < n; ++i) {
    Past_Infection * past_infection = infectee->get_past_infection(disease_id, i);
    //printf("DATES: %d %d\n", day, pastInf->get_infectious_end_date()); 
    probTaking *= (1 - prob_inf_blocking(past_infection->get_strain(), new_strain,
					   day - past_infection->get_infectious_end_date(), past_infection->get_age_at_exposure()));
  }
  return probTaking;
}

double MSEvolution::prob_past_vaccinations(Person* infectee, int new_strain, int day) {
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

double MSEvolution::get_prob_taking(Person* infectee, int new_strain, double quantity, int day) {
  double probTaking = 1.0;
  // Pr(Taking | quantity)
  probTaking *= prob_inoc(quantity);
  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(infectee, new_strain, day);
  // Pr(Taking | Protective Vaccinations)
  probTaking *= prob_past_vaccinations(infectee, new_strain, day);
  return probTaking;
}

/*
  Infection * MSEvolution::transmit( Infection * infection, Transmission & transmission, Person * infectee ) {
  int day = transmission.get_exposure_date();
  Transmission::Loads * loads = transmission.get_initial_loads();

  Transmission::Loads::iterator it;
  for ( it = loads->begin(); it != loads->end(); ) {
  double trans = get_prob_taking( infectee, it->first, it->second, day );
  if ( Random::draw_random() <= trans ) {
  it++;
  }
  else {
  loads->erase( it++ );
  }
  }

  if ( loads->empty() ) {
  return NULL;
  }
  else {
  return Evolution::transmit( infection, transmission, infectee );
  }
  }
*/

