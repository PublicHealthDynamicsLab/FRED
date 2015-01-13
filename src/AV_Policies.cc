/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: AV_Policies.h
//

#include <iostream>
#include <vector>
#include "Decision.h"
#include "AV_Policies.h"
#include "AV_Decisions.h"
#include "Policy.h"
#include "Manager.h"
#include "AV_Manager.h"

AV_Policy_Distribute_To_Symptomatics::AV_Policy_Distribute_To_Symptomatics(AV_Manager* avm):
Policy(dynamic_cast <Manager *> (avm)){
  Name = "Distribute AVs to Symptomatics";
  
  // Need to add the policies in the decisio
  decision_list.push_back(new AV_Decision_Begin_AV_On_Day(this));
  decision_list.push_back(new AV_Decision_Give_to_Sympt(this));
  decision_list.push_back(new AV_Decision_Allow_Only_One(this));
  //decision_list.push_back(new AV_Decision_Give_One_Chance(this));
}

int AV_Policy_Distribute_To_Symptomatics::choose(Person* person, int disease, int day) {
  int result=-1;
  for(unsigned int i=0; i < decision_list.size(); i++){
    
    int new_result = decision_list[i]->evaluate(person, disease, day);
    if(new_result == -1) return -1;
    else if(new_result > result) result = new_result;
  }
  return result;
}   

AV_Policy_Distribute_To_Everyone::AV_Policy_Distribute_To_Everyone(AV_Manager* avm):
Policy(dynamic_cast <Manager *> (avm)){
  Name = "Distribute AVs to Symptomatics";
  
  // Need to add the policies in the decision
  decision_list.push_back(new AV_Decision_Begin_AV_On_Day(this));
  decision_list.push_back(new AV_Decision_Allow_Only_One(this));
}

int AV_Policy_Distribute_To_Everyone::choose(Person* person, int disease, int day){
  int result=-1;
  for(unsigned int i=0; i < decision_list.size(); i++){   
    int new_result = decision_list[i]->evaluate(person, disease, day);
    if(new_result == -1) { return -1; }   
    if(new_result > result) result = new_result;
  }
  return result;
}
