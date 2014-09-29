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
// File: Past_Infection.h
//

#ifndef _FRED_PAST_INFECTION_H
#define _FRED_PAST_INFECTION_H

#include <stdio.h>
#include <vector>
#include <iostream>

#include "Global.h"

class Disease;
class Person;

using namespace std;

class Past_Infection {

public:
  Past_Infection();
  Past_Infection( int _strain_id, int _recovery_date, int _age_at_exposure );

  int get_strain();
  int get_recovery_date() { return recovery_date; }
  int get_age_at_exposure() { return age_at_exposure; }
  void report();
  static string format_header();

private:
  int strain_id;
  short int recovery_date;
  short int age_at_exposure;
};

#endif
