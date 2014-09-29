/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <map>
#include <cmath>
#include <fstream>

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

using namespace std;


MSEvolution::MSEvolution() { 
  halflife_inf = NULL;
  halflife_vac = NULL;
  init_prot_inf = 0.0;
  init_prot_vac = 0.0;
  sat_quantity = 0.0;
  protection = NULL;
}

void MSEvolution::setup( Disease * disease ) {
  Evolution::setup(disease);
  halflife_inf = new Age_Map("Infection Protection Half Life");
  halflife_inf->read_from_input("half_life_inf", disease->get_id());

  halflife_vac = new Age_Map("Vaccination Protection Half Life");
  halflife_vac->read_from_input("half_life_vac", disease->get_id());
  
  Params::get_param((char *) "init_protection_inf", &init_prot_inf);
  Params::get_param((char *) "init_protection_vac", &init_prot_vac);
  Params::get_param((char *) "saturation_quantity", &sat_quantity);
  
  protection = new Piecewise_Linear;
  protection->setup( "strain_dependent_protection", disease );

  prob_inoc_norm = 1 - exp( -1 );
 
}

MSEvolution::~MSEvolution() {
  delete halflife_inf;
  delete halflife_vac;
  delete protection;
}

inline double MSEvolution::residual_immunity( Person * person, int challenge_strain, int day ) {
  double probTaking = 1 - Evolution::residual_immunity( person, challenge_strain, day );
  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(person, challenge_strain, day);
  // Pr(Taking | Protective Vaccinations)
  //probTaking *= prob_past_vaccinations(person, challenge_strain, day);
  return ( 1 - probTaking );
}

double MSEvolution::prob_inoc( double quantity ) {
  //static double norm = 1 - exp( -1 );
  double prob = ( 1.0 - exp( ( 0 - quantity ) / sat_quantity ) ) / prob_inoc_norm;
  return ( prob < 1.0 ) ? prob : 1.0;
}

double MSEvolution::antigenic_distance( int strain1, int strain2 ) {
  int diff = strain1 - strain2;
  if ( diff * diff == 0 ) return 0.0;
  else if ( diff * diff == 1 ) return 1.0;
  else return 10.0;
}

double MSEvolution::prob_inf_blocking( int old_strain, int new_strain, int time, int age ) {
  FRED_VERBOSE( 3, "Prob Blocking %f old strain %d new strain %d time %d halflife %f age %d init prot inf %f\n",
      prob_blocking( old_strain, new_strain, time, halflife_inf->find_value( age ), init_prot_inf ),
       old_strain, new_strain, time, halflife_inf->find_value( age ), age, init_prot_inf );
  return prob_blocking( old_strain, new_strain, time, halflife_inf->find_value( age ), init_prot_inf ); 
}

double MSEvolution::prob_vac_blocking( int old_strain, int new_strain, int time, int age ) {
  return prob_blocking( old_strain, new_strain, time, halflife_vac->find_value( age ), init_prot_vac ); 
}

double MSEvolution::prob_blocking( int old_strain, int new_strain, int time, double halflife, double init_prot ) {
  double prob_block = 1.0;
  // Generalized Immunity
  prob_block *= ( 1 - ( init_prot * exp( ( 0 - time ) / ( halflife / 0.693 ) ) ) );
  // Strain Dependent Immunity 
  double ad = antigenic_distance( old_strain, new_strain );
  prob_block *= ( 1 - protection->get_prob( ad ) );
  // Make sure that it's a valid probability 
  assert( prob_block >= 0.0 && prob_block <= 1.0 );
  return ( 1 - prob_block );
}

double MSEvolution::prob_past_infections( Person * infectee, int new_strain, int day ) {
  int disease_id = disease->get_id();
  double probTaking = 1.0;
  int n = infectee->get_num_past_infections( disease_id );
  for( int i = 0; i < n; i++ ) {
    Past_Infection * past_infection = infectee->get_past_infection( disease_id, i );
    //printf("DATES: %d %d\n", day, pastInf->get_recovery_date()); 
    probTaking *= ( 1 - prob_inf_blocking( past_infection->get_strain(), new_strain, 
          day - past_infection->get_recovery_date(), past_infection->get_age_at_exposure() ) );
  }
  return probTaking;
}

double MSEvolution::prob_past_vaccinations( Person * infectee, int new_strain, int day ) {
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

double MSEvolution::get_prob_taking( Person * infectee, int new_strain, double quantity, int day ) {
  double probTaking = 1.0;
  // Pr(Taking | quantity)
  probTaking *= prob_inoc(quantity);
  // Pr(Taking | Past-Infections)
  probTaking *= prob_past_infections(infectee, new_strain, day);
  // Pr(Taking | Protective Vaccinations)
  probTaking *= prob_past_vaccinations(infectee, new_strain, day);
  return probTaking;
}

Infection * MSEvolution::transmit( Infection * infection,
    Transmission & transmission, Person * infectee ) {

  int day = transmission.get_exposure_date();
  
  Transmission::Loads * loads = transmission.get_initial_loads();

  bool force = transmission.get_forcing();
  Transmission::Loads::iterator it;
  
  for ( it = loads->begin(); it != loads->end(); ) {
    
    double trans = force ? 1.0 : get_prob_taking( infectee, it->first, it->second, day );
    
    if ( RANDOM() <= trans ) {
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

