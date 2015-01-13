
/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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


