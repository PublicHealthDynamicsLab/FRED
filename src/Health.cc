/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Health.cc
//

#include <new>
#include <stdexcept>

#include "Health.h"
#include "Place.h"
#include "Person.h"
#include "Disease.h"
#include "Evolution.h"
#include "Infection.h"
#include "Antiviral.h"
#include "Population.h"
#include "Random.h"
#include "Manager.h"
#include "AV_Manager.h"
#include "AV_Health.h"
#include "Vaccine.h"
#include "Vaccine_Dose.h"
#include "Vaccine_Health.h"
#include "Vaccine_Manager.h"
#include "Transmission.h"
#include "Past_Infection.h"
#include "Utils.h"

int nantivirals = -1; // This global variable needs to be removed
char dummy_label[8];

Health::Health (Person * person) {
  person_index = person->get_pop_index(); 
  past_infections = new vector< Past_Infection > [Global::Diseases];
  alive = true;
  intervention_flags = intervention_flags_type();
  // infection pointers stored in statically allocated array (length of which
  // is determined by static constant Global::MAX_NUM_DISEASES)
  active_infections = std::bitset< Global::MAX_NUM_DISEASES >();
  susceptibility_multp = std::vector< double >( Global::Diseases, 1.0 );
  susceptible = fred::disease_bitset();
  infectious = fred::disease_bitset();
  symptomatic = std::bitset< Global::MAX_NUM_DISEASES >();
  recovered_today = std::bitset< Global::MAX_NUM_DISEASES >();
  evaluate_susceptibility = std::bitset< Global::MAX_NUM_DISEASES >();

  for (int disease_id = 0; disease_id < Global::Diseases; disease_id++) {
    infection[ disease_id ] = NULL;
    infectee_count[ disease_id ] = 0;
    susceptible_date[ disease_id ] = -1;
    become_susceptible(disease_id);
  }

  if(nantivirals == -1) {
    Params::get_param_from_string("number_antivirals",&nantivirals);
  }

  immunity = fred::disease_bitset(); 

  vaccine_health.clear();
  intervention_flags[ takes_vaccine ] = false;

  av_health.clear();
  //takes_av = false;
  //intervention_flags[ takes_av ] = false // takes_av flag false on initialization
  checked_for_av.assign(nantivirals,false);

  // Determine if the agent is at risk
  at_risk = fred::disease_bitset();

  for(int disease_id = 0; disease_id < Global::Diseases; disease_id++) {
    Disease* disease = Global::Pop.get_disease(disease_id);

    if(!disease->get_at_risk()->is_empty()) {
      double at_risk_prob = disease->get_at_risk()->find_value( person->get_age() );

      if(RANDOM() < at_risk_prob) { // Now a probability <=1.0
        declare_at_risk(disease);
      }
    }
  }
}

Health::~Health() {
  // delete Infection objects pointed to; array is on stack, just let
  // it go out of scope
  for (size_t i = 0; i < Global::Diseases; ++i) {
    delete infection[i];
  }

  for(unsigned int i=0; i<vaccine_health.size(); i++) {
    delete vaccine_health[i];
  }

  vaccine_health.clear();
  intervention_flags[ takes_vaccine ] = false;

  for(unsigned int i=0; i<av_health.size(); i++)
    delete av_health[i];
  av_health.clear();

  /*
  for(int d=0; d<Global::Diseases; d++)
    for(unsigned int i=0; i<past_infections[d].size(); i++)
      delete past_infections[d][i];
  delete[] past_infections;
  */

  intervention_flags[ takes_av ] = false;
}

void Health::become_susceptible(int disease_id) {
  if (susceptible[disease_id])
    return;
  assert( !(active_infections[ disease_id ]) );
  susceptibility_multp[disease_id] = 1.0;
  susceptible[disease_id] = true;
  evaluate_susceptibility.reset(disease_id) = false; 
  Disease * disease = Global::Pop.get_disease(disease_id);
  
  Person * self = Global::Pop.get_person_by_index( person_index );
  
  disease->become_susceptible( self );
  
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now SUSCEPTIBLE for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}

void Health::become_susceptible(Disease * disease) {
  become_susceptible(disease->get_id());
}

void Health::become_exposed(Disease *disease, Transmission *transmission) {
  int disease_id = disease->get_id();
  infectious[disease_id] = false;
  symptomatic.reset(disease_id);
 
  Person * self = Global::Pop.get_person_by_index( person_index );

  Infection *new_infection = disease->get_evolution()->transmit(infection[disease_id], transmission, self );
  if ( new_infection != NULL ) { // Transmission succeeded
    active_infections.set( disease_id );
    if ( infection[ disease_id ] == NULL){
      self->become_unsusceptible( disease );
      disease->become_exposed( self );
    }
    infection[disease_id] = new_infection;
    susceptible_date[disease_id] = -1;
    if (Global::Verbose > 1) {
      if (!transmission->get_infected_place())
        fprintf(Global::Statusfp, "SEEDED person %d with disease %d\n",
            get_person_id(), disease->get_id());
      else
        fprintf(Global::Statusfp, "EXPOSED person %d to disease %d\n",
            get_person_id(), disease->get_id());
    }
  }
}

void Health::become_unsusceptible(Disease * disease) {
  int disease_id = disease->get_id();
  if (susceptible[disease_id] == false)
    return;
  susceptible[disease_id] = false;
  Person * self = Global::Pop.get_person_by_index( person_index );
  disease->become_unsusceptible(self);
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now UNSUSCEPTIBLE for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}

void Health::become_infectious(Disease * disease) {
  int disease_id = disease->get_id();
  assert(active_infections[ disease_id ]);
  infectious[disease_id] = true;
  Person * self = Global::Pop.get_person_by_index( person_index );
  disease->become_infectious(self);
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now INFECTIOUS for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}

void Health::become_symptomatic(Disease *disease) {
  int disease_id = disease->get_id();
  assert(active_infections[ disease_id ]);
  if (symptomatic[disease_id])
    return;
  symptomatic.set(disease_id) = true;
  Person * self = Global::Pop.get_person_by_index( person_index );
  disease->become_symptomatic(self);
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now SYMPTOMATIC for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}


void Health::recover(Disease * disease) {
  int disease_id = disease->get_id();
  assert(active_infections[ disease_id ]);
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now RECOVERED for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
  become_removed(disease_id);
  recovered_today.set(disease_id);
}

void Health::become_removed(int disease_id) {
  Disease * disease = Global::Pop.get_disease(disease_id);
  Person * self = Global::Pop.get_person_by_index( person_index );
  disease->become_removed(self,susceptible[disease_id],
      infectious[disease_id],
      symptomatic[disease_id]);
  susceptible[disease_id] = false;
  infectious[disease_id] = false;
  symptomatic[disease_id] = false;
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now REMOVED for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}

void Health::become_immune(Disease *disease) {
  int disease_id = disease->get_id();
  Person * self = Global::Pop.get_person_by_index( person_index );
  disease->become_immune(self,susceptible[disease_id],
      infectious[disease_id],
      symptomatic[disease_id]);
  susceptible[disease_id] = false;
  infectious[disease_id] = false;
  symptomatic[disease_id] = false;
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "person %d is now IMMUNE for disease %d\n",
        get_person_id(), disease_id);
    fflush(Global::Statusfp);
  }
}

void Health::update(int day) {
  // if deceased, health status should have been cleared during population
  // update (by calling Person->die(), then Health->die(), which will reset (bool) alive
  if ( !( alive ) ) { return; }
  // set disease-specific flags in bitset to detect calls to recover()
  recovered_today.reset();
  // if any disease has an active infection, then loop through and check
  // each disease infection
  if ( active_infections.any() ) {
    for (int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
      // update the infection (if it exists)
      // the check if agent has symptoms is performed by Infection->update (or one of the
      // methods called by it).  This sets the relevant symptomatic flag used by 'is_symptomatic()'
      if ( active_infections[ disease_id ] == true ) {
        infection[disease_id]->update(day);
        // This can only happen if the infection[disease_id] exists.
        // If the infection_update called recover(), it is now safe to
        // collect the susceptible date and delete the Infection object
        if (recovered_today[disease_id]) {
          susceptible_date[disease_id] = infection[disease_id]->get_susceptible_date();
          evaluate_susceptibility.set(disease_id);
          if(infection[disease_id]->provides_immunity()){
            past_infections[disease_id].push_back( Past_Infection(infection[disease_id]) );
          }
          delete infection[disease_id];
          active_infections.reset( disease_id );
          infection[disease_id] = NULL;
        }
      }
    }
  }
  // First check to see if we need to evaluate susceptibility
  // for any diseases; if so check for susceptibility due to loss of immunity
  // The evaluate_susceptibility bit for that disease will be reset in the
  // call to become_susceptible
  if ( evaluate_susceptibility.any() ) {
    for (int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
      if (day == susceptible_date[disease_id]) {
        become_susceptible(disease_id);
      }
    }
  }
  else if ( active_infections.none() ) {
    // no active infections, no need to evaluate susceptibility so we no longer
    // need to update this Person's Health
    Global::Pop.clear_mask_by_index( fred::Update_Health, person_index );
  }
} // end Health::update //

void Health::update_interventions(int day) {
  // if deceased, health status should have been cleared during population
  // update (by calling Person->die(), then Health->die(), which will reset (bool) alive
  if ( !( alive ) ) { return; }
  
  Person * self = Global::Pop.get_person_by_index( person_index );
  
  if ( intervention_flags.any() ) {
    // update vaccine status
    if (intervention_flags[ takes_vaccine ]) {
      int size = (int) vaccine_health.size();
      for (int i = 0; i < size; i++)
        vaccine_health[i]->update(day, self->get_age());
    }
    // update antiviral status
    if ( intervention_flags[ takes_av ] ) {
      for ( av_health_itr i = av_health.begin(); i != av_health.end(); ++i ) {
        ( *i )->update(day);
      }
    }
  }
} // end Health::update_interventions

void Health::declare_at_risk(Disease* disease) {
  int disease_id = disease->get_id();
  at_risk[disease_id] = true;
}

int Health::get_exposure_date(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return -1;
  else
    return infection[disease_id]->get_exposure_date();
}

int Health::get_infectious_date(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return -1;
  else
    return infection[disease_id]->get_infectious_date();
}

int Health::get_recovered_date(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return -1;
  else
    return infection[disease_id]->get_recovery_date();
}

int Health:: get_symptomatic_date(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return -1;
  else
    return infection[disease_id]->get_symptomatic_date();
}

Person * Health::get_infector(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return NULL;
  else
    return infection[disease_id]->get_infector();
}

int Health::get_infected_place(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return -1;
  else if (infection[disease_id]->get_infected_place() == NULL)
    return -1;
  else
    return infection[disease_id]->get_infected_place()->get_id();
}

char Health::get_infected_place_type(int disease_id) const {
  if (!( active_infections[ disease_id ] ))
    return 'X';
  else if (infection[disease_id]->get_infected_place() == NULL)
    return 'X';
  else
    return infection[disease_id]->get_infected_place()->get_type();
}

char * Health::get_infected_place_label(int disease_id) const {
  if (!( active_infections[ disease_id ] )) {
    strcpy(dummy_label, "-");
    return dummy_label;
  } else if (infection[disease_id]->get_infected_place() == NULL) {
    strcpy(dummy_label, "X");
    return dummy_label;
  } else
    return infection[disease_id]->get_infected_place()->get_label();
}

int Health::get_infectees(int disease_id) const {
  return infectee_count[disease_id];
}

double Health::get_susceptibility(int disease_id) const {
  double suscep_multp = susceptibility_multp[disease_id];

  if (!( active_infections[ disease_id ] ))
    return suscep_multp;
  else
    return infection[disease_id]->get_susceptibility() * suscep_multp;
}

double Health::get_infectivity(int disease_id, int day) const {
  if (!( active_infections[ disease_id ] )) {
    return 0.0;
  }
  else {
    return infection[disease_id]->get_infectivity(day);
  }
}

//Modify Operators
void Health::modify_susceptibility(int disease_id, double multp) {
  if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " susceptibility for disease "
    << disease_id << " by " << multp << "\n";

  susceptibility_multp[disease_id] *= multp;
}

void Health::modify_infectivity(int disease_id, double multp) {
  if (active_infections[ disease_id ]) {
    if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " infectivity for disease " << disease_id
      << " by " << multp << "\n";

    infection[disease_id]->modify_infectivity(multp);
  }
}

void Health::modify_infectious_period(int disease_id, double multp, int cur_day) {
  if (active_infections[ disease_id ]) {
    if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " infectivity for disease " << disease_id
      << " by " << multp << "\n";

    infection[disease_id]->modify_infectious_period(multp, cur_day);
  }
}

void Health::modify_asymptomatic_period(int disease_id, double multp, int cur_day) {
  if (active_infections[ disease_id ]) {
    if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " asymptomatic period  for disease " << disease_id
      << " by " << multp << "\n";

    infection[disease_id]->modify_asymptomatic_period(multp, cur_day);
  }
}

void Health::modify_symptomatic_period(int disease_id, double multp, int cur_day) {
  if (active_infections[ disease_id ]) {
    if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " symptomatic period  for disease " << disease_id
      << " by " << multp << "\n";

    infection[disease_id]->modify_symptomatic_period(multp, cur_day);
  }
}

void Health::modify_develops_symptoms(int disease_id, bool symptoms, int cur_day) {
  if (active_infections[ disease_id ] &&
      ((infection[disease_id]->is_infectious() && !infection[disease_id]->is_symptomatic()) ||
       !infection[disease_id]->is_infectious())) {
    if(Global::Debug > 2) cout << "Modifying Agent " << get_person_id() << " symptomaticity  for disease " << disease_id
      << " to " << symptoms << "\n";

    infection[disease_id]->modify_develops_symptoms(symptoms, cur_day);
    symptomatic[disease_id] = true;
  }
}

//Medication operators
void Health::take(Vaccine* vaccine, int day, Vaccine_Manager* vm) {
  // Compliance will be somewhere else
  Person * self = Global::Pop.get_person_by_index( person_index );
  int age = self->get_age();
  // Is this our first dose?
  Vaccine_Health* vaccine_health_for_dose = NULL;

  for(unsigned int ivh = 0; ivh < vaccine_health.size(); ivh++) {
    if(vaccine_health[ivh]->get_vaccine() == vaccine) {
      vaccine_health_for_dose = vaccine_health[ivh];
    }
  }

  if(vaccine_health_for_dose == NULL) { // This is our first dose of this vaccine
    vaccine_health.push_back(new Vaccine_Health(day,vaccine,age,this,vm));
    intervention_flags[ takes_vaccine ] = true;
  } else { // Already have a dose, need to take the next dose
    vaccine_health_for_dose->update_for_next_dose(day,age);
  }

  if (Global::VaccineTracefp != NULL) {
    fprintf(Global::VaccineTracefp," id %7d vaccid %3d",
        get_person_id(),vaccine_health[vaccine_health.size()-1]->get_vaccine()->get_ID());
    vaccine_health[vaccine_health.size()-1]->printTrace();
    fprintf(Global::VaccineTracefp,"\n");
  }

  return;
}

void Health::take(Antiviral* av, int day) {
  av_health.push_back(new AV_Health(day,av,this));
  intervention_flags[ takes_av ] = true;
  return;
}

bool Health::is_on_av_for_disease(int day, int d) const {
  for (unsigned int iav = 0; iav < av_health.size(); iav++)
    if (av_health[iav]->get_disease() == d && av_health[iav]->is_on_av(day))
      return true;
  return false;
}


void Health::infect( Person *infectee, int disease_id, Transmission *transmission ) {
  // 'infect' call chain:
  // Person::infect => Health::infect => Infection::transmit [Create transmission
  // and expose infectee]
  Disease * disease = Global::Pop.get_disease( disease_id );
  infection[ disease_id ]->transmit( infectee, transmission );
  
  #pragma omp atomic
  ++( infectee_count[ disease_id ] );

  disease->increment_infectee_count( infection[disease_id]->get_exposure_date() );

  FRED_STATUS( 1, "person %d infected person %d infectees = %d\n",
        get_person_id(), infectee->get_id(), infectee_count[disease_id] );
}

void Health::terminate() {
  for ( int disease_id = 0; disease_id < Global::Diseases; ++disease_id ) {
    become_removed( disease_id );
  }
}

Person * Health::get_self() {
  return Global::Pop.get_person_by_index( person_index );
}

int Health::get_person_id() {
  return Global::Pop.get_person_by_index( person_index )->get_id();
}

