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
// File: VaccineDose.cc
//

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "Vaccine_Dose.h"

Vaccine_Dose::Vaccine_Dose(Age_Map* _efficacy, Age_Map* _efficacy_delay, 
                           int _days_between_doses){
  efficacy = _efficacy;
  efficacy_delay = _efficacy_delay;
  days_between_doses = _days_between_doses;
}

Vaccine_Dose::~Vaccine_Dose(){
  if (efficacy) delete efficacy;
  if (efficacy_delay) delete efficacy_delay;
}

void Vaccine_Dose::print() const {
  cout << "Time Between Doses:\t " << days_between_doses << "\n";
  efficacy->print();
  efficacy_delay->print();
}

bool Vaccine_Dose::is_within_age(int age) const {
  if(efficacy->find_value(age) != 0.0){
    return true;
  }
  return false;
}
