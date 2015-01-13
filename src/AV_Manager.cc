/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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

AV_Manager::AV_Manager() {
  pop = NULL;
  are_policies_set = false;
  do_av = false;
  overall_start_day = -1;
}

AV_Manager::AV_Manager(Population *_pop) : Manager(_pop){
  pop = _pop;
  are_policies_set = false;
  //char s[80];
  int nav;
  Params::get_param_from_string("number_antivirals",&nav);
  
  do_av=0;
  if(nav > 0){
    do_av = 1;
    av_package = new Antivirals();
    
    // Gather relavent Input Parameters
    //overall_start_day = 0;
    //if(does_param_exist(s))
    //  get_param(s,&overall_start_day);
    
    // Need to fill the AV_Manager Policies
    policies.push_back(new AV_Policy_Distribute_To_Symptomatics(this)); 
    policies.push_back(new AV_Policy_Distribute_To_Everyone(this));
    
    // Need to run through the Antivirals and give them the appropriate policy
    set_policies();
  }
  else{
    overall_start_day = -1;
  }
}

void AV_Manager::update(int day){
  if(do_av==1){
    av_package->update(day);
    if(Global::Debug > 1){
      av_package->print_stocks();
    }
  }
}

void AV_Manager::reset(){
  if(do_av==1){
    av_package->reset();
  }
}

void AV_Manager::print(){
  if(do_av == 1)
    av_package->print();
}      

void AV_Manager::set_policies(){
  vector < Antiviral* > avs = av_package->get_AV_vector();
  for(unsigned int iav = 0;iav<avs.size();iav++){
    if(avs[iav]->is_prophylaxis()){
      avs[iav]->set_policy(policies[AV_POLICY_GIVE_EVERYONE]);
    }
    else{ 
      avs[iav]->set_policy(policies[AV_POLICY_PERCENT_SYMPT]);
    }
  }
  are_policies_set = true;
}

void AV_Manager::disseminate(int day){
  // There is no queue, only the whole population
  if(do_av==0) return;
  int npeople = pop->get_pop_size();
  //current_day = day;
  // The av_package are in a priority based order, so lets loop over the av_package first
  vector < Antiviral* > avs = av_package->get_AV_vector();
  for(unsigned int iav = 0; iav<avs.size(); iav++){
    Antiviral * av = avs[iav];
    if(av->get_current_stock() > 0){
      // Each AV has its one policy
      Policy *p = av->get_policy();
      
      current_av = av;

      for(int ip=0;ip<npeople;ip++){
        if(av->get_current_stock()== 0) break;
        Person* current_person = pop->get_person_by_index(ip);
        // Should the person get an av
  //int yeah_or_ney = p->choose(current_person,av->get_disease(),day);
  //if(yeah_or_ney == 0){
  if(p->choose_first_negative(current_person,av->get_disease(),day) == true){
          if(Global::Debug > 3) cout << "Giving Antiviral for disease " << av->get_disease() << " to " <<ip << "\n";
          av->remove_stock(1);
          current_person->get_health()->take(av,day);
        }
      }
    }
  }
}


