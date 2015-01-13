/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Classroom.cc
//

#include "Classroom.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"

//Private static variables that will be set by parameter lookups
double * Classroom::Classroom_contacts_per_day;
double *** Classroom::Classroom_contact_prob;
char Classroom::Classroom_closure_policy[80];
int Classroom::Classroom_closure_day = 0;
double Classroom::Classroom_closure_threshold = 0.0;
int Classroom::Classroom_closure_period = 0;
int Classroom::Classroom_closure_delay = 0;

//Private static variable to assure we only lookup parameters once
bool Classroom::Classroom_parameters_set = false;

Classroom::Classroom( const char *lab, double lon,
         double lat, Place *container, Population *pop) {
  type = CLASSROOM;
  setup( lab, lon, lat, container, pop);
  get_parameters(Global::Diseases);
  age_level = -1;
}

void Classroom::get_parameters(int diseases) {
  char param_str[80];
  
  if (Classroom::Classroom_parameters_set) return;
  
  Classroom::Classroom_contacts_per_day = new double [ diseases ];
  Classroom::Classroom_contact_prob = new double** [ diseases ];
  
  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "classroom_contacts[%d]", s);
    Params::get_param((char *) param_str, &Classroom::Classroom_contacts_per_day[s]);
    if (Classroom::Classroom_contacts_per_day[s] == -1) {
      Classroom::Classroom_contacts_per_day[s] = 2.0 * container->get_contacts_per_day(s);
    }
    
    sprintf(param_str, "classroom_prob[%d]", s);
    n = Params::get_param_matrix(param_str, &Classroom::Classroom_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nClassroom_contact_prob:\n");
      for (int i  = 0; i < n; i++)  {
        for (int j  = 0; j < n; j++) {
          printf("%f ", Classroom::Classroom_contact_prob[s][i][j]);
        }
        printf("\n");
      }
    }
  }
  
  Classroom::Classroom_parameters_set = true;
}

void Classroom::enroll(Person * per) {
  N++;
  int age = per->get_age();
  if (age_level == -1 && age < Global::ADULT_AGE) {
    age_level = age;
  }
}

int Classroom::get_group(int disease, Person * per) {
  int age = per->get_age();
  if (age < 12) { return 0; }
  else if (age < 16) { return 1; }
  else if (age < Global::ADULT_AGE) { return 2; }
  else return 3;
}

double Classroom::get_transmission_prob(int disease, Person * i, Person * s) {

  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Classroom::Classroom_contact_prob[disease][row][col];
  return tr_pr;
}

bool Classroom::should_be_open(int day, int disease) {
  return container->should_be_open(day, disease);
}

double Classroom::get_contacts_per_day(int disease) {
  return Classroom::Classroom_contacts_per_day[disease];
}

