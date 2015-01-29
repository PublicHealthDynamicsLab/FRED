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
#include "Utils.h"
#include "Random.h"
#include "Transmission.h"
#include "Regional_Layer.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"

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

Household::Household() {
  get_parameters(Global::Diseases);
  this->type = Place::HOUSEHOLD;
  this->subtype = fred::PLACE_SUBTYPE_NONE;
  this->sheltering = false;
  this->shelter_start_day = 0;
  this->shelter_end_day = 0;
  this->county_index = -1;
  this->census_tract_index = -1;
  this->deme_id = ' ';
  this->N = 0;
  this->group_quarters_units = 0;
  set_household_income(-1);
}

Household::Household(const char* lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat, Place* container, Population* pop) {
  get_parameters(Global::Diseases);
  this->type = Place::HOUSEHOLD;
  this->subtype = _subtype;
  this->sheltering = false;
  this->shelter_start_day = 0;
  this->shelter_end_day = 0;
  this->county_index = -1;
  this->census_tract_index = -1;
  this->deme_id = ' ';
  setup(lab, lon, lat, container, pop);
  this->N = 0;
  this->group_quarters_units = 0;
  this->group_quarters_workplace = NULL;
  set_household_income(-1);
}

void Household::get_parameters(int diseases) {
  if(Household::Household_parameters_set) {
    return;
  }

  if (Global::Enable_Vector_Transmission == false) {
    char param_str[80];
    Household::Household_contacts_per_day = new double [diseases];
    Household::Household_contact_prob = new double** [diseases];
    for(int disease_id = 0; disease_id < diseases; ++disease_id) {
      Disease* disease = Global::Pop.get_disease(disease_id);
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
  Params::get_param((char*) "cat_I_max_income", &Household::Cat_I_Max_Income);
  Params::get_param((char*) "cat_II_max_income", &Household::Cat_II_Max_Income);
  Params::get_param((char*) "cat_III_max_income", &Household::Cat_III_Max_Income);
  Params::get_param((char*) "cat_IV_max_income", &Household::Cat_IV_Max_Income);
  Params::get_param((char*) "cat_V_max_income", &Household::Cat_V_Max_Income);
  Params::get_param((char*) "cat_VI_max_income", &Household::Cat_VI_Max_Income);

  Household::Household_parameters_set = true;
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
  if(does_have) {
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = true;
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = false;
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = false;
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

void Household::record_profile() {
  // record the ages in sorted order
  this->ages.clear();
  for(int i = 0; i < this->N; ++i) {
    this->ages.push_back(this->enrollees[i]->get_age());
  }
  sort(this->ages.begin(), this->ages.end());

  // record the id's of the original members of the household
  this->ids.clear();
  for(int i = 0; i < this->N; ++i) {
    this->ids.push_back(this->enrollees[i]->get_id());
  }
}

