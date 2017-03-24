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
// File: Household.cc
//
#include <climits>
#include <set>

#include "Household.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Condition_List.h"
#include "Utils.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Population.h"
#include <algorithm>

using namespace std;

//Private static variables that will be set by parameter lookups
double Household::contacts_per_day;
double Household::same_age_bias = 0.0;
double** Household::prob_transmission_per_contact;

std::set<long int> Household::census_tract_set;
std::map<int, int> Household::count_inhabitants_by_household_income_level_map;
std::map<int, int> Household::count_children_by_household_income_level_map;
std::map<long int, int> Household::count_inhabitants_by_census_tract_map;
std::map<long int, int> Household::count_children_by_census_tract_map;

int Household::Cat_I_Max_Income = 0;
int Household::Cat_II_Max_Income = 0;
int Household::Cat_III_Max_Income = 0;
int Household::Cat_IV_Max_Income = 0;
int Household::Cat_V_Max_Income = 0;
int Household::Cat_VI_Max_Income = 0;

int Household::Min_hh_income = INT_MAX;
int Household::Max_hh_income = -1;
int Household::Min_hh_income_90_pct = -1;

Household::Household() : Place() {
  this->set_type(Place::TYPE_HOUSEHOLD);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->hh_schl_aged_chld_unemplyd_adlt_is_set = false;
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
  this->group_quarters_units = 0;
  this->group_quarters_workplace = NULL;
  this->income_quartile = -1;
  this->household_income = -1;
  this->household_income_code = Household_income_level_code::UNCLASSIFIED;
  this->migration_fips = 0;
}

Household::Household(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_HOUSEHOLD);
  this->set_subtype(_subtype);
  this->orig_household_structure = UNKNOWN;
  this->household_structure = UNKNOWN;
  this->hh_schl_aged_chld_unemplyd_adlt_is_set = false;
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
  this->intimacy = 1.0;
  this->group_quarters_units = 0;
  this->group_quarters_workplace = NULL;
  this->income_quartile = -1;
  this->household_income = -1;
  this->household_income_code = Household_income_level_code::UNCLASSIFIED;
  this->migration_fips = 0;
}

void Household::get_parameters() {

  Params::get_param("household_contacts", &Household::contacts_per_day);
  Params::get_param("neighborhood_same_age_bias", &Household::same_age_bias);
  Household::same_age_bias *= 0.5;
  int n = Params::get_param_matrix((char *)"household_trans_per_contact", &Household::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nHousehold contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	printf("%f ", Household::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  //Get the Household Income Cutoffs
  Params::get_param("cat_I_max_income", &Household::Cat_I_Max_Income);
  Params::get_param("cat_II_max_income", &Household::Cat_II_Max_Income);
  Params::get_param("cat_III_max_income", &Household::Cat_III_Max_Income);
  Params::get_param("cat_IV_max_income", &Household::Cat_IV_Max_Income);
  Params::get_param("cat_V_max_income", &Household::Cat_V_Max_Income);
  Params::get_param("cat_VI_max_income", &Household::Cat_VI_Max_Income);
}

int Household::get_group(int condition, Person* per) {
  int age = per->get_age();
  if(age < Global::ADULT_AGE) {
    return 0;
  } else {
    return 1;
  }
}

double Household::get_transmission_probability(int condition, Person* i, Person* s) {
  double age_i = i->get_real_age();
  double age_s = s->get_real_age();
  double diff = fabs(age_i - age_s);
  double prob = exp(-Household::same_age_bias * diff);
  return prob;
}

double Household::get_transmission_prob(int condition, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition, i);
  int col = get_group(condition, s);
  double tr_pr = Household::prob_transmission_per_contact[row][col];
  return tr_pr;
}

double Household::get_contacts_per_day(int condition) {
  return Household::contacts_per_day;
}

void Household::set_household_has_hospitalized_member(bool does_have) {
  this->hh_schl_aged_chld_unemplyd_adlt_is_set = false;
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
  if(this->hh_schl_aged_chld_unemplyd_adlt_is_set) {
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
    return ret_val;
  }
}

bool Household::has_school_aged_child_and_unemployed_adult() {
  //Household has been loaded
  assert(Global::Pop.is_load_completed());
  //If the household status hasn't changed, just return the flag
  if(this->hh_schl_aged_chld_unemplyd_adlt_is_set) {
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
    this->hh_schl_aged_chld_unemplyd_adlt_is_set = true;
    return ret_val;
  }
}

void Household::prepare_person_childcare_sickleave_map() {
  //Household has been loaded
  assert(Global::Pop.is_load_completed());

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
    if(!itr->second.stay_home_with_child(adult)) { //didn't already stay home with this child
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

void Household::set_household_structure () {

  if (this->is_college_dorm()) {
    this->household_structure = DORM_MATES;
    strcpy(this->household_structure_label, htype[this->household_structure].c_str());
    return;
  }

  if (this->is_prison_cell()) {
    this->household_structure = CELL_MATES;
    strcpy(this->household_structure_label, htype[this->household_structure].c_str());
    return;
  }

  if (this->is_military_barracks()) {
    this->household_structure = BARRACK_MATES;
    strcpy(this->household_structure_label, htype[this->household_structure].c_str());
    return;
  }

  if (this->is_nursing_home()) {
    this->household_structure = NURSING_HOME_MATES;
    strcpy(this->household_structure_label, htype[this->household_structure].c_str());
    return;
  }

  Person* person = NULL;
  int count[76];
  for (int i = 0; i < 76; i++) { count[i] = 0;}
  int hsize = 0;
  int male_adult = 0;
  int female_adult = 0;
  int male_minor = 0;
  int female_minor = 0;
  int max_minor_age = -1;
  int max_adult_age = -1;
  int min_minor_age = 100;
  int min_adult_age = 100;
  int size = get_size();
  // std::vector<int> ages;
  // ages.clear();
  for(int i = 0; i < size; ++i) {
    person = get_enrollee(i);
    if(person != NULL) {
      hsize++;
      int a = person->get_age();
      // ages.push_back(a);
      if (a > 75) {
	a = 75;
      }
      count[a]++;
      if (a >= 18 && a < min_adult_age) { min_adult_age = a; }
      if (a >= 18 && a > max_adult_age) { max_adult_age = a; }
      if (a < 18 && a < min_minor_age) { min_minor_age = a; }
      if (a < 18 && a > max_minor_age) { max_minor_age = a; }
      if (a >= 18) {
	if (person->get_sex() == 'F') {
	  female_adult++;
	}
	else {
	  male_adult++;
	}
      }
      else {
	if (person->get_sex() == 'F') {
	  female_minor++;
	}
	else {
	  male_minor++;
	}
      }
    }
  }
  /*
  std::sort (ages.begin(), ages.end());
  // find biggest difference in ages
  int max_diff = -1;
  int max_diff_i = 0;
  for (int i = 1; i < hsize; i++) {
    if (max_diff < (ages[i]-ages[i-1])) {
      max_diff = ages[i]-ages[i-1];
      max_diff_i = i;
    }
  }
  */
  htype_t t = UNKNOWN;

  if (max_minor_age > -1) {
    // households with minors:

    // more than two adults
    if (male_adult+female_adult>2) {
      // check for adult children in household
      int max_child_age = -1;
      for(int i = 0; i < hsize; ++i) {
	person = get_enrollee(i);
	int age = person->get_age();
	// find age of oldest "child"
	if (age < 30 && age < max_minor_age+15 && age > max_child_age) {
	  max_child_age = age;
	}
      }
      // count potential parents
      int males = 0;
      int females = 0;
      int max_par_age = -1;
      int min_par_age = 100;
      for(int i = 0; i < hsize; ++i) {
	person = get_enrollee(i);
	int age = person->get_age();
	// count potential parents
	if (age >= max_child_age+15) {
	  if (age > max_par_age) { max_par_age = age; }
	  if (age < min_par_age) { min_par_age = age; }
	  if (person->get_sex() == 'F') {
	    females++;
	  }
	  else {
	    males++;
	  }
	}
      }
      if (0 < males+females && males+females <= 2 && max_par_age < min_par_age+15) {
	// we have at least one potential biological parent
	if (males==1 && females==1) {
	  t = OPP_SEX_TWO_PARENTS;
	}
	else if (males+females==2) {
	  t = SAME_SEX_TWO_PARENTS;
	}
	else if (males+females==1) {
	  t = SINGLE_PARENT;
	}
      }
      else if (max_par_age >= min_par_age + 15) {
	// multi-gen family:
	// delete the older generation
	int ma = males;
	int fa = females;
	for(int i = 0; i < hsize; ++i) {
	  person = get_enrollee(i);
	  int age = person->get_age();
	  if (age >= min_par_age+15) {
	    if (person->get_sex() == 'F') {
	      fa--;
	    }
	    else {
	      ma--;
	    }
	  }
	}
	if (ma+fa==1) {
	  t = SINGLE_PAR_MULTI_GEN_FAMILY;
	}
	else if (ma+fa==2) {
	  t = TWO_PAR_MULTI_GEN_FAMILY;
	}
	else {
	  t = OTHER_FAMILY;
	}
      }
      else {
	t = OTHER_FAMILY;
      }
    } // end more than 2 adults
    
    // two adults
    if (male_adult+female_adult == 2) {
      if (max_adult_age < min_adult_age + 15) {
	if (male_adult==1 && female_adult==1) {
	  t = OPP_SEX_TWO_PARENTS;
	}
	else {
	  t = SAME_SEX_TWO_PARENTS;
	}
      }
      else {
	t = SINGLE_PAR_MULTI_GEN_FAMILY;
      }
    } // end two adults

    // one adult
    if (male_adult+female_adult == 1) {
      t = SINGLE_PARENT;
    } // end one adult

    // no adults
    if (male_adult+female_adult==0) {
      if (hsize==2 && max_minor_age >= 15 && min_minor_age <= max_minor_age-14) {
	t = SINGLE_PARENT;
      } 
      else if (count[15]+count[16]+count[17]==2 && min_minor_age <= max_minor_age-14) {
	t = OPP_SEX_TWO_PARENTS;
      } 
      else if (hsize==2 && count[15]+count[16]+count[17]==2) {
	if (female_minor && male_minor) {
	  t = OPP_SEX_SIM_AGE_PAIR;
	}
	else {
	  t = SAME_SEX_SIM_AGE_PAIR;
	}
      } 
      else if (hsize == 1 && max_minor_age > 14) {
	if (female_minor) {
	  t = SINGLE_FEMALE;
	}
	else {
	  t = SINGLE_MALE;
	}
      }
      else if (hsize > 2 && count[17]==hsize) {
	t = YOUNG_ROOMIES;
      }
      else {
	t = UNATTENDED_KIDS;
      }
    }
  } // end households with minors

  else {

    // single adults
    if (hsize == 1) {
      if (female_adult || female_minor) {
	t = SINGLE_FEMALE;
      }
      else {
	t = SINGLE_MALE;
      }
    }

    // pairs of adults
    if (hsize == 2) {
      if (max_adult_age < min_adult_age+15) {
	if (male_adult && female_adult) {
	  t = OPP_SEX_SIM_AGE_PAIR;
	}
	else {
	  t = SAME_SEX_SIM_AGE_PAIR;
	}
      }
      else {
	if (male_adult && female_adult) {
	  t = OPP_SEX_DIF_AGE_PAIR;
	}
	else {
	  t = SAME_SEX_DIF_AGE_PAIR;
	}
      }
    } // end adults pairs

    // more than 2 adults
    if (hsize > 2) {
      if (max_adult_age < 30) {
	t = YOUNG_ROOMIES;
      }
      else if (min_adult_age >= 30) {
	t = OLDER_ROOMIES;
      }
      else {
	t = MIXED_ROOMIES;
      }
    }
  } // end adult-only households

  this->household_structure = t;
  strcpy(this->household_structure_label, htype[t].c_str());

  /*
  printf("HOUSEHOLD_TYPE: %s size = %d ", get_household_structure_label(), get_size());
  for(int i = 0; i < hsize; ++i) {
    Person* person = get_enrollee(i);
    printf("%d%c ", person->get_age(), person->get_sex());
  }
  printf("\n");
  */

}
