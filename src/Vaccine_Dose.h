/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: VaccineDose.h
//

#ifndef _FRED_VACCINEDOSE_H
#define _FRED_VACCINEDOSE_H

#include "Global.h"
#include "Age_Map.h"

class Vaccine_Dose {
  // Vaccine_Dose is a class to hold the efficacy and dosing information 
  // for a vaccine.  A vaccine may have as many doses as it needs.
public:
  // Creation Operators
  Vaccine_Dose(Age_Map* _efficacy, Age_Map* _efficacy_delay, int _days_between_doses);
  ~Vaccine_Dose();
  
  //Parameter Access
  Age_Map* get_efficacy_map()       const { return efficacy;}
  Age_Map* get_efficacy_delay_map() const { return efficacy_delay;}
  
  double  get_efficacy(int age)         const { return efficacy->find_value(age);  }
  double  get_efficacy_delay(int age)   const { return efficacy_delay->find_value(age); }
  int     get_days_between_doses()  const { return days_between_doses; }
  
  bool    is_within_age(int age) const;
  
  //Utility Functions... no need for update or reset.
  void print() const;
  
private:
  int days_between_doses;       // Number of days until the next dose is administered
  Age_Map* efficacy;            // Age specific efficacy of vaccine, does the dose provide immunity
  Age_Map* efficacy_delay;      // Age specific delay to efficacy, how long does it take to develop immunity
  
protected:
  Vaccine_Dose() { }
};

#endif




