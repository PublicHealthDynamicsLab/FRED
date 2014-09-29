/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Policy.cpp
//
#include <iostream>
#include <iomanip>
#include <vector>
#include "Policy.h"
#include "Decision.h"
#include "Manager.h"

Policy::Policy() {
  Name = "";
  manager = NULL;
}

Policy::~Policy() {
  for (unsigned int i=0; i < decision_list.size(); i++)
    delete decision_list[i];      
  decision_list.clear();
}

Policy::Policy(Manager* mgr){
  Name = "Generic";
  manager = mgr;
}

bool Policy::choose_first_positive(Person* person, int disease, int current_day){
  for(unsigned int i=0; i < decision_list.size(); i++){    
    if(decision_list[i]->evaluate(person,disease,current_day) == 1) return true;
  }
  return false;
}   

bool Policy::choose_first_negative(Person* person, int disease, int current_day){
  for(unsigned int i=0; i < decision_list.size(); i++){    
    if(decision_list[i]->evaluate(person,disease,current_day) == -1) return false;
  }
  return true;
}

// Haven't used this yet, don't know if it is exactly right
int Policy::choose(Person* person, int disease, int current_day){
  int result=-1;
  for(unsigned int i=0; i < decision_list.size(); i++){    
    int new_result = decision_list[i]->evaluate(person,disease,current_day);
    if(new_result == -1) return -1;
    else if(new_result > result) result = new_result;
    //  cout <<"\nResult for decision "<<i<< " is "<< result;
  }
  return result;
}   

void Policy::print() const {
  cout << "\nPolicy List for Decision "<< Name;
  cout << "\n\n" << setw(40) << "Policy " << setw(20) << "Type";
  cout << "\n------------------------------------------------------------------\n";
  for(unsigned int i=0;i<decision_list.size(); ++i){
    cout << setw(40) << decision_list[i]->get_name() << setw(20) << decision_list[i]->get_type() << "\n";
  } 
}

void Policy::reset() { 
  cout << "Policy Reset not implemented yet";
}
