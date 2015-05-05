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
std::vector<double> Hospital::Hospital_health_insurance_prob;
int Hospital::HAZEL_disaster_start_day = -1;
int Hospital::HAZEL_disaster_end_day = -1;
double Hospital::HAZEL_disaster_max_bed_usage_multiplier = 1.0;
std::vector<double> Hospital::HAZEL_reopening_CDF;

//Private static variable to assure we only lookup parameters once
bool Hospital::Hospital_parameters_set = false;

Hospital::Hospital() {
  this->type = Place::HOSPITAL;
  this->bed_count = 0;
  this->capacity = 0;
  this->is_open = true;
  get_parameters(Global::Diseases);
}

Hospital::Hospital(const char* lab, fred::place_subtype _subtype, double lon, double lat, Place* container, Population* pop) {
  if(!Hospital::Hospital_parameters_set) {
    get_parameters(Global::Diseases);
  }
  this->type = Place::HOSPITAL;
  this->subtype = _subtype;
  this->capacity = 0;
  this->bed_count = 0;
  setup(lab, lon, lat, container, pop);

  if(Global::Enable_Health_Insurance) {
    vector<double>::iterator itr;
    int insr = static_cast<int>(Insurance_assignment_index::PRIVATE);
    for(itr = Hospital::Hospital_health_insurance_prob.begin(); itr != Hospital::Hospital_health_insurance_prob.end(); ++itr, ++insr) {
      set_accepts_insurance(insr, (RANDOM() < *itr));
    }
  }

  if(Global::Enable_HAZEL) {
    this->checked_open_day_vec = new std::vector<bool>();
  }
}

void Hospital::get_parameters(int diseases) {
  char param_str[80];
  
  if(Hospital::Hospital_parameters_set) {
    return;
  }
  
  if(!Global::Enable_Vector_Transmission) {
    Hospital::Hospital_contacts_per_day = new double[diseases];
    Hospital::Hospital_contact_prob = new double**[diseases];

    char param_str[80];
    for(int disease_id = 0; disease_id < diseases; ++disease_id) {
      Disease* disease = Global::Pop.get_disease(disease_id);
      sprintf(param_str, "%s_hospital_contacts", disease->get_disease_name());
      Params::get_param((char*)param_str, &Hospital::Hospital_contacts_per_day[disease_id]);
      sprintf(param_str, "%s_hospital_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Hospital::Hospital_contact_prob[disease_id]);
      if(Global::Verbose > 1) {
        printf("\nHospital_contact_prob:\n");
        for(int i  = 0; i < n; ++i)  {
          for(int j  = 0; j < n; ++j) {
            printf("%f ", Hospital::Hospital_contact_prob[disease_id][i][j]);
          }
          printf("\n");
        }
      }
    }
  }
  
  if(Global::Enable_Health_Insurance) {
    Params::get_param_vector((char*)"hospital_health_insurance_prob", Hospital::Hospital_health_insurance_prob);
    assert(static_cast<int>(Hospital::Hospital_health_insurance_prob.size()) == static_cast<int>(Insurance_assignment_index::UNSET));
  }

  if(Global::Enable_HAZEL) {
    Params::get_param_vector((char*)"HAZEL_reopening_CDF", Hospital::HAZEL_reopening_CDF);
    Params::get_param((char*)"HAZEL_disaster_start_day", &Hospital::HAZEL_disaster_start_day);
    Params::get_param((char*)"HAZEL_disaster_end_day", &Hospital::HAZEL_disaster_end_day);
    Params::get_param((char*)"HAZEL_disaster_max_bed_usage_multiplier", &Hospital::HAZEL_disaster_max_bed_usage_multiplier);
//    printf("**************************************\n");
//    printf("BEGIN HAZEL_reopening_CDF\n");
//    for(int i = 0; i < static_cast<int>(Hospital::Hospital_health_insurance_prob.size()); ++i) {
//      printf("HAZEL_reopening_CDF[%d]: %.2f\n", i, Hospital::Hospital_health_insurance_prob[i]);
//    }
//    printf("END HAZEL_reopening_CDF\n");
//    printf("**************************************\n");
  }

  Hospital::Hospital_parameters_set = true;
}

int Hospital::get_group(int disease, Person* per) {
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

bool Hospital::should_be_open(int day) {
  if(Global::Diseases == 0) {
    return true;
  } else {
    return should_be_open(day, 0);
  }
}

bool Hospital::should_be_open(int day, int disease) {
  if(Global::Enable_HAZEL) {
    if(day < Hospital::HAZEL_disaster_start_day) {
      this->is_open = true;
    } else if(Hospital::HAZEL_disaster_start_day <= day && day <= Hospital::HAZEL_disaster_end_day) {
      this->is_open = false;
    } else if(day > Hospital::HAZEL_disaster_end_day) {
      if(!this->is_open) {
        int cdf_day = day - Hospital::HAZEL_disaster_end_day;

        if(this->checked_open_day_vec->size() == cdf_day) {
          return this->checked_open_day_vec->at(cdf_day - 1);
        } else {
          int cdf_size = static_cast<int>(Hospital::HAZEL_reopening_CDF.size());
          cdf_day = (cdf_day < 0 ? 0 : cdf_day - 1);
          cdf_day = (cdf_day >= cdf_size ? cdf_size - 1 : cdf_day);
          this->is_open = (RANDOM() < Hospital::HAZEL_reopening_CDF[cdf_day]);
          this->checked_open_day_vec->push_back(this->is_open);
        }
      }
    }
  }
  return this->is_open;
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






