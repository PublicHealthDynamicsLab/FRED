/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File HIV_Infection.cc

#include "HIV_Infection.h"
#include "HIV_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"

HIV_Infection::HIV_Infection(Disease* _disease, Person* _infector, Person* _host, Place* _place, int day) :
  Infection(disease, _infector, _host, _place, day) {

  // initialize HIV specific-variables here:

}
