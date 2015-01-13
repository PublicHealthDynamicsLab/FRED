/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Perceptions.cc
//

#include "Perceptions.h"
#include "Person.h"
#include "Disease.h"
#include "Household.h"

int Perceptions::get_neighborhood_cases(int disease) {
  Place * p = self->get_neighborhood();
  if (p == NULL) return 0.0;
  else return p->get_daily_cases(disease);
}

int Perceptions::get_neighborhood_deaths(int disease) {
  Place * p = self->get_neighborhood();
  if (p == NULL) return 0.0;
  else return p->get_daily_deaths(disease);
}

int Perceptions::get_workplace_cases(int disease) {
  int count = 0;
  Place * p = self->get_workplace();
  if (p != NULL) count = p->get_daily_cases(disease);
  return count;
}

int Perceptions::get_workplace_deaths(int disease) {
  int count = 0;
  Place * p = self->get_workplace();
  if (p != NULL) count = p->get_daily_deaths(disease);
  return count;
}

int Perceptions::get_school_cases(int disease) {
  int count = 0;
  Place * p = self->get_school();
  if (p != NULL) count = p->get_daily_cases(disease);
  return count;
}

int Perceptions::get_school_deaths(int disease) {
  int count = 0;
  Place * p = self->get_activities()->get_school();
  if (p != NULL) count = p->get_daily_deaths(disease);
  return count;
}

double Perceptions::get_household_school_incidence(int disease) {
  int count = 0;
  int total_school = 0;
  Household * house = (Household *) self->get_household();
  int hsize = house->get_size();
  for (int i = 0; i < hsize; i++) {
    Person * pp = house->get_housemate(i);
    Place * s = pp->get_school();
    if (s != NULL) { 
      count += s->get_daily_cases(disease);
      total_school += s->get_size();
    }
  }
  if (total_school == 0) return 0.0;
  return (double) count / (double) total_school;
}


