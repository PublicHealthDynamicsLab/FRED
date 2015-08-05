/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "Natural_History.h"
#include "Binary_Natural_History.h"
#include "HIV_Natural_History.h"
#include "Utils.h"

using namespace std;

void Natural_History::setup(Disease *disease) {
  this->disease = disease;
}

Natural_History * Natural_History::get_natural_history(char* natural_history_model) {

  if (strcmp(natural_history_model, "binary") == 0) {
    return new Binary_Natural_History;
  }
    
  if (strcmp(natural_history_model, "HIV") == 0) {
    return new HIV_Natural_History;
  }

  FRED_WARNING("Unknown natural_history_model (%s)-- using Binary_Natural_History.\n", natural_history_model);
  return new Binary_Natural_History;
}


int Natural_History::get_days_symp() {
  return 0;
}


