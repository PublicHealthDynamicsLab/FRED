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
// File: Household.cc
//
#include <limits>
#include <set>

#include "Household.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Disease_List.h"
#include "Utils.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Population.h"
#include <algorithm>

using namespace std;

//Private static variables that will be set by parameter lookups
double* Household::Household_contacts_per_day;
double*** Household::Household_contact_prob;

std::set<long int> Household::census_tract_set;
std::map<int, int> Household::count_inhabitants_by_household_income_level_map;
std::map<int, int> Household::count_children_by_household_income_level_map;
std::map<long int, int> Household::count_inhabitants_by_census_tract_map;
std::map<long int, int> Household::count_children_by_census_tract_map;

//Private static variable to assure we only lookup parameters once
bool Household::Household_parameters_set = false;

int Household::Cat_I_Max_Income = 0;
int Household::Cat_II_Max_Income = 0;
int Household::Cat_III_Max_Income = 0;
int Household::Cat_IV_Max_Income = 0;
int Household::Cat_V_Max_Income = 0;
int Household::Cat_VI_Max_Income = 0;

int Household::Min_hh_income = INT_MAX;
int Household::Max_hh_income = -1;
int Household::Min_hh_income_90_pct = -1;

Household::Household() {
  get_parameters(Global::Diseases.get_number_of_diseases());
  this->type = Place::HOUSEHOLD;
  this->subtype = fred::PLACE_SUBTYPE_NONE;
  this->sheltering = false;
  this->hh_schl_aged_chld_unemplyd_adlt_chng = false;
  this->hh_schl_aged_chld = false;
  this->hh_schl_aged_chld_unemplyd_adlt = false;
  this->hh_sympt_child = false;
  this->hh_working_adult_using_sick_leave = false;
  this->primary_healthcare_available = true;
  this->other_healthcare_location_that_accepts_insurance_available = true;
  this->healthcare_available = true;
  this->count_seeking_hc = 0;
  this->count_primary_hc_unav = 0;
  this->count_hc_accept_ins_unav = 0;
  this->shelter_start_day = 0;
  this->shelter_end_day = 0;
  this->deme_id = ' ';
  this->group_quarters_units = 0;
  this->income_quartile = -1;
  set_household_income(-1);
}

Household::Household(const char* lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) {
  get_parameters(Global::Diseases.get_number_of_diseases());
  this->type = Place::HOUSEHOLD;
  this->subtype = _subtype;
  this->sheltering = false;
  this->hh_schl_aged_chld_unemplyd_adlt_chng = false;
  this->hh_schl_aged_chld = false;
  this->hh_schl_aged_chld_unemplyd_adlt = false;
  this->hh_sympt_child = false;
  this->hh_working_adult_using_sick_leave = false;
  this->seeking_healthcare = false;
  this->primary_healthcare_available = true;
  this->other_healthcare_location_that_accepts_insurance_available = true;
  this->healthcare_available = true;
  this->count_seeking_hc = 0;
  this->count_primary_hc_unav = 0;
  this->count_hc_accept_ins_unav = 0;
  this->shelter_start_day = 0;
  this->shelter_end_day = 0;
  this->deme_id = ' ';
  setup(lab, lon, lat);
  this->group_quarters_units = 0;
  this->group_quarters_workplace = NULL;
  this->income_quartile = -1;
  set_household_income(-1);
}

void Household::get_parameters(int diseases) {
  if(Household::Household_parameters_set) {
    return;
  }
  Household::Household_parameters_set = true;

  char param_str[80];
  Household::Household_contacts_per_day = new double[diseases];
  Household::Household_contact_prob = new double**[diseases];
  for(int disease_id = 0; disease_id < diseases; ++disease_id) {
    Disease* disease = Global::Diseases.get_disease(disease_id);
    if (strcmp("respiratory",disease->get_transmission_mode())==0) {
      sprintf(param_str, "%s_household_contacts", disease->get_disease_name());
      Params::get_param((char*) param_str, &Household::Household_contacts_per_day[disease_id]);
      sprintf(param_str, "%s_household_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Household::Household_contact_prob[disease_id]);
      if(Global::Verbose > 1) {
	printf("\nHousehold_contact_prob:\n");
	for(int i  = 0; i < n; ++i)  {
	  for(int j  = 0; j < n; ++j) {
	    printf("%f ", Household::Household_contact_prob[disease_id][i][j]);
	  }
	  printf("\n");
	}
      }
    }
  }

  //Get the Household Income Cutoffs
  Params::get_param((char*)"cat_I_max_income", &Household::Cat_I_Max_Income);
  Params::get_param((char*)"cat_II_max_income", &Household::Cat_II_Max_Income);
  Params::get_param((char*)"cat_III_max_income", &Household::Cat_III_Max_Income);
  Params::get_param((char*)"cat_IV_max_income", &Household::Cat_IV_Max_Income);
  Params::get_param((char*)"cat_V_max_income", &Household::Cat_V_Max_Income);
  Params::get_param((char*)"cat_VI_max_income", &Household::Cat_VI_Max_Income);

}

int Household::get_group(int disease, Person* per) {
  int age = per->get_age();
  if(age < Global::ADULT_AGE) {
    return 0;
  } else {
    return 1;
  }
}

double Household::get_transmission_prob(int disease, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Household::Household_contact_prob[disease][row][col];
  return tr_pr;
}

double Household::get_contacts_per_day(int disease) {
  return Household::Household_contacts_per_day[disease];
}

void Household::set_household_has_hospitalized_member(bool does_have) {
  this->hh_schl_aged_chld_unemplyd_adlt_chng = true;
  if(does_have) {
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = true;
  } else {
    //Initially say no one is hospitalized
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = false;
    //iterate over all housemates  to see if anyone is still hospitalized
    vector<Person*>::iterator it;

    for(it = this->enrollees.begin(); it != this->enrollees.end(); ++it) {
      if((*it)->is_hospitalized()) {
        this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = true;
        if(this->get_household_visitation_hospital() == NULL) {
          this->set_household_visitation_hospital(static_cast<Hospital*>((*it)->get_activities()->get_hospital()));
        }
        break;
      }
    }
  }
}

bool Household::has_school_aged_child() {
  //Household has been loaded
  assert(Global::Pop.is_load_completed());
  //If the household status hasn't changed, just return the flag
  if(!this->hh_schl_aged_chld_unemplyd_adlt_chng) {
    return this->hh_schl_aged_chld;
  } else {
    bool ret_val = false;
    for(int i = 0; i < static_cast<int>(this->enrollees.size()); ++i) {
      Person* per = this->enrollees[i];
      if(per->is_student() && per->is_child()) {
        ret_val = true;
        break;
      }
    }
    this->hh_schl_aged_chld = ret_val;
    this->hh_schl_aged_chld_unemplyd_adlt_chng = false;
    return ret_val;
  }
}

bool Household::has_school_aged_child_and_unemployed_adult() {
  //Household has been loaded
  assert(Global::Pop.is_load_completed());
  //If the household status hasn't changed, just return the flag
  if(!this->hh_schl_aged_chld_unemplyd_adlt_chng) {
    return this->hh_schl_aged_chld_unemplyd_adlt;
  } else {
    bool ret_val = false;
    if(has_school_aged_child()) {
      for(int i = 0; i < static_cast<int>(this->enrollees.size()); ++i) {
        Person* per = this->enrollees[i];
        if(per->is_child()) {
          continue;
        }

        //Person is an adult, but is also a student
        if(per->is_adult() && per->is_student()) {
          continue;
        }

        //Person is an adult, but isn't at home
        if(per->is_adult() &&
           (per->is_hospitalized() ||
            per->is_college_dorm_resident() ||
            per->is_military_base_resident() ||
            per->is_nursing_home_resident() ||
            per->is_prisoner())) {
          continue;
        }

        //Person is an adult AND is either retired or unemployed
        if(per->is_adult() &&
           (per->get_activities()->get_profile() == RETIRED_PROFILE ||
            per->get_activities()->get_profile() == UNEMPLOYED_PROFILE)) {
          ret_val = true;
          break;
        }
      }
    }
    this->hh_schl_aged_chld_unemplyd_adlt = ret_val;
    this->hh_schl_aged_chld_unemplyd_adlt_chng = false;
    return ret_val;
  }
}

void Household::prepare_person_childcare_sickleave_map() {
  //Household has been loaded
  assert(Global::Pop.is_load_completed());
  assert(Household::Household_parameters_set);

  if(Global::Report_Childhood_Presenteeism) {
    if(has_school_aged_child() && !has_school_aged_child_and_unemployed_adult()) {
      for(int i = 0; i < static_cast<int>(this->enrollees.size()); ++i) {
        Person* per = this->enrollees[i];
        if(per->is_child()) {
          continue;
        }

        //Person is an adult, but is also a student
        if(per->is_adult() && per->is_student()) {
          continue;
        }

        //Person is an adult, but isn't at home
        if(per->is_adult() &&
           (per->is_hospitalized() ||
            per->is_college_dorm_resident() ||
            per->is_military_base_resident() ||
            per->is_nursing_home_resident() ||
            per->is_prisoner())) {
          continue;
        }

        //Person is an adult AND is neither retired nor unemployed
        if(per->is_adult() &&
           per->get_activities()->get_profile() != RETIRED_PROFILE &&
           per->get_activities()->get_profile() != UNEMPLOYED_PROFILE) {
          //Insert the adult into the sickleave info map
          HH_Adult_Sickleave_Data sickleave_info;

          //Add any school-aged children to that person's info
          for(int j = 0; j < static_cast<int>(this->enrollees.size()); ++j) {
            Person* child_check = this->enrollees[j];
            if(child_check->is_student() && child_check->is_child()) {
              sickleave_info.add_child_to_maps(child_check);
            }
          }
          std::pair<std::map<Person*, HH_Adult_Sickleave_Data>::iterator, bool> ret;
          ret = this->adult_childcare_sickleave_map.insert(
							   std::pair<Person*, HH_Adult_Sickleave_Data>(per, sickleave_info));
          assert(ret.second); // Shouldn't insert the same person twice
        }
      }
    }
  } else {
    return;
  }
}

bool Household::have_working_adult_use_sickleave_for_child(Person* adult, Person* child) {
  std::map<Person*, HH_Adult_Sickleave_Data>::iterator itr;
  itr = this->adult_childcare_sickleave_map.find(adult);
  if(itr != this->adult_childcare_sickleave_map.end()) {
    if(!itr->second.stay_home_with_child(adult)) { //didn't already stayed home with this child
      return itr->second.stay_home_with_child(child);
    }
  }
  return false;
}

void Household::record_profile() {
  // record the ages
  this->ages.clear();
  int size = get_size();
  for(int i = 0; i < size; ++i) {
    this->ages.push_back(this->enrollees[i]->get_age());
  }

  // record the id's of the original members of the household
  this->ids.clear();
  for(int i = 0; i < size; ++i) {
    this->ids.push_back(this->enrollees[i]->get_id());
  }
}

