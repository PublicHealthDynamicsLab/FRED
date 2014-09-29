/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <vector>
#include <map>

#include "HIV_Intrahost.h"
#include "Disease.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Params.h"
#include "Random.h"

HIV_IntraHost::HIV_IntraHost() {
}

HIV_IntraHost::~HIV_IntraHost() {
}

void HIV_IntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);
  int id = disease->get_id();
}



