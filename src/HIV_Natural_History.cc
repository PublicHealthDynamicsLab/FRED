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
  int id = disease->get_id();
}



