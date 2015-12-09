/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Drug_Use_Epidemic.cc

#include "Drug_Use_Epidemic.h"
#include "Drug_Use_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Utils.h"

Drug_Use_Epidemic::Drug_Use_Epidemic(Disease* _disease) :
  Epidemic(_disease) {
}


void Drug_Use_Epidemic::setup() {

  Epidemic::setup();

  // initialize Drug_Use specific-variables here:

}


void Drug_Use_Epidemic::update(int day) {

  Epidemic::update(day);

  // put daily updates here:

}


void Drug_Use_Epidemic::report_disease_specific_stats(int day) {

  // put values that should appear in outfile here:

  // this is just an example for testing:
  int user_count = day;
  track_value(day, (char*)"Drug_Use", user_count);

  // print additional daily output here:

}

void Drug_Use_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Drug_Use_Epidemic finished\n");

}


