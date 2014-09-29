/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


//
//
// File: Decision.cpp
//

#include "Policy.h"
#include "Decision.h"

Decision::Decision(){
  name = "";
  type = "";
  policy = NULL;
}

Decision::~Decision(){ }

Decision::Decision(Policy *p){
  policy = p;
  name = "Generic Decision";
  type = "Generic";
}


