/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File HIV_Epidemic.cc

#include "HIV_Epidemic.h"
#include "HIV_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Utils.h"

HIV_Epidemic::HIV_Epidemic(Disease* _disease) :
  Epidemic(_disease) {
}


void HIV_Epidemic::setup() {

  Epidemic::setup();

  // initialize HIV specific-variables here:

}


void HIV_Epidemic::update(int day) {

  Epidemic::update(day);

  // put daily updates here:

}


void HIV_Epidemic::report_disease_specific_stats(int day) {

  // put values that should appear in outfile here:

  // this is just an example for testing:
  int hiv_count = day;
  track_value(day, (char*)"HIV", hiv_count);

  // print additional daily output here:

}

void HIV_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("HIV_Epidemic finished\n");

}


