/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>

#include "Past_Infection.h"
#include "Disease.h"
#include "Strain.h"
#include "Person.h"

using namespace std;

Past_Infection :: Past_Infection() { }

Past_Infection::Past_Infection( int _strain_id, int _recovery_date, int _age_at_exposure ) {
  strain_id = _strain_id;
  recovery_date = _recovery_date;
  age_at_exposure = _age_at_exposure;
}

int Past_Infection::get_strain() {
  return strain_id;
}

void Past_Infection::report () {
  printf( "DEBUG %d %d %d\n",
      recovery_date, age_at_exposure, strain_id );
}

string Past_Infection :: format_header() {
  return "# person_id disease_id recovery_date age_at_exposure strain_id\n";
}
