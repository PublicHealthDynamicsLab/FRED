/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Hospital.cc
//

#include <math.h>
#include <string>
#include <sstream>

#include "Condition.h"
#include "Condition_List.h"
#include "Global.h"
#include "Hospital.h"
#include "Params.h"
#include "Person.h"
#include "Place_List.h"
#include "Random.h"

//Private static variables that will be set by parameter lookups
double Hospital::contacts_per_day;
double** Hospital::prob_transmission_per_contact;
std::vector<double> Hospital::Hospital_health_insurance_prob;

Hospital::Hospital() : Place() {
  this->set_type(Place::TYPE_HOSPITAL);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->bed_count = 0;
  this->occupied_bed_count = 0;
  this->daily_patient_capacity = -1;
  this->current_daily_patient_count = 0;
  this->add_capacity = false;

  if(Global::Enable_Health_Insurance) {
    vector<double>::iterator itr;
    int insr = static_cast<int>(Insurance_assignment_index::PRIVATE);
    for(itr = Hospital::Hospital_health_insurance_prob.begin(); itr != Hospital::Hospital_health_insurance_prob.end(); ++itr, ++insr) {
      set_accepts_insurance(insr, (Random::draw_random() < *itr));
    }
  }
}

Hospital::Hospital(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_HOSPITAL);
  this->set_subtype(_subtype);
  this->bed_count = 0;
  this->occupied_bed_count = 0;
  this->daily_patient_capacity = -1;
  this->current_daily_patient_count = 0;
  this->add_capacity = false;

  if(Global::Enable_Health_Insurance) {
    vector<double>::iterator itr;
    int insr = static_cast<int>(Insurance_assignment_index::PRIVATE);
    for(itr = Hospital::Hospital_health_insurance_prob.begin(); itr != Hospital::Hospital_health_insurance_prob.end(); ++itr, ++insr) {
      set_accepts_insurance(insr, (Random::draw_random() < *itr));
    }
  }
}

void Hospital::get_parameters() {
  Params::get_param("hospital_contacts", &Hospital::contacts_per_day);
  int n = Params::get_param_matrix((char *)"hospital_trans_per_contact", &Hospital::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nHospital contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Hospital::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  if(Global::Enable_Health_Insurance) {
    Params::get_param_vector((char*)"hospital_health_insurance_prob", Hospital::Hospital_health_insurance_prob);
    assert(static_cast<int>(Hospital::Hospital_health_insurance_prob.size()) == static_cast<int>(Insurance_assignment_index::UNSET));
  }
}

int Hospital::get_group(int condition, Person* per) {
  // 0 - Healthcare worker
  // 1 - Patient
  // 2 - Visitor
  if(per->get_activities()->is_hospital_staff() && !per->is_hospitalized()) {
    return 0;
  }

  Place* hosp = per->get_activities()->get_hospital();
  if(hosp != NULL && hosp->get_id() == this->get_id()) {
    return 1;
  } else {
    return 2;
  }
}

double Hospital::get_transmission_prob(int condition, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition, i);
  int col = get_group(condition, s);
  double tr_pr = Hospital::prob_transmission_per_contact[row][col];
  return tr_pr;
}

int Hospital::get_bed_count(int sim_day) {
  return this->bed_count;
}

int Hospital::get_daily_patient_capacity(int sim_day) {
  return this->daily_patient_capacity;
}

double Hospital::get_contacts_per_day(int condition) {
  return Hospital::contacts_per_day;
}

bool Hospital::is_open(int sim_day) {
  bool open = (sim_day < this->close_date || this->open_date <= sim_day);
  return open;
}

bool Hospital::should_be_open(int sim_day) {
  return is_open(sim_day);
}

bool Hospital::should_be_open(int sim_day, int condition) {
  return is_open(sim_day);
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

void Hospital::set_accepts_insurance(int insr_indx, bool does_accept) {
  assert(insr_indx >= 0);
  assert(insr_indx < static_cast<int>(Insurance_assignment_index::UNSET));
  switch(insr_indx) {
    case static_cast<int>(Insurance_assignment_index::PRIVATE):
      set_accepts_insurance(Insurance_assignment_index::PRIVATE, does_accept);
      break;
    case static_cast<int>(Insurance_assignment_index::MEDICARE):
      set_accepts_insurance(Insurance_assignment_index::MEDICARE, does_accept);
      break;
    case static_cast<int>(Insurance_assignment_index::MEDICAID):
      set_accepts_insurance(Insurance_assignment_index::MEDICAID, does_accept);
      break;
    case static_cast<int>(Insurance_assignment_index::HIGHMARK):
      set_accepts_insurance(Insurance_assignment_index::HIGHMARK, does_accept);
      break;
    case static_cast<int>(Insurance_assignment_index::UPMC):
      set_accepts_insurance(Insurance_assignment_index::UPMC, does_accept);
      break;
    case static_cast<int>(Insurance_assignment_index::UNINSURED):
      set_accepts_insurance(Insurance_assignment_index::UNINSURED, does_accept);
      break;
    default:
      Utils::fred_abort("Invalid Insurance Assignment Type", "");
  }
}

std::string Hospital::to_string() {
  std::stringstream ss;
  ss << "Hospital[" << this->get_label() << "]: bed_count: " << this->bed_count
     << ", occupied_bed_count: " << this->occupied_bed_count
     << ", daily_patient_capacity: " << this->daily_patient_capacity
     << ", current_daily_patient_count: " << this->current_daily_patient_count
     << ", add_capacity: " << this->add_capacity
     << ", subtype: " << this->get_subtype();

  return ss.str();
}


