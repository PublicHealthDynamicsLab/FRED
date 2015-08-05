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
// #include "FixedIntraHost.h"
#include "Utils.h"
// #include "RSAIntraHost.h"
// #include "ODEIntraHost.h"
#include "HIV_Intrahost.h"

using namespace std;

void Natural_History::setup(Disease *disease) {
  this->disease = disease;
}

Natural_History *Natural_History::newNatural_History(int type) {

  switch(type) {
  case 0:
    return new Binary_Natural_History;
    
    /*
  case 1:
    return new FixedNatural_History;
    
  case 2:
    return new ODENatural_History;
    
  case 3:
    return new RSANatural_History;
    
  case 4:
    return new HIV_Natural_History;
    */

  default:
    FRED_WARNING("Unknown Natural_History type (%d) supplied to Natural_History factory.  Using Binary_Natural_History.\n", type);
    return new Binary_Natural_History;
  }
}

int Natural_History::get_days_symp() {
  return 0;
}


