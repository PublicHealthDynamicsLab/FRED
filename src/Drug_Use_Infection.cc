/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Drug_Use_Infection.cc

#include "Drug_Use_Infection.h"
#include "Drug_Use_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Mixing_Group.h"
#include "Utils.h"

Drug_Use_Infection::Drug_Use_Infection(Disease* _disease, Person* _infector, Person* _host, Mixing_Group* _mixing_group, int day) :
  Infection(_disease, _infector, _host, _mixing_group, day) {
}

void Drug_Use_Infection::setup() {

  Infection::setup();

  // initialize Drug_Use specific-variables here:

}

double Drug_Use_Infection::get_infectivity(int day) {
  return (is_infectious(day) ? 1.0 : 0.0);
}

double Drug_Use_Infection::get_symptoms(int day) {
  return (is_symptomatic(day) ? 1.0 : 0.0);
}

bool Drug_Use_Infection::is_fatal(int day) {
  return false; 
}

void Drug_Use_Infection::update(int day) {

  FRED_VERBOSE(1, "update Drug_Use INFECTION on day %d for host %d\n", day, host->get_id());

  // put daily update here
}
