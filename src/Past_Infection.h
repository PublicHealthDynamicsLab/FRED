/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Past_Infection.h
//

#ifndef _FRED_PAST_INFECTION_H
#define _FRED_PAST_INFECTION_H

#include "Global.h"
#include <stdio.h>
#include <vector>
#include <iostream>

class Disease;
class Person;
class Infection;

using namespace std;

class Past_Infection {
public:
  Past_Infection();
  Past_Infection(Infection *infection);
  Past_Infection(vector<int> &_strains, int _recovery_date, int _age_at_exposure, Disease *_disease, Person *_host);
  void get_strains(vector<int> &strains);
  int get_recovery_date() { return recovery_date; }
  int get_age_at_exposure() { return age_at_exposure; }
  void report();
  static string format_header();

private:
  Person *host;
  Disease *disease;
  vector<int> strains;
  int recovery_date;
  int age_at_exposure;
};

#endif
