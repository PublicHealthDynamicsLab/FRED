/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "Evolution.h"
#include "Global.h"
#include "Infection.h"
#include "Person.h"

#include <map>

using namespace std;

void Evolution :: setup(Disease *disease) {
  this->disease = disease;
}

inline double Evolution::residual_immunity(Person *person, int challenge_strain, int day) {
  return double( !( person->get_health()->is_susceptible( disease->get_id() ) ) );
}

void Evolution::print() {}

/*
  Transmission::Loads * Evolution::get_primary_loads(int day) {
  Transmission::Loads *loads = new Transmission::Loads;
  loads->insert( pair<int, double> (1, 1) );
  return loads;
  }

  Transmission::Loads * Evolution::get_primary_loads(int day, int strain) {
  Transmission::Loads *loads = new Transmission::Loads;
  loads->insert( pair<int, double> (strain, 1) );
  return loads;
  }
*/

double Evolution::antigenic_diversity(Person *p1, Person *p2) {
  Infection *inf1 = p1->get_health()->get_infection(disease->get_id());
  Infection *inf2 = p2->get_health()->get_infection(disease->get_id());

  if(!inf1 || !inf2) return 0;

  vector<int> str1; inf1->get_strains(str1);
  vector<int> str2; inf1->get_strains(str2);

  // TODO how to handle multiple strains???

  return antigenic_distance(str1.at(0), str2.at(0));
}
