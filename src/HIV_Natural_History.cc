/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "HIV_Natural_History.h"
#include "Disease.h"
#include "Natural_History.h"

HIV_Natural_History::HIV_Natural_History() {
}

HIV_Natural_History::~HIV_Natural_History() {
}

void HIV_Natural_History::setup(Disease *disease) {
  Natural_History::setup(disease);
  read_hiv_files();
}

void HIV_Natural_History::read_hiv_files() {
  // Read in any file having to do with HIV natural history, such as
  // mortality rates

  // If you read in a value X here that should be accessible to
  // HIV_Infection, you should define a method in HIV_Natural_History.h
  // called get_X() that returns the value of X.  Then HIV_Infection can
  // access the value of X as follows:

  // int my_x = this->disease->get_natural_history()->get_X();

}

void HIV_Natural_History::update_infection(int day, Person* host, Infection *infection) {
}




