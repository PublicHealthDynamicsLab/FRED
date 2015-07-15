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

#include <math.h>
#include <string>
#include <sstream>

#include "Disease.h"
#include "Global.h"
#include "Hospital.h"
#include "Params.h"
#include "Person.h"
#include "Place_List.h"
#include "Random.h"

class HAZEL_Hospital_Init_Data;

typedef std::map<std::string, HAZEL_Hospital_Init_Data> HospitalInitMapT;

//Private static variables that will be set by parameter lookups
double* Hospital::Hospital_contacts_per_day;
double*** Hospital::Hospital_contact_prob;
std::vector<double> Hospital::Hospital_health_insurance_prob;

bool Hospital::HAZEL_hospital_init_map_file_exists = false;
double Hospital::HAZEL_disaster_capacity_multiplier = 1.0;
int Hospital::HAZEL_mobile_van_max = 0;
int Hospital::HAZEL_mobile_van_count = 0;
int Hospital::HAZEL_mobile_van_active_count = 0;
std::vector<double> Hospital::HAZEL_reopening_CDF;
HospitalInitMapT Hospital::HAZEL_hospital_init_map;

//Private static variable to assure we only lookup parameters once
bool Hospital::Hospital_parameters_set = false;

Hospital::Hospital() {
  if(!Hospital::Hospital_parameters_set) {
    get_parameters(Global::Diseases);
  }
  this->type = Place::HOSPITAL;
  this->bed_count = 0;
  this->occupied_bed_count = 0;
  this->daily_patient_capacity = -1;
  this->current_daily_patient_count = 0;
  this->HAZEL_closure_dates_have_been_set = false;
  std::vector<bool>* checked_open_day_vec;
}

Hospital::Hospital(const char* lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat, Place* container, Population* pop) {
  if(!Hospital::Hospital_parameters_set) {
    get_parameters(Global::Diseases);
  }
  this->type = Place::HOSPITAL;
  this->subtype = _subtype;
  this->bed_count = 0;
  this->occupied_bed_count = 0;
  this->daily_patient_capacity = -1;
  this->current_daily_patient_count = 0;
  this->add_capacity = false;
  setup(lab, lon, lat, container, pop);

  if(Global::Enable_Health_Insurance) {
    vector<double>::iterator itr;
    int insr = static_cast<int>(Insurance_assignment_index::PRIVATE);
    for(itr = Hospital::Hospital_health_insurance_prob.begin(); itr != Hospital::Hospital_health_insurance_prob.end(); ++itr, ++insr) {
      set_accepts_insurance(insr, (Random::draw_random() < *itr));
    }
  }

  if(Global::Enable_HAZEL && Hospital::HAZEL_hospital_init_map_file_exists) {
    //Use the values read in from the map file
    if(Hospital::HAZEL_hospital_init_map.find(string(this->get_label())) != Hospital::HAZEL_hospital_init_map.end()) {
      HAZEL_Hospital_Init_Data init_data = Hospital::HAZEL_hospital_init_map.find(string(this->get_label()))->second;
      this->set_accepts_insurance(Insurance_assignment_index::HIGHMARK, init_data.accpt_highmark);
      this->set_accepts_insurance(Insurance_assignment_index::MEDICAID, init_data.accpt_medicaid);
      this->set_accepts_insurance(Insurance_assignment_index::MEDICARE, init_data.accpt_medicare);
      this->set_accepts_insurance(Insurance_assignment_index::PRIVATE, init_data.accpt_private);
      this->set_accepts_insurance(Insurance_assignment_index::UNINSURED, init_data.accpt_uninsured);
      this->set_accepts_insurance(Insurance_assignment_index::UPMC, init_data.accpt_upmc);
      if(init_data.is_mobile) {
        this->set_subtype(fred::PLACE_SUBTYPE_MOBILE_HEALTHCARE_CLINIC);
        Hospital::HAZEL_mobile_van_count++;
      }
      this->set_daily_patient_capacity((init_data.panel_week / 5) + 1);
      this->add_capacity = init_data.add_capacity;
    }
  }
}

void Hospital::get_parameters(int diseases) {
  if(Hospital::Hospital_parameters_set) {
    return;
  }
  
  char param_str[80];

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

  if(Global::Enable_HAZEL) {
    char HAZEL_hosp_init_file_name[FRED_STRING_SIZE];
    char hosp_init_file_dir[FRED_STRING_SIZE];


    Params::get_param_vector((char*)"HAZEL_reopening_CDF", Hospital::HAZEL_reopening_CDF);
    Params::get_param_from_string("HAZEL_disaster_capacity_multiplier", &Hospital::HAZEL_disaster_capacity_multiplier);
    Params::get_param_from_string("HAZEL_mobile_van_max", &Hospital::HAZEL_mobile_van_max);

    Params::get_param_from_string("HAZEL_hospital_init_file_directory", hosp_init_file_dir);
    Params::get_param_from_string("HAZEL_hospital_init_file_name", HAZEL_hosp_init_file_name);
    if(strcmp(HAZEL_hosp_init_file_name, "none") == 0) {
      Hospital::HAZEL_hospital_init_map_file_exists = false;
    } else {
      FILE* HAZEL_hosp_init_map_fp = NULL;

      char filename[FRED_STRING_SIZE];
      sprintf(filename, "%s%s", hosp_init_file_dir, HAZEL_hosp_init_file_name);
      HAZEL_hosp_init_map_fp = Utils::fred_open_file(filename);
      if(HAZEL_hosp_init_map_fp != NULL) {
        Hospital::HAZEL_hospital_init_map_file_exists = true;
        enum column_index {
          HOSP_ID = 0,
          PNL_WK = 1,
          ACCPT_PRIV = 2,
          ACCPT_MEDICR = 3,
          ACCPT_MEDICD = 4,
          ACCPT_HGHMRK = 5,
          ACCPT_UPMC = 6,
          ACCPT_UNINSRD = 7,
          REOPEN_AFTR_DAYS = 8,
          IS_MOBILE = 9,
          ADD_CAPACITY = 10
        };

        char line_str[255];
        int temp_int = 0;
        Utils::Tokens tokens;
        for(char* line = line_str; fgets(line, 255, HAZEL_hosp_init_map_fp); line = line_str) {
          tokens = Utils::split_by_delim(line, ',', tokens, false);
          // skip header line
          if(strcmp(tokens[HOSP_ID], "sp_id") != 0) {
            char place_type = Place::HOSPITAL;
            char s[80];
            sprintf(s, "%c%s", place_type, tokens[HOSP_ID]);
            string hosp_id_str(s);
            HAZEL_Hospital_Init_Data init_data = HAZEL_Hospital_Init_Data(tokens[PNL_WK], tokens[ACCPT_PRIV],
                tokens[ACCPT_MEDICR], tokens[ACCPT_MEDICD], tokens[ACCPT_HGHMRK],
                tokens[ACCPT_UPMC], tokens[ACCPT_UNINSRD], tokens[REOPEN_AFTR_DAYS],
                tokens[IS_MOBILE], tokens[ADD_CAPACITY]);

            this->HAZEL_hospital_init_map.insert(std::pair<string, HAZEL_Hospital_Init_Data>(hosp_id_str, init_data));
          }
          tokens.clear();
        }
        fclose(HAZEL_hosp_init_map_fp);
      }
    }
  }

  if(Global::Enable_Health_Insurance || (Global::Enable_HAZEL && !Hospital::HAZEL_hospital_init_map_file_exists)) {
    Params::get_param_vector((char*)"hospital_health_insurance_prob", Hospital::Hospital_health_insurance_prob);
    assert(static_cast<int>(Hospital::Hospital_health_insurance_prob.size()) == static_cast<int>(Insurance_assignment_index::UNSET));
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

int Hospital::get_bed_count(int sim_day) {
  if(Global::Enable_HAZEL) {
    if(sim_day < Place_List::get_HAZEL_disaster_end_sim_day()) {
      return this->bed_count;
    } else {
      if(this->add_capacity) {
        return static_cast<int>(ceil(Hospital::HAZEL_disaster_capacity_multiplier * this->bed_count));
      } else {
        return this->bed_count;
      }
    }
  } else {
    return this->bed_count;
  }
}

int Hospital::get_daily_patient_capacity(int sim_day) {
  if(Global::Enable_HAZEL) {
    if(sim_day < Place_List::get_HAZEL_disaster_end_sim_day()) {
      return this->daily_patient_capacity;
    } else {
      if(this->add_capacity) {
        return static_cast<int>(ceil(Hospital::HAZEL_disaster_capacity_multiplier * this->daily_patient_capacity));
      } else {
        return this->daily_patient_capacity;
      }
    }
  } else {
    return this->daily_patient_capacity;
  }
}

double Hospital::get_contacts_per_day(int disease) {
  return Hospital::Hospital_contacts_per_day[disease];
}

bool Hospital::should_be_open(int day) {

  if(Global::Enable_HAZEL) {
    // If we haven't made closure decision, do it now
    if(!this->HAZEL_closure_dates_have_been_set) {
      if(this->get_subtype() == fred::PLACE_SUBTYPE_MOBILE_HEALTHCARE_CLINIC) {
        if(day <= Place_List::get_HAZEL_disaster_end_sim_day()) {
          //Not open until after disaster
          return false;
        } else {
          if(Hospital::HAZEL_mobile_van_max == 0) {
            return false;
          }
          // First roll against the max number allowed to be open / number of mobile clinics in simulation
          double prob = 0.0;
          if(Hospital::HAZEL_mobile_van_max > Hospital::HAZEL_mobile_van_count) {
            prob = 1.0;
          } else if(Hospital::HAZEL_mobile_van_active_count < Hospital::HAZEL_mobile_van_max && Hospital::HAZEL_mobile_van_count != 0) {
            prob = static_cast<double>(Hospital::HAZEL_mobile_van_max) / static_cast<double>(Hospital::HAZEL_mobile_van_count);
          }
          if(Random::draw_random() < prob) {
            Hospital::HAZEL_mobile_van_active_count++;
            //The Mobile Healthcare Clinics open 1 to 5 days after the disaster ends
            double cdf_arr[5] = { 0.15, 0.35, 0.55, 0.75, 1.0 };
            int cdf_day = Random::draw_from_cdf(cdf_arr, 5);
            this->set_close_date(Place_List::get_HAZEL_disaster_end_sim_day());
            this->set_open_date(Place_List::get_HAZEL_disaster_end_sim_day() + cdf_day);
            this->HAZEL_closure_dates_have_been_set = true;
            return true;
          } else {
            return false;
          }
        }
      } else {
        apply_individual_HAZEL_closure_policy();
      }
    }
  }

  return is_open(day);
}

bool Hospital::should_be_open(int day, int disease) {

  if(Global::Enable_HAZEL) {
    return this->should_be_open(day);
  }

  return is_open(day);
}

void Hospital::apply_individual_HAZEL_closure_policy() {
  assert(Global::Enable_HAZEL);
  if(Hospital::HAZEL_hospital_init_map_file_exists) {
    if(Hospital::HAZEL_hospital_init_map.find(string(this->get_label())) != Hospital::HAZEL_hospital_init_map.end()) {
      HAZEL_Hospital_Init_Data init_data = Hospital::HAZEL_hospital_init_map.find(string(this->get_label()))->second;
      if(init_data.reopen_after_days > 0) {
        if(Place_List::get_HAZEL_disaster_start_sim_day() != -1 && Place_List::get_HAZEL_disaster_end_sim_day() != -1) {
          this->set_close_date(Place_List::get_HAZEL_disaster_start_sim_day());
          this->set_open_date(Place_List::get_HAZEL_disaster_end_sim_day() + init_data.reopen_after_days);
          this->HAZEL_closure_dates_have_been_set = true;
        }
      }
    }
  }

  if(!this->HAZEL_closure_dates_have_been_set) {
    int cdf_day = Random::draw_from_cdf_vector(Hospital::HAZEL_reopening_CDF);
    this->set_close_date(Place_List::get_HAZEL_disaster_start_sim_day());
    this->set_open_date(Place_List::get_HAZEL_disaster_end_sim_day() + cdf_day);
    this->HAZEL_closure_dates_have_been_set = true;
  }
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
     << ", HAZEL_closure_dates_have_been_set: " << this->HAZEL_closure_dates_have_been_set
     << ", subtype: " << this->subtype;

  return ss.str();
}


