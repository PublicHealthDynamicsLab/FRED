/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Place.cc
//

#include "Place.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Infection.h"
#include "Transmission.h"
#include "Date.h"
#include "Neighborhood.h"
#include "Cell.h"
#include "Seasonality.h"
#include "Utils.h"
#include "Large_Grid.h"
#include "Large_Cell.h"

void Place::setup( const char *lab, fred::geo lon, fred::geo lat, Place* cont, Population *pop ) {
  population = pop;
  // actual id assigned in Place_List::add_place
  id = -1;
  container = cont;
  strcpy(label, lab);
  longitude = lon;
  latitude = lat;
  
  // allocate disease-related memory
  susceptibles = new (nothrow) vector<Person *> [Global::Diseases];
  assert (susceptibles != NULL);
  infectious = new (nothrow) vector<Person *> [Global::Diseases];
  assert (infectious != NULL);
  S = new (nothrow) int [Global::Diseases];
  assert (S != NULL);
  I = new (nothrow) int [Global::Diseases];
  assert (I != NULL);
  Sympt = new (nothrow) int [Global::Diseases];
  assert (Sympt != NULL);
  cases = new (nothrow) int [Global::Diseases];
  assert (cases != NULL);
  total_cases = new (nothrow) int [Global::Diseases];
  assert (total_cases != NULL);
  deaths = new (nothrow) int [Global::Diseases];
  assert (deaths != NULL);
  total_deaths = new (nothrow) int [Global::Diseases];
  assert (total_deaths != NULL);
  if ( Global::Report_Incidence ) {
    incidence.resize(Global::Diseases);
  }
  if ( Global::Report_Prevalence ) {
    prevalence.resize(Global::Diseases);
  }
  N = 0;
  days_infectious = 0;
  total_infections = 0;
}

void Place::print_stats(int day, int disease_id) {
  /*
   * The output format for this needs to be changed and 
   * it produces an insane amount of output in the current format
   *

   // print out incidence and prevalence values
   map<int, int> :: iterator it;
   fprintf(Incfp, "%d %s ", day, label);
   int tot = 0;
   for(it = incidence[disease_id].begin(); it != incidence[disease_id].end(); it++){
   tot += it->second;
   }
   fprintf(Incfp, "%d ", tot);

   for(it = incidence[disease_id].begin(); it != incidence[disease_id].end(); it++){
   fprintf(Incfp, " %d %d ", it->first, it->second);
   }
   fprintf(Incfp, "\n");

   fprintf(Prevfp, "%d %s ", day, label);
   for(it = incidence[disease_id].begin(); it != incidence[disease_id].end(); it++){
   tot += it->second;
   }
   fprintf(Prevfp, "%d ", tot);

   for(it = prevalence[disease_id].begin(); it != prevalence[disease_id].end(); it++){
   fprintf(Prevfp, "%d %d ", it->first, it->second);
   }
   fprintf(Prevfp, "\n");
  */
}


void Place::prepare() {
  for (int d = 0; d < Global::Diseases; d++) {
    susceptibles[d].reserve(N);
    infectious[d].reserve(N);
    total_cases[d] = total_deaths[d] = 0;
  }
  update(0);
  open_date = 0;
  close_date = INT_MAX;
  if (Global::Verbose > 2) {
    printf("prepare place: %d\n", id);
    print(0);
    fflush(stdout);
  }
}

void Place::update(int day) {
  for (int d = 0; d < Global::Diseases; d++) {
    // if(day > 0) print_stats(day, d);
    cases[d] = deaths[d] = 0;
    susceptibles[d].clear();
    infectious[d].clear();
    Sympt[d] = S[d] = I[d] = 0;
    if ( Global::Report_Incidence ) 
      incidence[d].clear();
    if ( Global::Report_Prevalence )
      prevalence[d].clear();
  }
}

void Place::print(int disease_id) {
  printf("Place %d label %s type %c ", id, label, type);
  printf("S %d I %d N %d\n", S[disease_id], I[disease_id], N);
  fflush(stdout);
}

void Place::enroll(Person * per) {
  N++;
}

void Place::unenroll(Person * per) {
  N--;
}

void Place::add_susceptible(int disease_id, Person * per) {
  // Protect from concurrent writes
  fred::Scoped_Lock lock( mutex ); 
  // push person onto susceptible container for this place
  susceptibles[disease_id].push_back(per);
  S[disease_id]++;
  assert (S[disease_id] == static_cast <int> (susceptibles[disease_id].size()));
}

void Place::add_susceptibles( int disease_id,
    std::vector< Person * > & _susceptibles ) {

  // Protect from concurrent writes
  fred::Scoped_Lock lock( mutex ); 
  // push person onto susceptible container for this place
  susceptibles[ disease_id ].insert( susceptibles[ disease_id ].end(),
     _susceptibles.begin(), _susceptibles.end() );

  S[ disease_id ] += _susceptibles.size(); 
  assert ( S[ disease_id ] == static_cast < int > ( susceptibles[ disease_id ].size() ) );
}

void Place::add_infectious(int disease_id, Person * per) {
  
  // Protect from concurrent writes
  fred::Scoped_Lock lock( mutex );
  // push person onto infectious container for this place
  infectious[disease_id].push_back(per);
  I[disease_id]++;
  assert(I[disease_id] == static_cast <int> (infectious[disease_id].size()));

  
  if ( I[ disease_id ] == 1 ) {
    Disease * dis = population->get_disease(disease_id);
    dis->add_infectious_place(this, type);
  }

  if (per->get_health()->is_symptomatic()) {
    #pragma omp atomic
    Sympt[disease_id]++;
    #pragma omp atomic
    cases[disease_id]++;
    #pragma omp atomic
    total_cases[disease_id]++;
  }
}

void Place::print_susceptibles(int disease_id) {
  vector<Person *>::iterator itr;
  for (itr = susceptibles[disease_id].begin();
       itr != susceptibles[disease_id].end(); itr++) {
    printf(" %d", (*itr)->get_id());
  }
  printf("\n");
}

void Place::print_infectious(int disease_id) {
  vector<Person *>::iterator itr;
  for (itr = infectious[disease_id].begin();
       itr != infectious[disease_id].end(); itr++) {
    printf(" %d", (*itr)->get_id());
  }
  printf("\n");
}

bool Place::is_open(int day) {
  if (container) {
    return container->is_open(day);
  } else {
    return (day < close_date || open_date <= day);
  }
}

double Place::get_contact_rate(int day, int disease_id) {
  
  Disease * disease = population->get_disease(disease_id);
  // expected number of susceptible contacts for each infectious person
  // OLD: double contacts = get_contacts_per_day(s) * ((double) S[s]) / ((double) (N-1));
  double contacts = get_contacts_per_day(disease_id) * disease->get_transmissibility();
  if (Global::Enable_Seasonality) {

    //contacts = contacts * Global::Clim->get_seasonality_multiplier_by_lat_lon(
    //    latitude,longitude,disease_id);

    double m = Global::Clim->get_seasonality_multiplier_by_lat_lon(
        latitude,longitude,disease_id);
    //cout << "SEASONALITY: " << day << " " << m << endl;
    contacts *= m;
  }

  // increase neighborhood contacts on weekends
  if (type == NEIGHBORHOOD) {
    int day_of_week = Global::Sim_Current_Date->get_day_of_week();
    if (day_of_week == 0 || day_of_week == 6) {
      contacts = Neighborhood::get_weekend_contact_rate(disease_id) * contacts;
    }
  }
  // Utils::fred_verbose(1,"Disease %d, expected contacts = %f\n", disease_id, contacts);
  return contacts;
}

int Place::get_contact_count(Person * infector, int disease_id, int day, double contact_rate) {
  // reduce number of infective contacts by infector's infectivity
  double infectivity = infector->get_infectivity(disease_id, day);
  double infector_contacts = contact_rate * infectivity;

  FRED_VERBOSE( 1, "infectivity = %f, so ", infectivity );
  FRED_VERBOSE( 1, "infector's effective contacts = %f\n", infector_contacts );
  
  // randomly round off the expected value of the contact counts
  int contact_count = (int) infector_contacts;
  double r = RANDOM();
  if (r < infector_contacts - contact_count) contact_count++;

  FRED_VERBOSE( 1, "infector contact_count = %d  r = %f\n", contact_count, r );
  
  return contact_count;
}

void Place::attempt_transmission(double transmission_prob, Person * infector, 
                                        Person * infectee, int disease_id, int day) {

  double susceptibility = infectee->get_susceptibility(disease_id);
  
  FRED_VERBOSE( 2, "susceptibility = %f\n", susceptibility );

  double r = RANDOM();
  double infection_prob = transmission_prob * susceptibility;
  
  FRED_CONDITIONAL_VERBOSE( 1, r >= infection_prob,
      "transmission failed: r = %f  prob = %f\n", r, infection_prob );

  if (r < infection_prob) {
    // successful transmission; create a new infection in infectee
    // TODO this should be a const reference instead of new allocation???
    Transmission *transmission = new Transmission(infector, this, day);
    infector->infect( infectee, disease_id, transmission );

    if (disease_id == 0) {
      #pragma omp atomic
      total_infections++;
    }

    FRED_VERBOSE( 1, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob );
    FRED_CONDITIONAL_VERBOSE( 1, infector->get_exposure_date(disease_id) == 0,
        "SEED infection day %i from %d to %d\n", day, infector->get_id(),infectee->get_id() );
    FRED_CONDITIONAL_VERBOSE( 1, infector->get_exposure_date(disease_id) != 0,
        "infection day %i of disease %i from %d to %d\n",
        day, disease_id, infector->get_id(), infectee->get_id() );
    FRED_CONDITIONAL_VERBOSE( 3, infection_prob > 1, "infection_prob exceeded unity!\n" );

  }
}

void Place::spread_infection(int day, int disease_id) {
  // Place::spread_infection is used for all derived places except for Households

  // the number of possible infectees per infector is max of (N-1) and S[s]
  // where N is the capacity of this place and S[s] is the number of current susceptibles
  // visiting this place.  S[s] might exceed N if we have some ad hoc visitors,
  // since N is estimated only at startup.
  int number_targets = (N-1 > S[disease_id]? N-1 : S[disease_id]);

  FRED_STATUS(1,"spread_infection: Disease %d day %d place %d type %c\n",
      disease_id, day, id, type);

  if ( disease_id == 0 ) days_infectious++;
  if ( number_targets == 0 ) return;
  if ( is_open( day ) == false ) return;
  if ( should_be_open( day, disease_id ) == false ) return;

  vector<Person *>::iterator itr;
  // contact_rate is contacts_per_day with weeked and seasonality modulation (if applicable)
  double contact_rate = get_contact_rate(day,disease_id);

  // randomize the order of the infectious list
  FYShuffle<Person *>(infectious[disease_id]);

  for (itr = infectious[disease_id].begin(); itr != infectious[disease_id].end(); ++itr) {
    Person * infector = *itr;      // infectious indiv
    assert(infector->get_health()->is_infectious(disease_id));
    
    // get the actual number of contacts to attempt to infect
    int contact_count = get_contact_count(infector,disease_id,day,contact_rate);
    
    // report saturation in this place
    FRED_CONDITIONAL_VERBOSE(3, contact_count > number_targets, 
        "frustration! making %d attempts to infect %d targets\n",
        contact_count,number_targets);

    // get a susceptible target for each contact resulting in infection
    for (int c = 0; c < contact_count; ++c) {
      // select a target infectee with replacement, including all possible visitors
      int pos = IRAND(0,number_targets-1);
      if (pos > S[disease_id]-1) continue; // target is not one of the susceptibles present
      // at this point we have a susceptible target
      // make sure that we aren't out of range
      FRED_CONDITIONAL_WARNING( pos < 0 || pos >= susceptibles[ disease_id ].size(),
          " susceptibles.size() = %d pos = %d\n",
          (int) susceptibles[ disease_id ].size(),
          pos );
      assert( pos >= 0 && pos < susceptibles[ disease_id ].size() );
      Person * infectee = susceptibles[ disease_id ][ pos ];

      FRED_VERBOSE( 1, "possible infectee = %d  pos = %d  S[%d] = %d\n",
            infectee->get_id(), pos, disease_id, S[disease_id] );

      // is the target still susceptible?
      if ( infectee->is_susceptible( disease_id ) ) {
        // get the transmission probs for this infector/infectee pair
        double transmission_prob = get_transmission_prob(disease_id, infector, infectee);
        
        FRED_STATUS(1,"infectee is susceptible\n","");
        FRED_STATUS(1,"trans_prob = %f\n", transmission_prob);
        
        attempt_transmission( transmission_prob, infector, infectee, disease_id, day );

      } // end of susceptible infectee
    } // end contact loop
  } // end infectious list loop
}

void Place::modify_incidence_count(int disease_id, vector<int> strains, int incr) {
  if ( Global::Report_Incidence ) {
    map<int, int>& inc = incidence[disease_id];
    for(int s=0; s < (int) strains.size(); s++) {
      if(inc.find(strains[s]) == inc.end()) inc[strains[s]] = 0;
      inc[strains[s]] = inc[strains[s]] + 1;
    }
  }
}

void Place::modify_prevalence_count(int disease_id, vector<int> strains, int incr) {
  if ( Global::Report_Prevalence ) {
    map<int, int>& prev = prevalence[disease_id];
    for(int s=0; s < (int) strains.size(); s++) {
      if(prev.find(strains[s]) == prev.end()) prev[strains[s]] = 0; 
      prev[strains[s]] += 1;
    }
  }
}

Place * Place::select_neighborhood(double community_prob, double community_distance, double local_prob) {
  return grid_cell->select_neighborhood(community_prob, community_distance, local_prob);
}

