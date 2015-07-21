/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "HIV_Intrahost.h"
#include "Disease.h"
#include "Intrahost.h"

HIV_IntraHost::HIV_IntraHost() {
}

HIV_IntraHost::~HIV_IntraHost() {
}

void HIV_IntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);
  int id = disease->get_id();
}



