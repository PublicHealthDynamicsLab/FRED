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
// File: VaccineStatus.cc
//
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "Random.h"
#include "Vaccine_Health.h"
#include "Vaccine.h"
#include "Vaccine_Dose.h"
#include "Vaccine_Manager.h"
#include "Health.h"
#include "Person.h"
#include "Global.h"

Vaccine_Health::Vaccine_Health(int _vaccination_day, Vaccine* _vaccine, int _age, 
             Person * _person, Vaccine_Manager* _vaccine_manager){
  
  vaccine               = _vaccine;
  vaccination_day       = _vaccination_day;
  person                = _person;
  double efficacy       = vaccine->get_dose(0)->get_efficacy(_age);
  double efficacy_delay = vaccine->get_dose(0)->get_efficacy_delay(_age);
  vaccine_manager       = _vaccine_manager;
  
  vaccination_effective_day = -1;
  if(RANDOM() < efficacy)
    vaccination_effective_day = vaccination_day + efficacy_delay;
  current_dose =0;
  days_to_next_dose = -1;
  if(Global::Debug > 1) {
    cout << "Agent: " << person->get_id() << " took dose " << current_dose << " on day "<< vaccination_day << "\n";
  }
  if(vaccine->get_number_doses() > 1){
    days_to_next_dose = vaccination_day + vaccine->get_dose(0)->get_days_between_doses();
  }
  
}

void Vaccine_Health::print() const {
  // Need to make work :)
  cout << "\nVaccine_Status";
}

void Vaccine_Health::printTrace() const {
  fprintf(Global::VaccineTracefp," vaccday %5d age %3d iseff %2d effday %5d currentdose %3d",vaccination_day,
    person->get_age(),is_effective(), vaccination_effective_day, current_dose);
  fflush(Global::VaccineTracefp);
}

void Vaccine_Health::update(int day, int age){
  // First check for immunity 
  if (is_effective() && (day == vaccination_effective_day)) {
    // Going out to Person, so that activities can be accessed
    Disease* s = Global::Pop.get_disease(0);
    person->become_immune(s);
    effective = true;
    if(Global::Debug < 1) {
      cout << "Agent " << person->get_id() 
     << " has become immune from dose "<< current_dose 
     << "on day " << day << "\n";
    }
  }
  
  // Next check on dose
  // Even immunized people get another dose
  // If they are to get another dose, then put them on the queue based on dose priority
 
  if (current_dose < vaccine->get_number_doses()-1) {   // Are we done with the dosage?
    // Get the dosage policy from the manager
    if(day >= days_to_next_dose){
      current_dose++;
      days_to_next_dose = day + vaccine->get_dose(current_dose)->get_days_between_doses();
      int vaccine_dose_priority = vaccine_manager->get_vaccine_dose_priority();
      if(Global::Debug < 1){
  cout << "Agent " << person->get_id()
       << " being put in to the queue with priority " << vaccine_dose_priority
       << " for dose " << current_dose 
       << " on day " << day << "\n";
      }
      switch(vaccine_dose_priority){
      case VACC_DOSE_NO_PRIORITY:
  vaccine_manager->add_to_regular_queue_random(person);
  break;
      case VACC_DOSE_FIRST_PRIORITY:
  vaccine_manager->add_to_priority_queue_begin(person);
  break;
      case VACC_DOSE_RAND_PRIORITY:
  vaccine_manager->add_to_priority_queue_random(person);
  break;
      case VACC_DOSE_LAST_PRIORITY:
  vaccine_manager->add_to_priority_queue_end(person);
  break;
  
      }
    }
  }
}

void Vaccine_Health::update_for_next_dose(int day, int age){
  vaccination_day = day;
  if(!is_effective()){
    double efficacy = vaccine->get_dose(current_dose)->get_efficacy(age);
    double efficacy_delay = vaccine->get_dose(current_dose)->get_efficacy_delay(age);
    if (RANDOM() < efficacy)
      vaccination_effective_day = day + efficacy_delay;        
  }
}
