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
// File: Activities.cc
//
#include "Activities.h"
#include "Age_Map.h"
#include "Classroom.h"
#include "Global.h"
#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "County.h"
#include "Geo.h"
#include "Global.h"
#include "Household.h"
#include "Mixing_Group.h"
#include "Neighborhood.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Network.h"
#include "Office.h"
#include "Params.h"
#include "Person.h"
#include "Person_Network_Link.h"
#include "Place.h"
#include "Place_List.h"
#include "Random.h"
#include "School.h"
#include "Travel.h"
#include "Utils.h"
#include "Workplace.h"

bool Activities::is_initialized = false;
bool Activities::is_weekday = false;
int Activities::day_of_week = 0;
double Activities::Work_absenteeism = 0.0;
double Activities::School_absenteeism = 0.0;

// run-time parameters
Age_Map* Activities::Hospitalization_prob = NULL;
Age_Map* Activities::Outpatient_healthcare_prob = NULL;

Activities_Tracking_Data Activities::Tracking_data;

const char* get_label_for_place(Place* place) {
  return (place != NULL ? place->get_label() : "NULL");
}

/// static method called in main (Fred.cc)
void Activities::initialize_static_variables() {

  if(Activities::is_initialized) {
    return;
  }

  FRED_STATUS(0, "Activities::initialize() entered\n", "");

  // read optional parameters
  Params::disable_abort_on_failure();
  
  Params::get_param("work_absenteeism", &Activities::Work_absenteeism);
  Params::get_param("school_absenteeism", &Activities::School_absenteeism);

  // restore requiring parameters
  Params::set_abort_on_failure();

  Activities::is_initialized = true;
}

Activities::Activities() {
  this->myself = NULL;
  this->sick_leave_available = false;
  this->sick_days_remaining = 0.0;
  this->my_sick_leave_decision_has_been_made = false;
  this->my_sick_leave_decision = false;
  this->home_neighborhood = NULL;
  this->profile = UNDEFINED_PROFILE;
  this->schedule_updated = -1;
  this->stored_daily_activity_locations = NULL;
  this->primary_healthcare_facility = NULL;
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->is_confined_due_to_contact_tracing = false;
  this->is_hospitalized = false;
  this->sim_day_hospitalization_ends = -1;
  this->last_school = NULL;
  this->grade = 0;
  this->return_from_travel_sim_day = -1;
  this->in_parents_home = false;
  this->link = new Person_Place_Link [Activity_index::DAILY_ACTIVITY_LOCATIONS];
  this->networks.clear();
}

void Activities::setup(Person* self, Place* house, Place* school, Place* work) {

  FRED_VERBOSE(1, "ACTIVITIES_SETUP: person %d age %d household %s\n",
	       self->get_id(), self->get_age(), house->get_label());

  this->myself = self;
  clear_daily_activity_locations();

  FRED_VERBOSE(1, "set household %s\n", get_label_for_place(house));
  set_household(house);

  FRED_VERBOSE(1, "set school %s\n", get_label_for_place(school));
  set_school(school);

  FRED_VERBOSE(1, "set workplace %s\n", get_label_for_place(work));
  set_workplace(work);
  FRED_VERBOSE(1, "set workplace %s ok\n", get_label_for_place(work));

  // increase the population in county of residence
  int fips = get_household()->get_county_fips();
  Global::Places.increment_population_of_county(fips, this->myself);

  // get the neighborhood from the household
  set_neighborhood(get_household()->get_patch()->get_neighborhood());
  FRED_VERBOSE(1, "ACTIVITIES_SETUP: person %d neighborhood %d %s\n", this->myself->get_id(),
	       get_neighborhood()->get_id(), get_neighborhood()->get_label());
  FRED_CONDITIONAL_VERBOSE(0, get_neighborhood() == NULL,
			   "Help! NO NEIGHBORHOOD for person %d house %d \n", this->myself->get_id(), get_household()->get_id());
  this->home_neighborhood = get_neighborhood();

  // assign profile
  assign_initial_profile();
  FRED_VERBOSE(1,"set profile ok\n");

  // need to set the daily schedule
  this->schedule_updated = -1;
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->is_confined_due_to_contact_tracing = false;

  // sick leave variables
  this->my_sick_leave_decision_has_been_made = false;
  this->my_sick_leave_decision = false;
  this->sick_days_remaining = 0.0;
  this->sick_leave_available = false;

  if(self->lives_in_group_quarters()) {
    int day = Global::Simulation_Day;
    // no pregnancies in group_quarters
    if(self->get_demographics()->is_pregnant()) {
      printf("GQ CANCELS PREGNANCY: today %d person %d age %d due %d\n",
	     day, self->get_id(), self->get_age(), self->get_demographics()->get_maternity_sim_day());
      self->get_demographics()->cancel_pregnancy(self);
    }
    // cancel any planned pregnancy
    if(day <= self->get_demographics()->get_conception_sim_day()) {
      printf("GQ CANCELS PLANNED CONCEPTION: today %d person %d age %d conception %d\n",
	     day, self->get_id(), self->get_age(), self->get_demographics()->get_conception_sim_day());
      self->get_demographics()->cancel_conception(self);
    }
  }
  FRED_VERBOSE(1,"Activity::setup finished for person %d\n", self->get_id());

}

void Activities::prepare() {
  initialize_sick_leave();
  // clear confinement to household
  is_confined_for_condition = new bool [ Global::Conditions.get_number_of_conditions() ];
  for(int condition_id = 0; condition_id < Global::Conditions.get_number_of_conditions(); ++condition_id) {
    is_confined_for_condition[condition_id] = false;
  }
  is_confined_due_to_contact_tracing = false;
}

void Activities::initialize_sick_leave() {
  FRED_VERBOSE(1, "Activities::initialize_sick_leave entered\n");

  //Only initalize for those who are working
  if(get_workplace() != NULL || (is_teacher() && get_school() != NULL)) {
    this->my_sick_leave_decision_has_been_made = false;
    this->my_sick_leave_decision = false;
    this->sick_days_remaining = 0.0;
    this->sick_leave_available = false;
    FRED_VERBOSE(1, "Activities::initialize_sick_leave sick_days_remaining = %d\n", this->sick_days_remaining);
  }
}

void Activities::before_run() {
}

void Activities::report(int day) {
}

void Activities::assign_initial_profile() {
  int age = this->myself->get_age();
  if(age == 0) {
    this->profile = PRESCHOOL_PROFILE;
    this->in_parents_home = true;
  } else if(get_school() != NULL) {
    this->profile = STUDENT_PROFILE;
    this->in_parents_home = true;
  } else if(age < Global::SCHOOL_AGE) {
    this->profile = PRESCHOOL_PROFILE;		// child at home
    this->in_parents_home = true;
  } else if(age < Global::ADULT_AGE) {
    this->profile = STUDENT_PROFILE;		// school age
    this->in_parents_home = true;
  } else if(get_workplace() != NULL) {
    this->profile = WORKER_PROFILE;		// worker
  } else if(age < Global::RETIREMENT_AGE) {
    this->profile = WORKER_PROFILE;		// worker
  } else if(Global::RETIREMENT_AGE <= age) {
    if(Random::draw_random()< 0.5 ) {
      this->profile = RETIRED_PROFILE;		// retired
    }
    else {
      this->profile = WORKER_PROFILE;		// older worker
    }
  } else {
    this->profile = UNEMPLOYED_PROFILE;
  }

  if(this->profile == WORKER_PROFILE || this->profile == UNEMPLOYED_PROFILE) {
    // determine if I am living in parents house
    int rel = myself->get_demographics()->get_relationship();
    if (rel == Global::CHILD || rel == Global::GRANDCHILD || rel == Global::FOSTER_CHILD) {
      this->in_parents_home = true;
    }
  }  

  // weekend worker
  if((this->profile == WORKER_PROFILE && Random::draw_random() < 0.2)) {  // 20% weekend worker
    this->profile = WEEKEND_WORKER_PROFILE;
  }

  // profiles for group quarters residents
  if(get_household()->is_college()) {
    this->profile = COLLEGE_STUDENT_PROFILE;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_military_base()) {
    this->profile = MILITARY_PROFILE;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_prison()) {
    this->profile = PRISONER_PROFILE;
    FRED_VERBOSE(2, "INITIAL PROFILE AS PRISONER ID %d AGE %d SEX %c HOUSEHOLD %s\n",
		 this->myself->get_id(), age, this->myself->get_sex(), get_household()->get_label());
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_nursing_home()) {
    this->profile = NURSING_HOME_RESIDENT_PROFILE;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
}


void Activities::update(int sim_day) {
  FRED_STATUS(1, "Activities update entered\n");
  // decide if this is a weekday:
  Activities::is_weekday = Date::is_weekday();
  FRED_STATUS(1, "Activities update completed\n");
}


void Activities::update_daily_activities(int sim_day) {

  FRED_VERBOSE(1,"update_activities for person %d day %d\n", this->myself->get_id(), sim_day);

  // skip all scheduled activities if traveling abroad
  if(this->is_traveling_outside) {
    return;
  }

  if(sim_day > this->schedule_updated) {

    // get list of places to visit today
    update_schedule(sim_day);

    // decide which neighborhood to visit today
    if(this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY]) {
      Place* destination_neighborhood = Global::Neighborhoods->select_destination_neighborhood(this->home_neighborhood);
      set_neighborhood(destination_neighborhood);
      // FRED_VERBOSE(0,"update_activities for person %d day %d nbhd %s\n",
      // this->myself->get_id(), sim_day, destination_neighborhood->get_label());
    }

    bool stay_home = false;
    
    if (this->is_confined_due_to_contact_tracing) {
      this->my_sick_leave_decision_has_been_made = true;
      this->my_sick_leave_decision = true;
      if (Global::Enable_Health_Records) {
	fprintf(Global::HealthRecordfp,
		"HEALTH RECORD: %s day %d person %d CONFINED_TO_HOUSEHOLD due to CONTACT_TRACING\n",
		Date::get_date_string().c_str(),
		sim_day,
		this->myself->get_id());
      }
    }

    if (this->my_sick_leave_decision_has_been_made) {
      stay_home = this->my_sick_leave_decision;
    }
    else {
      for (int i = 0; stay_home==false && i < Global::Conditions.get_number_of_conditions(); i++) {
	if (Global::Conditions.get_condition(i)->is_household_confinement_enabled(this->myself)) {

	  if (Global::Conditions.get_condition(i)->get_daily_household_confinement(this->myself)) {
	    this->myself->clear_household_confinement(i);
	    stay_home = Global::Conditions.get_condition(i)->is_confined_to_household(this->myself);
	    this->my_sick_leave_decision = stay_home;
	    this->my_sick_leave_decision_has_been_made = false;
	  }
	  else {
	    stay_home = Global::Conditions.get_condition(i)->is_confined_to_household(this->myself);
	    this->my_sick_leave_decision = stay_home;
	    this->my_sick_leave_decision_has_been_made = true;
	  }
	  if (Global::Enable_Health_Records && stay_home) {
	    fprintf(Global::HealthRecordfp,
		    "HEALTH RECORD: %s day %d person %d CONFINED_TO_HOUSEHOLD due to COND %d daily_update %d\n",
		    Date::get_date_string().c_str(),
		    sim_day,
		    this->myself->get_id(), i,
		    Global::Conditions.get_condition(i)->get_daily_household_confinement(this->myself)? 1 : 0);
	  }
	}
      }
    }
    
    if(stay_home) {
      // withdraw to household
      this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
      this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
      this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
      this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
      this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = false;
      this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = false;
      this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = false;
    }
  }
}

void Activities::update_schedule(int sim_day) {

  // update this schedule only once per day
  if(sim_day <= this->schedule_updated) {
    return;
  }

  this->schedule_updated = sim_day;
  this->on_schedule.reset();

  if(this->is_confined_due_to_contact_tracing) {
    // always visit the household
    this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;
    return;
  }

  if(Global::Enable_Hospitals && this->is_hospitalized && !(this->sim_day_hospitalization_ends == sim_day)) {
    // only visit the hospital
    this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
  } else {
    //If the hospital stay should end today, go back to normal
    if(Global::Enable_Hospitals && this->is_hospitalized && (this->sim_day_hospitalization_ends == sim_day)) {
      end_hospitalization();
    }

    // always visit the household
    this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;

    // decide whether to visit the neighborhood
    if(this->profile == PRISONER_PROFILE || this->profile == NURSING_HOME_RESIDENT_PROFILE) {
      // prisoners and nursing home residents stay indoors
      this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = false;
    } else {
      // provisionally visit the neighborhood
      this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = true;
    }

    // weekday vs weekend provisional activity
    if(Activities::is_weekday) {
      if(get_school() != NULL) {
        this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = true;
      }
      if(get_classroom() != NULL) {
        this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = true;
      }
      if(get_workplace() != NULL) {
        this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = true;
      }
      if(get_office() != NULL) {
        this->on_schedule[Activity_index::OFFICE_ACTIVITY] = true;
      }
    } else {
      if(this->profile == WEEKEND_WORKER_PROFILE || this->profile == STUDENT_PROFILE) {
        if(get_workplace() != NULL) {
          this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = true;
        }
        if(get_office() != NULL) {
          this->on_schedule[Activity_index::OFFICE_ACTIVITY] = true;
        }
      } else if(this->is_hospital_staff()) {
        if(Random::draw_random() < 0.4) {
          if(get_workplace() != NULL) {
            this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = true;
          }
          if(get_office() != NULL) {
            this->on_schedule[Activity_index::OFFICE_ACTIVITY] = true;
          }
        }
      }
    }

    // skip work at background absenteeism rate
    if(Activities::Work_absenteeism > 0.0 && this->on_schedule[Activity_index::WORKPLACE_ACTIVITY]) {
      if(Random::draw_random() < Activities::Work_absenteeism) {
        this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
        this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
      }
    }

    // skip school at background school absenteeism rate
    if(Activities::School_absenteeism > 0.0 && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
      if(Random::draw_random() < Activities::School_absenteeism) {
        this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
        this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
      }
    }

  }
  FRED_STATUS(1, "update_schedule on day %d\n%s\n", sim_day, schedule_to_string(sim_day).c_str());
}

void Activities::start_hospitalization(int sim_day, int length_of_stay) {
  assert(length_of_stay > 0);
  if(Global::Enable_Hospitals) {
    assert(!this->myself->is_hospitalized());
    //If agent is traveling, return home first
    if(this->is_traveling || this->is_traveling_outside) {
      stop_traveling();
    }

    // get agent's preferred hospital
    Hospital* hosp = get_hospital();

    //If no preferred hospital, then use the household's hospital
    Household* hh = get_household();
    assert(hh != NULL);
    if(hosp == NULL) {
      hosp = hh->get_household_visitation_hospital();
      assert(hosp != NULL);
    } else if(hosp->is_healthcare_clinic() ||
              hosp->is_mobile_healthcare_clinic()) {
      hosp = hh->get_household_visitation_hospital();
      assert(hosp != NULL);
    }

    if(hosp != hh->get_household_visitation_hospital()) {
      //Change the household visitation hospital so that visitors go to the right place
      hh->set_household_visitation_hospital(hosp);
    }

    if(hosp->get_occupied_bed_count() < hosp->get_bed_count(sim_day)) {
      store_daily_activity_locations();
      clear_daily_activity_locations();
      this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = false;
      this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
      this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
      this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
      this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
      this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = false;
      this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
      this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = false;
      assign_hospital(hosp);

      this->is_hospitalized = true;
      this->sim_day_hospitalization_ends = sim_day + length_of_stay;
      hosp->increment_occupied_bed_count();

      //Set the flag for the household
      hh->set_household_has_hospitalized_member(true);
    } else {
      //No room in the hospital
      //TODO: should we do something else?
    }

  }
}

void Activities::end_hospitalization() {
  if(Global::Enable_Hospitals) {
    this->is_hospitalized = false;
    this->sim_day_hospitalization_ends = -1;
    Hospital* tmp_hosp = get_hospital();
    assert(tmp_hosp != NULL);
    tmp_hosp->decrement_occupied_bed_count();
    restore_daily_activity_locations();

    //Set the flag for the household
    get_household()->set_household_has_hospitalized_member(false);
  }
}

void Activities::print_schedule(int day) {
  FRED_STATUS(0, "%s\n", this->schedule_to_string(day).c_str());
}

void Activities::print() {
  FRED_STATUS(0, "%s\n", this->to_string().c_str());
}

void Activities::assign_school() {
  int day = Global::Simulation_Day;
  School* old_school = this->last_school;
  int grade = this->myself->get_age();
  FRED_VERBOSE(1, "assign_school entered for person %d age %d grade %d\n",
	       this->myself->get_id(), this->myself->get_age(), grade);

  Household* hh = get_household();

  if(hh == NULL) {
    if(Global::Enable_Hospitals && this->myself->is_hospitalized() && get_permanent_household() != NULL) {
      hh = get_permanent_household();
    }
  }
  assert(hh != NULL);

  Place* school = NULL;
  school = Global::Places.get_census_tract(hh->get_census_tract_fips())->select_new_school(grade);
  if (school == NULL) {
    school = Global::Places.get_county(hh->get_county_fips())->select_new_school(grade);
    FRED_VERBOSE(1, "DAY %d ASSIGN_SCHOOL FROM COUNTY %s selected for person %d age %d\n",
		 day, school->get_label(), this->myself->get_id(), this->myself->get_age());
  }
  else {
    FRED_VERBOSE(1, "DAY %d ASSIGN_SCHOOL FROM CENSUS_TRACT %s selected for person %d age %d\n",
		 day, school->get_label(), this->myself->get_id(), this->myself->get_age());
  }
  if (school == NULL) {
    school = Global::Places.get_random_school(grade);
  }
  assert(school != NULL);
  set_school(school);
  set_classroom(NULL);
  assign_classroom();
  FRED_VERBOSE(1, "assign_school finished for person %d age %d: school %s classroom %s\n",
               this->myself->get_id(), this->myself->get_age(),
	       get_school()->get_label(), get_classroom()->get_label());
}

void Activities::assign_classroom() {
  if(School::get_max_classroom_size() == 0) {
    return;
  }
  assert(get_school() != NULL && get_classroom() == NULL);
  FRED_VERBOSE(1, "assign classroom entered\n");

  School* school = get_school();
  Place* place = school->select_classroom_for_student(this->myself);
  if (place == NULL) {
    FRED_VERBOSE(0, "CLASSROOM_WARNING: assign classroom returns NULL: person %d age %d school %s\n",
		 this->myself->get_id(), this->myself->get_age(), school->get_label());
  }
  set_classroom(place);
  FRED_VERBOSE(1,"assign classroom finished\n");
}

void Activities::assign_workplace() {
  Household* hh;
  if(Global::Enable_Hospitals && this->is_hospitalized) {
    hh = get_permanent_household();
  } else {
    hh = get_household();
  }
  Place* p = NULL;
  p = Global::Places.get_census_tract(hh->get_census_tract_fips())->select_new_workplace();
  if (p == NULL) {
    p = Global::Places.get_county(hh->get_county_fips())->select_new_workplace();
    FRED_VERBOSE(1, "ASSIGN_WORKPLACE FROM COUNTY %s selected for person %d age %d\n",
		 p->get_label(), this->myself->get_id(), this->myself->get_age());
  }
  else {
    FRED_VERBOSE(1, "ASSIGN_WORKPLACW FROM CENSUS_TRACT %s selected for person %d age %d\n",
		 p->get_label(), this->myself->get_id(), this->myself->get_age());
  }
  change_workplace(p);
}

void Activities::assign_office() {
  if(get_workplace() != NULL && get_office() == NULL && get_workplace()->is_workplace()
     && Workplace::get_max_office_size() > 0) {
    Place* place = get_workplace()->assign_office(this->myself);
    if(place == NULL) {
      FRED_VERBOSE(0, "OFFICE WARNING: No office assigned for person %d workplace %d\n", this->myself->get_id(),
		   get_workplace()->get_id());
    }
    set_office(place);
  }
}

void Activities::assign_primary_healthcare_facility() {
  Hospital* tmp_hosp = Global::Places.get_random_primary_care_facility_matching_criteria(this->myself, (Global::Enable_Health_Insurance && true), true);
  if(tmp_hosp != NULL) {
    this->primary_healthcare_facility = tmp_hosp;
    Place_List::increment_hospital_ID_current_assigned_size_map(tmp_hosp->get_id());
  } else {
    //Expand search radius
    tmp_hosp = Global::Places.get_random_primary_care_facility_matching_criteria(this->myself, Global::Enable_Health_Insurance, false);
    if(tmp_hosp != NULL) {
      this->primary_healthcare_facility = tmp_hosp;
      Place_List::increment_hospital_ID_current_assigned_size_map(tmp_hosp->get_id());
    } else {
      //Don't use health insurance even if it is enabled
      tmp_hosp = Global::Places.get_random_primary_care_facility_matching_criteria(this->myself, false, false);
      if(tmp_hosp != NULL) {
        this->primary_healthcare_facility = tmp_hosp;
        Place_List::increment_hospital_ID_current_assigned_size_map(tmp_hosp->get_id());
      }
    }
  }
}

void Activities::assign_hospital(Place* place) {
  if(place == NULL) {
    FRED_VERBOSE(1, "Warning! No Hospital Place assigned for person %d\n", this->myself->get_id());
  }
  set_hospital(place);
}

void Activities::assign_ad_hoc_place(Place* place) {
  if(place == NULL) {
    FRED_VERBOSE(1, "Warning! No Ad Hoc Place assigned for person %d\n", this->myself->get_id());
  }
  set_ad_hoc(place);
}

void Activities::unassign_ad_hoc_place() {
  set_ad_hoc(NULL);
}

int Activities::get_degree() {
  int degree;
  int n;
  degree = 0;
  n = get_group_size(Activity_index::NEIGHBORHOOD_ACTIVITY);
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Activity_index::SCHOOL_ACTIVITY);
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Activity_index::WORKPLACE_ACTIVITY);
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Activity_index::HOSPITAL_ACTIVITY);
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Activity_index::AD_HOC_ACTIVITY);
  if(n > 0) {
    degree += (n - 1);
  }
  return degree;
}

int Activities::get_group_size(int index) {
  int size = 0;
  if(get_daily_activity_location(index) != NULL) {
    size = get_daily_activity_location(index)->get_size();
  }
  return size;
}

bool Activities::is_hospital_staff() {
  bool ret_val = false;

  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_hospital() &&
         !get_household()->is_hospital()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Activities::is_prison_staff() {
  bool ret_val = false;

  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_prison() &&
         !get_household()->is_prison()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Activities::is_college_dorm_staff() {
  bool ret_val = false;

  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_college() &&
         !get_household()->is_college()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Activities::is_military_base_staff() {
  bool ret_val = false;

  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_military_base() &&
         !get_household()->is_military_base()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Activities::is_nursing_home_staff() {
  bool ret_val = false;

  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_nursing_home() &&
         !get_household()->is_nursing_home()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

void Activities::update_profile_after_changing_household() {
  int age = this->myself->get_age();
  int day = Global::Simulation_Day;

  // printf("person %d house %s subtype %c\n", this->myself->get_id(), get_household()->get_label(), get_household()->get_subtype());

  // profiles for new group quarters residents
  if(get_household()->is_college()) {
    if(this->profile != COLLEGE_STUDENT_PROFILE) {
      this->profile = COLLEGE_STUDENT_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household(get_household()->get_index())->get_group_quarters_workplace());
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE TO COLLEGE_STUDENT: person %d age %d DORM %s\n",
		   this->myself->get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_military_base()) {
    if(this->profile != MILITARY_PROFILE) {
      this->profile = MILITARY_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household(get_household()->get_index())->get_group_quarters_workplace());
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE TO MILITARY: person %d age %d BARRACKS %s\n",
		   this->myself->get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_prison()) {
    if(this->profile != PRISONER_PROFILE) {
      this->profile = PRISONER_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household(get_household()->get_index())->get_group_quarters_workplace());
      FRED_VERBOSE(1,"AFTER_MOVE CHANGING PROFILE TO PRISONER: person %d age %d PRISON %s\n",
		   this->myself->get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_nursing_home()) {
    if(this->profile != NURSING_HOME_RESIDENT_PROFILE) {
      this->profile = NURSING_HOME_RESIDENT_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household(get_household()->get_index())->get_group_quarters_workplace());
      FRED_VERBOSE(1,"AFTER_MOVE CHANGING PROFILE TO NURSING HOME: person %d age %d NURSING_HOME %s\n",
		   this->myself->get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  // former military
  if(this->profile == MILITARY_PROFILE && get_household()->is_military_base() == false) {
    change_school(NULL);
    change_workplace(NULL);
    this->profile = WORKER_PROFILE;
    assign_workplace();
    initialize_sick_leave();
    FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM MILITRAY TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		 this->myself->get_id(), age, this->myself->get_sex(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

  // former prisoner
  if(this->profile == PRISONER_PROFILE && get_household()->is_prison() == false) {
    change_school(NULL);
    change_workplace(NULL);
    this->profile = WORKER_PROFILE;
    assign_workplace();
    initialize_sick_leave();
    FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM PRISONER TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		 this->myself->get_id(), age, this->myself->get_sex(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

  // former college student
  if(this->profile == COLLEGE_STUDENT_PROFILE && get_household()->is_college() == false) {
    if(Random::draw_random() < 0.25) {
      // time to leave college for good
      change_school(NULL);
      change_workplace(NULL);
      // get a job
      this->profile = WORKER_PROFILE;
      assign_workplace();
      initialize_sick_leave();
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM COLLEGE STUDENT TO WORKER: id %d age %d sex %c HOUSE %s WORKPLACE %s OFFICE %s\n",
		   this->myself->get_id(), age, this->myself->get_sex(), get_household()->get_label(), get_workplace()->get_label(), get_office()->get_label());
    }
    return;
  }

  // school age => select a new school
  if(this->profile == STUDENT_PROFILE && age < Global::ADULT_AGE) {
    School* school = get_school();
    School* old_school = this->last_school;
    int grade = this->myself->get_age();
    Household* hh = get_household();
    if(hh == NULL) {
      if(Global::Enable_Hospitals && this->myself->is_hospitalized() && get_permanent_household() != NULL) {
	hh = get_permanent_household();
      }
    }
    assert(hh != NULL);

    // FRED_VERBOSE(0, "DAY %d AFTER_MOVE checking school status\n", day);
    // stay in current school if this grade is available and is attended by student in this census tract
    Census_Tract* ct = Global::Places.get_census_tract(hh->get_census_tract_fips());
    if (school != NULL && ct->is_school_attended(school->get_id(), grade)) {
      set_classroom(NULL);
      assign_classroom();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AFTER_MOVE STAY IN CURRENT SCHOOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, this->myself->get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_orig_size(), get_classroom()->get_label());
    }
    else {
      // select a new school
      change_school(NULL);
      change_workplace(NULL);
      assign_school();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AFTER_MOVE SELECT NEW SCHOOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, this->myself->get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_orig_size(), get_classroom()->get_label());
    }
    return;
  }


  // worker => select a new workplace
  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    change_school(NULL);
    Place* old_workplace = get_workplace();
    change_workplace(NULL);
    assign_workplace();
    initialize_sick_leave();
    FRED_VERBOSE(1, "AFTER_MOVE SELECT NEW WORKPLACE: person %d age %d sex %c OLD WORKPLACE %s NEW WORKPLACE %s OFFICE %s\n",
		 this->myself->get_id(), age, this->myself->get_sex(),
		 old_workplace == NULL ? "NULL" : old_workplace->get_label(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

}

void Activities::update_profile_based_on_age() {
  int age = this->myself->get_age();
  int day = Global::Simulation_Day;

  // pre-school children entering school
  if(this->profile == PRESCHOOL_PROFILE && Global::SCHOOL_AGE <= age && age < Global::ADULT_AGE) {
    this->profile = STUDENT_PROFILE;
    change_school(NULL);
    change_workplace(NULL);
    assign_school();
    assert(get_school() && get_classroom());
    FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE FROM PRESCHOOL TO STUDENT: person %d age %d SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                 this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
		 get_school()->get_orig_size(), get_classroom()->get_label());
    Activities::Tracking_data.entered_school++;
    return;
  }

  // school age
  if(this->profile == STUDENT_PROFILE && age < Global::ADULT_AGE) {
    School* old_school = this->last_school;
    School* school = get_school();
    int grade = this->myself->get_age();
    // FRED_VERBOSE(0, "DAY %d AGE_UP checking school status, age = %d\n", day, age);
    // stay in current school if this grade is available
    if (school != NULL && school->get_classrooms_in_grade(grade) > 0) {
      FRED_VERBOSE(1, "DAY %d AGE_UP checking school status, age = %d classroms %d\n", day, age, get_school()->get_classrooms_in_grade(grade));
      set_classroom(NULL);
      assign_classroom();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AGE_UP STAY IN SCHHOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, this->myself->get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_orig_size(), get_classroom()->get_label());
    }
    else {
      change_school(NULL);
      change_workplace(NULL);
      assign_school();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AGE_UP KEEPING STUDENT PROFILE: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day,  this->myself->get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_orig_size(), get_classroom()->get_label());
    }
    return;
  }


  // graduating from school
  if(this->profile == STUDENT_PROFILE && Global::ADULT_AGE <= age) {
    School* old_school = this->last_school;
    change_school(NULL);
    change_workplace(NULL);
    this->profile = WORKER_PROFILE;
    assign_workplace();
    initialize_sick_leave();
    FRED_VERBOSE(1, "DAY %d AGE_UP CHANGING PROFILE FROM STUDENT TO WORKER: person %d age %d LAST_SCHOOL %s sex %c WORKPLACE %s OFFICE %s\n",
                 day, this->myself->get_id(), age,
		 old_school == NULL ? "NULL" : old_school->get_label(),
		 this->myself->get_sex(), get_workplace()->get_label(), get_office()->get_label());
    return;
  }


  // unemployed worker re-entering workplace
  if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
    if(get_workplace() == NULL) {
      assign_workplace();
      initialize_sick_leave();
      FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE FROM UNEMPLOYED TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		   this->myself->get_id(), age, this->myself->get_sex(), get_workplace()->get_label(), get_office()->get_label());
    }
    // NOTE: no return;
  }

  // possibly entering retirement
  if(this->profile != RETIRED_PROFILE && Global::RETIREMENT_AGE <= age && get_household()->is_group_quarters()==false) {
    if(Random::draw_random()< 0.5 ) {
      // quit working
      if(is_teacher()) {
        change_school(NULL);
      }
      change_workplace(NULL);
      this->profile = RETIRED_PROFILE;
      initialize_sick_leave(); // no sick leave available if retired
      FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE TO RETIRED: person %d age %d sex\n",
		   this->myself->get_id(), age, this->myself->get_sex());
    }
    return;
  }

}


void Activities::start_traveling(Person* visited) {

  //Can't travel if hospitalized
  if(Global::Enable_Hospitals && this->is_hospitalized) {
    return;
  }

  //Notify the household that someone is not going to be there
  if(Global::Report_Childhood_Presenteeism) {
    Household* hh = get_household();
    if(hh != NULL) {
      hh->set_hh_schl_aged_chld_unemplyd_adlt_is_set(false);
    }
  }

  if(visited == NULL) {
    this->is_traveling_outside = true;
  } else {
    store_daily_activity_locations();
    clear_daily_activity_locations();
    set_household(visited->get_household());
    set_neighborhood(visited->get_neighborhood());
    if(this->profile == WORKER_PROFILE) {
      set_workplace(visited->get_workplace());
      set_office(visited->get_office());
    }
  }
  this->is_traveling = true;
  FRED_STATUS(1, "start traveling: id = %d\n", this->myself->get_id());
}

void Activities::stop_traveling() {
  if(!this->is_traveling_outside) {
    restore_daily_activity_locations();
  }
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->return_from_travel_sim_day = -1;
  if(Global::Report_Childhood_Presenteeism) {
    Household* hh = get_household();
    if(hh != NULL) {
      hh->set_hh_schl_aged_chld_unemplyd_adlt_is_set(false);
    }
  }
  FRED_STATUS(1, "stop traveling: id = %d\n", this->myself->get_id());
}

bool Activities::become_a_teacher(Place* school) {
  bool success = false;
  FRED_VERBOSE(1, "become_a_teacher: person %d age %d\n", this->myself->get_id(), this->myself->get_age());
  // print(self);
  if(get_school() != NULL) {
    if(Global::Verbose > 1) {
      FRED_WARNING("become_a_teacher: person %d age %d ineligible -- already goes to school %d %s\n",
		   this->myself->get_id(), this->myself->get_age(), get_school()->get_id(), get_school()->get_label());
    }
    this->profile = STUDENT_PROFILE;
  } else {
    // set profile
    this->profile = TEACHER_PROFILE;
    // join the school
    FRED_VERBOSE(1, "set school to %s\n", school->get_label());
    set_school(school);
    set_classroom(NULL);
    success = true;
  }
  
  // withdraw from this workplace and any associated office
  Place* workplace = get_workplace();
  FRED_VERBOSE(1, "leaving workplace %d %s\n", workplace->get_id(), workplace->get_label());
  change_workplace(NULL);
  FRED_VERBOSE(1, "become_a_teacher finished for person %d age %d  school %s\n",
	       this->myself->get_id(),
	       this->myself->get_age(),
	       school->get_label());
  // print(self);
  return success;
}

void Activities::change_school(Place* place) {
  FRED_VERBOSE(1, "person %d set school %s\n", myself->get_id(), place ? place->get_label() : "NULL");
  set_school(place);
  FRED_VERBOSE(1,"set classroom to NULL\n");
  set_classroom(NULL);
  if(place != NULL) {
    FRED_VERBOSE(1, "assign classroom\n");
    assign_classroom();
  }
}

void Activities::change_workplace(Place* place, int include_office) {
  FRED_VERBOSE(1, "person %d set workplace %s\n", this->myself->get_id(), place ? place->get_label() : "NULL");
  set_workplace(place);
  set_office(NULL);
  if(place != NULL) {
    if(include_office) {
      assign_office();
    }
  }
}

std::string Activities::schedule_to_string(int day) {
  std::stringstream ss;
  ss << "day " << day << " schedule for person " << this->myself->get_id() << "  ";
  for(int p = 0; p < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++p) {
    if(get_daily_activity_location(p)) {
      ss << activity_lookup(p) << ": ";
      ss << (this->on_schedule[p] ? "+" : "-");
      ss << get_daily_activity_location_id(p) << " ";
    }
  }
  return ss.str();
}

std::string Activities::to_string() {
  std::stringstream ss;
  ss << "Activities for person " << this->myself->get_id() << ": ";
  for(int p = 0; p < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++p) {
    if(get_daily_activity_location(p)) {
      ss << activity_lookup(p) << ": ";
      ss << get_daily_activity_location_id(p) << " ";
    }
  }
  return ss.str();
}

void Activities::change_household(Place* house) {

  // household can not be NULL
  assert(house != NULL); 

  FRED_VERBOSE(1, "move_to_new_house start person %d house %s subtype %c\n",
	       this->myself->get_id(), house->get_label(), house->get_subtype());

  // update county census counts
  int old_fips = get_household()->get_county_fips();
  int new_fips = house->get_county_fips();
  Global::Places.decrement_population_of_county(old_fips, this->myself);
  Global::Places.increment_population_of_county(new_fips, this->myself);

  // change household
  set_household(house);

  // set neighborhood
  set_neighborhood(house->get_patch()->get_neighborhood());

  // possibly update the profile depending on new household
  update_profile_after_changing_household();

  FRED_VERBOSE(1, "move_to_new_house finished person %d house %s subtype %c profile %c\n",
	       this->myself->get_id(), this->myself->get_household()->get_label(),
	       this->myself->get_household()->get_subtype(), this->profile);
}

void Activities::terminate() {
  if(get_travel_status()) {
    if(this->is_traveling && !this->is_traveling_outside) {
      restore_daily_activity_locations();
    }
    Travel::terminate_person(this->myself);
  }

  //If the agent was hospitalized, restore original daily activity locations
  if(this->is_hospitalized) {
    restore_daily_activity_locations();
  }

  // decrease the population in county of residence
  int fips = get_household()->get_county_fips();
  Global::Places.decrement_population_of_county(fips, this->myself);

  // withdraw from society
  unenroll_from_daily_activity_locations();
}

void Activities::end_of_run() {
  if(Global::Report_Presenteeism || Global::Report_Childhood_Presenteeism) {
    double mean_sick_days_used = (Activities::Tracking_data.total_employees_taking_sick_leave == 0 ? 0.0 : (Activities::Tracking_data.total_employees_days_used / static_cast<double>(Activities::Tracking_data.total_employees_taking_sick_leave)));
    FRED_STATUS(0, "Sick Leave Report: %d Employees Used Sick Leave for an average of %0.2f days each\n", Activities::Tracking_data.total_employees_taking_sick_leave, mean_sick_days_used);
  }
}

int Activities::get_visiting_health_status(Place* place, int day, int condition_id) {

  // assume we are not visiting this place today
  int status = 0;

  // traveling abroad?
  if(this->is_traveling_outside) {
    return status;
  }

  if(day > this->schedule_updated) {
    // get list of places to visit today
    update_schedule(day);

    // noninfectious people stay in neighborhood
    set_neighborhood(this->home_neighborhood);
  }

  // see if the given place is on my schedule today
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    if(this->on_schedule[i] && get_daily_activity_location(i) == place) {
      if(this->myself->is_susceptible(condition_id)) {
        status = 1;
        break;
      } else if(this->myself->is_infectious(condition_id)) {
        status = 2;
        break;
      } else {
        status = 3;
        break;
      }
    }
  }
  return status;
}

void Activities::update_enrollee_index(Mixing_Group* mixing_group, int new_index) {
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    if(mixing_group == get_daily_activity_location(i)) {
      FRED_VERBOSE(1, "update_enrollee_index for person %d i %d new_index %d\n", this->myself->get_id(), i, new_index);
      this->link[i].update_enrollee_index(new_index);
      return;
    }
  }
  FRED_VERBOSE(0, "update_enrollee_index: person %d place %d %s not found in daily activity locations: ",
               this->myself->get_id(), mixing_group->get_id(), mixing_group->get_label());
  
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    Place* place = get_daily_activity_location(i);
    printf("%s ", place ? place->get_label() : "NULL");
  }
  printf("\n");
  assert(0);
}

///////////////////////////////////

void Activities::clear_daily_activity_locations() {
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    if(this->link[i].is_enrolled()) {
      this->link[i].unenroll(this->myself);
    }
    assert(this->link[i].get_place() == NULL);
  }
}
  
void Activities::enroll_in_daily_activity_location(int i) {
  Place* place = get_daily_activity_location(i);
  if(place != NULL) {
    this->link[i].enroll(this->myself, place);
  }
}

void Activities::enroll_in_daily_activity_locations() {
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    enroll_in_daily_activity_location(i);
  }
}

void Activities::unenroll_from_daily_activity_location(int i) {
  Place* place = get_daily_activity_location(i);
  if(place != NULL) {
    this->link[i].unenroll(this->myself);
  }
}

void Activities::unenroll_from_daily_activity_locations() {
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    unenroll_from_daily_activity_location(i);
  }
  clear_daily_activity_locations();
}

void Activities::store_daily_activity_locations() {
  this->stored_daily_activity_locations = new Place* [Activity_index::DAILY_ACTIVITY_LOCATIONS];
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    this->stored_daily_activity_locations[i] = get_daily_activity_location(i);
  }
}

void Activities::restore_daily_activity_locations() {
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    set_daily_activity_location(i, this->stored_daily_activity_locations[i]);
  }
  delete[] this->stored_daily_activity_locations;
}

int Activities::get_daily_activity_location_id(int p) {
  return get_daily_activity_location(p) == NULL ? -1 : get_daily_activity_location(p)->get_id();
}

const char* Activities::get_daily_activity_location_label(int p) {
  return (get_daily_activity_location(p) == NULL) ? "NULL" : get_daily_activity_location(p)->get_label();
}


void Activities::set_daily_activity_location(int i, Place* place) {
  if(place != NULL) {
    FRED_VERBOSE(1, "SET FAVORITE PLACE %d to place %d %s\n",i, place->get_id(), place->get_label());
  } else {
    FRED_VERBOSE(1, "SET FAVORITE PLACE %d to NULL\n",i);
  }
  // update link if necessary
  Place* old_place = get_daily_activity_location(i);
  FRED_VERBOSE(1, "old place %s\n", old_place? old_place->get_label():"NULL");
  if(place != old_place) {
    if(old_place != NULL) {
      // remove old link
      // printf("remove old link\n");
      this->link[i].unenroll(this->myself);
    }
    if(place != NULL) {
      this->link[i].enroll(this->myself, place);
    }
  }
  FRED_VERBOSE(1, "set daily activity location finished\n");
}

bool Activities::is_present(int sim_day, Place* place) {

  // not here if traveling abroad
  if(this->is_traveling_outside) {
    return false;
  }

  // update list of places to visit today if not already done
  if(sim_day > this->schedule_updated) {
    update_schedule(sim_day);
  }

  // see if this place is on the list
  for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
    if(get_daily_activity_location(i) == place && this->on_schedule[i]) {
      return true;
    }
  }
  return false;
}

void Activities::create_network_link_to(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->create_link_to(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::create_network_link_from(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->create_link_from(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::destroy_network_link_to(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->destroy_link_to(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::destroy_network_link_from(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->destroy_link_from(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::add_network_link_to(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->add_link_to(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::add_network_link_from(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->add_link_from(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::delete_network_link_to(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->delete_link_to(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::delete_network_link_from(Person* person, Network* network) {
  int size = this->networks.size();
  for (int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->delete_link_from(person);
      return;
    }
  }
  Utils::fred_abort("network not found");
}

void Activities::join_network(Network* network) {
  FRED_VERBOSE(1, "JOINING NETWORK: id = %d\n", this->myself->get_id());
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return;
    }
  }
  Person_Network_Link* network_link = new Person_Network_Link(this->myself, network);
  this->networks.push_back(network_link);
}

//************* Mina added
void Activities::unenroll_network(Network* network) {
  FRED_VERBOSE(1, "UNENROLL NETWORK: id = %d\n", this->myself->get_id());
  int size = this->networks.size();
  
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->remove_from_network();
      return;
    }
  }
  
  //  //mina debug
  //  if(this->myself->get_id() == Global::TEST_ID){
  //    cout << "Mina: join_network end. network size for person: " << this->networks.size() << endl;
  //    getchar();
  //  }
}
//***************************


bool Activities::is_enrolled_in_network(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return true;
    }
  }
  return false;
}

void Activities::print_network(FILE* fp, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->print(fp);
    }
  }
}

bool Activities::is_connected_to(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->is_connected_to(person);
    }
  }
  Utils::fred_abort("network not found");
  return false;
}


bool Activities::is_connected_from(Person* person, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->is_connected_from(person);
    }
  }
  Utils::fred_abort("network not found\n");
  return false;
}

int Activities::get_out_degree(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_out_degree();
    }
  }
  Utils::fred_abort("network not found\n");
  return 0;
}

int Activities::get_out_degree_sexual_acts(Network* network, Relationships* relationships) {    //mina added this method
  int size = this->networks.size();
  int sexual_acts_today = 0;
  int num_matched_partners = relationships->get_partner_list_size();

  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
     
      for (int j = 0; j < num_matched_partners; j++) {
        if(relationships->if_sexual_act_today(j)) sexual_acts_today++;
      }
      
      //      if(myself->get_sex() == 'F' && num_matched_partners > 0){
      //        cout << "Activities::get_out_degree_sexual_acts matched_partners " <<  num_matched_partners << "  num_matched partners " << num_matched_partners << "  sexual acts today " << sexual_acts_today << endl;
      //        getchar();  //for debug
      //      }

      return sexual_acts_today;
    }
  }
  Utils::fred_abort("network not found\n");
  return 0;
}

int Activities::get_in_degree(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_in_degree();
    }
  }
  Utils::fred_abort("network not found\n");
  return 0;
}

void Activities::clear_network(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      this->networks[i]->clear();
      return;
    }
  }
  Utils::fred_abort("network not found\n");
}

Person* Activities::get_end_of_link(int n, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_end_of_link(n);
    }
  }
  Utils::fred_abort("network not found\n");
  return NULL;
}

Person* Activities::get_beginning_of_link(int n, Network* network) {    //mina added
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_beginning_of_link(n);
    }
  }
  Utils::fred_abort("network not found\n");
  return NULL;
}


int Activities::get_index_of_sick_leave_dist(Person* per) {
  return -1;
}

/**
 * @return a pointer to this agent's Household
 */
Household* Activities::get_household() {
  return static_cast<Household*>(get_daily_activity_location(Activity_index::HOUSEHOLD_ACTIVITY));
}

Household* Activities::get_stored_household() {
  return static_cast<Household*>(this->stored_daily_activity_locations[Activity_index::HOUSEHOLD_ACTIVITY]);
}

/**
 * @return a pointer to this agent's Neighborhood
 */
Neighborhood* Activities::get_neighborhood() {
  return static_cast<Neighborhood*>( get_daily_activity_location(Activity_index::NEIGHBORHOOD_ACTIVITY));
}

/**
 * @return a pointer to this agent's School
 */
School* Activities::get_school() {
  return static_cast<School*>( get_daily_activity_location(Activity_index::SCHOOL_ACTIVITY));
}

/**
 * @return a pointer to this agent's Classroom
 */
Classroom* Activities::get_classroom() {
  return static_cast<Classroom*>( get_daily_activity_location(Activity_index::CLASSROOM_ACTIVITY));
}

/**
 * @return a pointer to this agent's Workplace
 */
Workplace* Activities::get_workplace() {
  return static_cast<Workplace*>( get_daily_activity_location(Activity_index::WORKPLACE_ACTIVITY));
}

/**
 * @return a pointer to this agent's Office
 */
Office* Activities::get_office() {
  return static_cast<Office*>( get_daily_activity_location(Activity_index::OFFICE_ACTIVITY));
}

/**
 * @return a pointer to this agent's Hospital
 */
Hospital* Activities::get_hospital() {
  return static_cast<Hospital*>( get_daily_activity_location(Activity_index::HOSPITAL_ACTIVITY));
}

void Activities::set_last_school(Place* school) {
  this->last_school = static_cast<School*>(school);
}

void Activities::clear_confinement_to_household(int condition_id) {
  is_confined_for_condition[condition_id] = false;
  is_confined_due_to_contact_tracing = false;
  for (int i = 0; i < Global::Conditions.get_number_of_conditions(); i++) {
    if (is_confined_for_condition[i]) {
      is_confined_due_to_contact_tracing = true;
      return;
    }
  }
}

