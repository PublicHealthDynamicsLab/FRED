/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: AV_Manager.cpp
//

#include "Global.h"
#include "Manager.h"
#include "AV_Manager.h"
#include "Policy.h"
#include "AV_Policies.h"
#include "Population.h"
#include "Antivirals.h"
#include "Antiviral.h"
#include "Params.h"
#include "Person.h"
#include "Tracker.h"

AV_Manager::AV_Manager() {
  this->pop = NULL;
  this->are_policies_set = false;
  this->do_av = false;
  this->overall_start_day = -1;
  this->av_package = NULL;
  this->current_av = NULL;
}

AV_Manager::AV_Manager(Population *_pop) : Manager(_pop){
  this->pop = _pop;
  this->are_policies_set = false;
  //char s[80];
  int nav;
  Params::get_param_from_string("number_antivirals", &nav);
  
  this->do_av = false;
  if(nav > 0){
    this->do_av = true;
    this->av_package = new Antivirals();
    
    // Gather relavent Input Parameters
    //overall_start_day = 0;
    //if(does_param_exist(s))
    //  get_param(s,&overall_start_day);
    
    // Need to fill the AV_Manager Policies
    this->policies.push_back(new AV_Policy_Distribute_To_Symptomatics(this));
    this->policies.push_back(new AV_Policy_Distribute_To_Everyone(this));
    
    // Need to run through the Antivirals and give them the appropriate policy
    set_policies();
  } else {
    this->overall_start_day = -1;
  }
}

void AV_Manager::update(int day){
  if(this->do_av){
    this->av_package->update(day);
    if(Global::Debug > 1){
      this->av_package->print_stocks();
    }
  }
}

void AV_Manager::reset(){
  if(this->do_av) {
    this->av_package->reset();
  }
}

void AV_Manager::print(){
  if(this->do_av) {
    this->av_package->print();
  }
}      

void AV_Manager::set_policies(){
  vector < Antiviral* > avs = this->av_package->get_AV_vector();
  for(unsigned int iav = 0;iav<avs.size();iav++){
    if(avs[iav]->is_prophylaxis()){
      avs[iav]->set_policy(this->policies[AV_POLICY_GIVE_EVERYONE]);
    } else {
      avs[iav]->set_policy(this->policies[AV_POLICY_PERCENT_SYMPT]);
    }
  }
  this->are_policies_set = true;
}

void AV_Manager::disseminate(int day) {
  // There is no queue, only the whole population
  if(!this->do_av) {
    return;
  }
  int num_avs = 0;
  //current_day = day;
  // The av_package are in a priority based order, so lets loop over the av_package first
  vector<Antiviral*> avs = this->av_package->get_AV_vector();
  for(unsigned int iav = 0; iav < avs.size(); iav++) {
    Antiviral * av = avs[iav];
    if(av->get_current_stock() > 0) {
      // Each AV has its one policy
      Policy *p = av->get_policy();

      this->current_av = av;

      // loop over the entire population
      for(int ip = 0; ip < this->pop->get_index_size(); ++ip) {
        if(av->get_current_stock() == 0) {
          break;
        }
        Person * current_person = this->pop->get_person_by_index(ip);
	if (current_person != NULL) {
	  // Should the person get an av
	  //int yeah_or_ney = p->choose(current_person,av->get_disease(),day);
	  //if(yeah_or_ney == 0){

	  if(p->choose_first_negative(current_person, av->get_disease(), day) == true) {
	    if(Global::Debug > 3)
	      cout << "Giving Antiviral for disease " << av->get_disease() << " to " << ip << "\n";
	    av->remove_stock(1);
	    current_person->get_health()->take(av, day);
	    num_avs++;
	  }
	}
      }
    }
  }
  Global::Daily_Tracker->set_index_key_pair(day,"Av", num_avs);
}


