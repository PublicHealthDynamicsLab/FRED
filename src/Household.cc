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
// File: Household.cc
//
#include <climits>
#include <set>

#include "Household.h"
#include "Global.h"
#include "Property.h"
#include "Person.h"
#include "Utils.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include <algorithm>

using namespace std;

Household::Household(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, Place_Type::get_type_id("Household"), lon, lat) {
  this->set_subtype(_subtype);
  this->orig_household_structure = UNKNOWN;
  this->household_structure = UNKNOWN;
  this->hh_schl_aged_chld_unemplyd_adlt_is_set = false;
  this->hh_schl_aged_chld = false;
  this->hh_schl_aged_chld_unemplyd_adlt = false;
  this->seeking_healthcare = false;
  this->primary_healthcare_available = true;
  this->other_healthcare_location_that_accepts_insurance_available = true;
  this->healthcare_available = true;
  this->count_seeking_hc = 0;
  this->count_primary_hc_unav = 0;
  this->count_hc_accept_ins_unav = 0;
  this->group_quarters_units = 0;
  this->group_quarters_workplace = NULL;
  this->migration_admin_code = 0;
  this->in_low_vaccination_school = false;
  this->refuse_vaccine = false;
}

void Household::get_properties() {
}

void Household::set_household_has_hospitalized_member(bool does_have) {
  this->hh_schl_aged_chld_unemplyd_adlt_is_set = false;
  if(does_have) {
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = true;
  } else {
    //Initially say no one is hospitalized
    this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = false;
    //iterate over all Housemates  to see if anyone is still hospitalized
    person_vector_t::iterator it;

    for(it = this->members.begin(); it != this->members.end(); ++it) {
      if((*it)->person_is_hospitalized()) {
        this->not_home_bitset[Household_extended_absence_index::HAS_HOSPITALIZED] = true;
        if(this->get_household_visitation_hospital() == NULL) {
          this->set_household_visitation_hospital(static_cast<Hospital*>((*it)->get_hospital()));
        }
        break;
      }
    }
  }
}

bool Household::has_school_aged_child() {
  //Household has been loaded
  assert(Person::is_load_completed());
  //If the household status hasn't changed, just return the flag
  if(this->hh_schl_aged_chld_unemplyd_adlt_is_set) {
    return this->hh_schl_aged_chld;
  } else {
    bool ret_val = false;
    for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
      Person* per = this->members[i];
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
  assert(Person::is_load_completed());
  //If the household status hasn't changed, just return the flag
  if(this->hh_schl_aged_chld_unemplyd_adlt_is_set) {
    return this->hh_schl_aged_chld_unemplyd_adlt;
  } else {
    bool ret_val = false;
    if(has_school_aged_child()) {
      for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
        Person* per = this->members[i];
        if(per->is_child()) {
          continue;
        }

        //Person is an adult, but is also a student
        if(per->is_adult() && per->is_student()) {
          continue;
        }

        //Person is an adult, but isn't at home
        if(per->is_adult() &&
           (per->person_is_hospitalized() ||
            per->is_college_dorm_resident() ||
            per->is_military_base_resident() ||
            per->is_nursing_home_resident() ||
            per->is_prisoner())) {
          continue;
        }

        //Person is an adult AND is either retired or unemployed
        if(per->is_adult() &&
           (per->get_profile() == Activity_Profile::RETIRED ||
            per->get_profile() == Activity_Profile::UNEMPLOYED)) {
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
    person = get_member(i);
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
	person = get_member(i);
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
	person = get_member(i);
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
	  person = get_member(i);
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
    Person* person = get_member(i);
    printf("%d%c ", person->get_age(), person->get_sex());
    }
    printf("\n");
  */

}

void Household::set_household_vaccination() {
  return;
  // printf("VAX REFUSAL hh %s size %d set_household_vacc_prob entered\n", this->label, this->get_size()); fflush(stdout);
  int n = 0;
  int age = 100;
  for(int i = 0; i < get_size(); ++i) {
    Person* person = this->members[i];
    Place* school = person->get_school();
    if (school != NULL) {
      if (school->is_low_vaccination_place()) {
	double rate = school->get_vaccination_rate();
	// printf("SET_VAX_REFUSAL: hh %s school %s rate %f\n", this->label, school->get_label(), rate);
	if (rate < Random::draw_random(0,1)) {
	  person->set_vaccine_refusal();
	  n++;
	  if (person->get_age() < age) {
	    age = person->get_age();
	  }
	  // printf("SET_VAX_REFUSAL: hh %s person %d age %d school %s rate %f\n", this->label, person->get_id(), person->get_age(), school->get_label(), rate);
	}
	else {
	  // printf("NO_VAX_REFUSAL: hh %s person %d age %d school %s rate %f\n", this->label, person->get_id(), person->get_age(), school->get_label(), rate);
	}
      }
    }
  }

  return;
  // refuse vacination for youunger children is any child has refused
  if (n) {
    for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
      Person* person = this->members[i];
      if (person->get_age() < age) {
	person->set_vaccine_refusal();
	// printf("YOUNGER_REFUSAL: hh %s person %d age %d\n", this->label, person->get_id(), person->get_age());
      }
    }
  }
}


  /*
    int n = 0;
    this->vaccination_probability = Place_Type::get_place_type("School")->get_default_vaccination_rate();
    for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
    Person* person = this->members[i];
    School* school = person->get_school();
    if (school != NULL) {
    n++;
    double rate = school->get_vaccination_rate();
    // FRED_VERBOSE(0, "set_household_vacc_prob n = %d rate = %f\n", n, rate);
    if (rate < this->vaccination_probability) {
    this->vaccination_probability = rate;
    }
    }
    }
    if (0 && n) {
    FRED_VERBOSE(0, "set_household_vacc_prob n = %d vax_prob = %f\n", n, this->vaccination_probability);
    }

    if (this->vaccination_probability < Place_Type::get_place_type("School")->get_default_vaccination_rate()) {
    this->in_low_vaccination_school = true;
    for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
    Person* person = this->members[i];
    School* school = person->get_school();
    if (school != NULL) {
    double rate = school->get_vaccination_rate();
    printf("SCHOOL_VAX: hh %s person %d age %d h_vax %f school %s s_vax %f\n",
    this->label, person->get_id(), person->get_age(), this->vaccination_probability,
    school->get_label(), rate);
    }
    }
    this->refuse_vaccine = (Random::draw_random(0,1) < this->vaccination_probability)==false;
  */

