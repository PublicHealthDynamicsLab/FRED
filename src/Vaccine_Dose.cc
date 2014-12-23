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
// File: VaccineDose.cc
//

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#include "Vaccine_Dose.h"
#include "Random.h"

Vaccine_Dose::Vaccine_Dose(Age_Map* _efficacy, Age_Map* _efficacy_delay, 
			   Age_Map* _efficacy_duration, int _days_between_doses){
  efficacy = _efficacy;
  efficacy_delay = _efficacy_delay;
  efficacy_duration = _efficacy_duration;
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
  efficacy_duration->print();
}

bool Vaccine_Dose::is_within_age(double real_age) const {
  double eff = efficacy->find_value(real_age);
  // printf("age = %.1f  eff = %f\n", real_age, eff);
  if(eff != 0.0){
    return true;
  }
  return false;
}


double Vaccine_Dose::get_duration_of_immunity(double real_age) {
  double expected_duration = efficacy_duration->find_value(real_age);
  // select a value from an exponential distribution with mean expected_duration
  double actual_duration = 0.0;
  if (expected_duration > 0.0) {
    actual_duration = draw_exponential(1.0 / expected_duration);
  }
  return actual_duration;
}
