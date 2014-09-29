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
// File: VaccineHealth.h
//

#ifndef _FRED_VACCINEHEALTH_H
#define _FRED_VACCINEHEALTH_H

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "Random.h"
#include "Population.h"
#include "Utils.h"

class Vaccine;
class Vaccine_Dose;
class Person;
class Health;
class Vaccine_Manager;

class Vaccine_Health {
public:
  //Creation Operations
  Vaccine_Health();
  Vaccine_Health(int _vaccination_day, Vaccine* _vaccine, int _age, 
     Person * _person, Vaccine_Manager* _vaccine_manager);
  
  // Access Members
  int get_vaccination_day()              const { return vaccination_day; }
  int get_vaccination_effective_day()    const { return vaccination_effective_day; }
  int is_effective()                     const { if(vaccination_effective_day != -1) return 1; else return 0;}
  Vaccine* get_vaccine()                 const { return vaccine; }
  int get_current_dose()                 const { return current_dose; }
  int get_days_to_next_dose()            const { return days_to_next_dose; }
  Vaccine_Manager* get_vaccine_manager() const { return vaccine_manager; }
  // Modifiers
  void set_vaccination_day(int day) { 
    if(vaccination_day ==-1){
      vaccination_day = day; 
    }
    else{
      //This is an error, but it will not stop a run, only pring a Warning.
      FRED_WARNING("WARNING! Vaccination Status, setting vaccine day of someone who has already been vaccinated\n");
    }
  }
  bool isEffective() { return effective; }
  
  //Utility Functions
  void print() const;
  void printTrace() const;
  void update(int day, int age);
  void update_for_next_dose(int day, int age);
  
private:
  int vaccination_day;              // On which day did you get the vaccine
  int vaccination_effective_day;    // On which day is the vaccine effective
  Vaccine* vaccine;                 // Which vaccine did you take
  int current_dose;                 // Current Dose that the agent is on
  int days_to_next_dose;            // How long between doses
  Person * person;                  // The person object this belongs to.
  Vaccine_Manager* vaccine_manager; // Which manager did the vaccine come from?
  bool effective;
};

#endif
