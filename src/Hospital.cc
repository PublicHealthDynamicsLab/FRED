/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Hospital.cc
//

#include <math.h>
#include <string>
#include <sstream>

#include "Condition.h"
#include "Global.h"
#include "Hospital.h"
#include "Property.h"
#include "Person.h"
#include "Random.h"

//Private static variables that will be set by property lookups
std::vector<double> Hospital::Hospital_health_insurance_prob;

Hospital::Hospital() : Place() {
  this->type_id = Place_Type::get_type_id("Hospital");
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
  this->type_id = Place_Type::get_type_id("Hospital");
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

void Hospital::get_properties() {
  if(Global::Enable_Health_Insurance) {
    Property::get_property_vector((char*)"Hospital_health_insurance_prob", Hospital::Hospital_health_insurance_prob);
    assert(static_cast<int>(Hospital::Hospital_health_insurance_prob.size()) == static_cast<int>(Insurance_assignment_index::UNSET));
  }
}

int Hospital::get_bed_count(int sim_day) {
  return this->bed_count;
}

int Hospital::get_daily_patient_capacity(int sim_day) {
  return this->daily_patient_capacity;
}

bool Hospital::is_open(int sim_day) {
  bool open = (sim_day < this->close_day || this->open_day <= sim_day);
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


