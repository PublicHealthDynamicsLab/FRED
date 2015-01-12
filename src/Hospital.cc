/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Hospital.cc
//

#include "Hospital.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"

//Private static variables that will be set by parameter lookups
double* Hospital::Hospital_contacts_per_day;
double*** Hospital::Hospital_contact_prob;

//Private static variable to assure we only lookup parameters once
bool Hospital::Hospital_parameters_set = false;

Hospital::Hospital() {
  this->type = Place::HOSPITAL;
  this->bed_count = 0;
  get_parameters(Global::Diseases);
}

Hospital::Hospital(const char* lab, fred::place_subtype _subtype, double lon, double lat, Place* container, Population* pop) {
  this->type = Place::HOSPITAL;
  this->subtype = _subtype;
  this->bed_count = 0;
  setup(lab, lon, lat, container, pop);
  get_parameters(Global::Diseases);
}

void Hospital::get_parameters(int diseases) {
  char param_str[80];
  
  if(Hospital::Hospital_parameters_set) {
    return;
  }
  
  Hospital::Hospital_contacts_per_day = new double[diseases];
  Hospital::Hospital_contact_prob = new double**[diseases];
  
  for(int s = 0; s < diseases; ++s) {
    int n;
    sprintf(param_str, "hospital_contacts[%d]", s);
    Params::get_param((char*) param_str, &Hospital::Hospital_contacts_per_day[s]);
    
    sprintf(param_str, "hospital_prob[%d]", s);
    n = 0;
    Params::get_param((char*) param_str, &n);
    if(n) {
      double* tmp;
      tmp = new double [n];
      Params::get_param_vector((char*) param_str, tmp);
      n = (int)sqrt((double) n);
      Hospital::Hospital_contact_prob[s] = new double* [n];
      for(int i  = 0; i < n; ++i) {
        Hospital::Hospital_contact_prob[s][i] = new double[n];
      }
      for(int i  = 0; i < n; ++i) {
        for(int j  = 0; j < n; ++j) {
          Hospital::Hospital_contact_prob[s][i][j] = tmp[i*n+j];
        }
      }
      delete tmp;
      
      if (Global::Verbose > 1) {
        printf("\nHospital_contact_prob:\n");
        for(int i  = 0; i < n; ++i)  {
          for(int j  = 0; j < n; ++j) {
            printf("%f ", Hospital::Hospital_contact_prob[s][i][j]);
          }
          printf("\n");
        }
      }
    }
  }
  
  Hospital::Hospital_parameters_set = true;
}

int Hospital::get_group(int disease, Person* per) {
  return 0;
}

double Hospital::get_transmission_prob(int disease, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Hospital::Hospital_contact_prob[disease][row][col];
  return tr_pr;
}

double Hospital::get_contacts_per_day(int disease) {
  return Hospital::Hospital_contacts_per_day[disease];
}

void Hospital::set_accepts_insurance(Insurance_assignment_index::e insr, bool does_accept) {
  assert(insr >= Insurance_assignment_index::PRIVATE);
  assert(insr < Insurance_assignment_index::UNSET);
  switch(insr) {
    case Insurance_assignment_index::PRIVATE:
      this->accepted_insurance_bitset[Insurance_assignment_index::PRIVATE] = does_accept;
      break;
    case Insurance_assignment_index::MEDICARE:
      this->accepted_insurance_bitset[Insurance_assignment_index::MEDICARE] = does_accept;
      break;
    case Insurance_assignment_index::MEDICAID:
      this->accepted_insurance_bitset[Insurance_assignment_index::MEDICAID] = does_accept;
      break;
    case Insurance_assignment_index::HIGHMARK:
      this->accepted_insurance_bitset[Insurance_assignment_index::HIGHMARK] = does_accept;
      break;
    case Insurance_assignment_index::UPMC:
      this->accepted_insurance_bitset[Insurance_assignment_index::UPMC] = does_accept;
      break;
    case Insurance_assignment_index::UNINSURED:
      this->accepted_insurance_bitset[Insurance_assignment_index::UNINSURED] = does_accept;
      break;
    default:
      Utils::fred_abort("Invalid Insurance Assignment Type", "");
  }
}






