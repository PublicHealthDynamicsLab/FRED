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
// File: Activities.cc
//
#include "Activities.h"
#include "Behavior.h"
#include "Classroom.h"
#include "Global.h"
#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Geo.h"
#include "Global.h"
#include "Household.h"
#include "Manager.h"
#include "Mixing_Group.h"
#include "Neighborhood.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Network.h"
#include "Params.h"
#include "Person.h"
#include "Person_Network_Link.h"
#include "Place.h"
#include "Place_List.h"
#include "Random.h"
#include "School.h"
#include "Travel.h"
#include "Utils.h"
#include "Vaccine_Manager.h"
#include "Workplace.h"

class Activities_Tracking_Data;

const char* get_label_for_place(Place* place) {
  return place ? place->get_label() : "NULL";
}

bool Activities::is_initialized = false;
bool Activities::is_weekday = false;
int Activities::day_of_week = 0;

// run-time parameters
bool Activities::Enable_default_sick_behavior = false;
double Activities::Default_sick_day_prob = 0.0;
//double Activities::SLA_mean_sick_days_absent = 0.0;
//double Activities::SLU_mean_sick_days_absent = 0.0;
double Activities::SLA_absent_prob = 0.0;
double Activities::SLU_absent_prob = 0.0;
double Activities::Flu_days = 0.0;
double Activities::Hospitalization_visit_housemate_prob = 0.0;
int Activities::HAZEL_seek_hc_ramp_up_days = 0;
double Activities::HAZEL_seek_hc_ramp_up_mult = 1.0;

Age_Map* Activities::Hospitalization_prob = NULL;
Age_Map* Activities::Outpatient_healthcare_prob = NULL;

Activities_Tracking_Data Activities::Tracking_data;

// Childhood Presenteeism parameters
double Activities::Sim_based_prob_stay_home_not_needed = 0.0;
double Activities::Census_based_prob_stay_home_not_needed = 0.0;

// sick leave statistics
double Activities::Standard_sicktime_allocated_per_child = 0.0;

int Activities::Sick_leave_dist_method = 0;
std::vector<double> Activities::WP_size_sl_prob_vec;
std::vector<double> Activities::HH_income_qtile_sl_prob_vec;
double Activities::WP_small_mean_sl_days_available = 0.0;
double Activities::WP_large_mean_sl_days_available = 0.0;
int Activities::WP_size_cutoff_sl_exception = 0;
double Activities::HH_income_qtile_mean_sl_days_available = 0.0;

/// static method called in main (Fred.cc)
void Activities::initialize_static_variables() {

  if(Activities::is_initialized) {
    return;
  }

  FRED_STATUS(0, "Activities::initialize() entered\n", "");

  int temp_int = 0;
  Params::get_param_from_string("enable_default_sick_behavior", &temp_int);
  Activities::Enable_default_sick_behavior = (temp_int == 0 ? false : true);
  Params::get_param_from_string("sick_day_prob", &Activities::Default_sick_day_prob);
  Params::get_param_from_string("SLA_absent_prob", &Activities::SLA_absent_prob);
  Params::get_param_from_string("SLU_absent_prob", &Activities::SLU_absent_prob);
  Params::get_param_from_string("wp_small_mean_sl_days_available", &Activities::WP_small_mean_sl_days_available);
  Params::get_param_from_string("wp_large_mean_sl_days_available", &Activities::WP_large_mean_sl_days_available);
  Params::get_param_from_string("wp_size_cutoff_sl_exception", &Activities::WP_size_cutoff_sl_exception);

  Params::get_param_from_string("flu_days", &Activities::Flu_days);
  Params::get_param_from_string("prob_of_visiting_hospitalized_housemate", &Activities::Hospitalization_visit_housemate_prob);

  Params::get_param_from_string("sick_leave_dist_method", &Activities::Sick_leave_dist_method);

  if(!Activities::Enable_default_sick_behavior) {
    if(Activities::Sick_leave_dist_method == Activities::WP_SIZE_DIST) {
      Params::get_param_vector((char*)"wp_size_sl_prob_vec", Activities::WP_size_sl_prob_vec);
      assert(static_cast<int>(Activities::WP_size_sl_prob_vec.size()) == Workplace::get_workplace_size_group_count());
      for(int i = 0; i < Workplace::get_workplace_size_group_count(); ++i) {
        Activities::Tracking_data.employees_with_sick_leave.push_back(0);
        Activities::Tracking_data.employees_without_sick_leave.push_back(0);
        Activities::Tracking_data.employees_days_used.push_back(0);
        Activities::Tracking_data.employees_taking_day_off.push_back(0);
        Activities::Tracking_data.employees_sick_leave_days_used.push_back(0);
        Activities::Tracking_data.employees_taking_sick_leave_day_off.push_back(0);
        Activities::Tracking_data.employees_sick_days_present.push_back(0);
      }
    } else if(Activities::Sick_leave_dist_method == Activities::HH_INCOME_QTILE_DIST) {
      Params::get_param_vector((char*)"hh_income_qtile_sl_prob_vec", Activities::HH_income_qtile_sl_prob_vec);
      assert(static_cast<int>(Activities::HH_income_qtile_sl_prob_vec.size()) == 4);
      for(int i = 0; i < 4; ++i) {
        Activities::Tracking_data.employees_with_sick_leave.push_back(0);
        Activities::Tracking_data.employees_without_sick_leave.push_back(0);
        Activities::Tracking_data.employees_days_used.push_back(0);
        Activities::Tracking_data.employees_taking_day_off.push_back(0);
        Activities::Tracking_data.employees_sick_leave_days_used.push_back(0);
        Activities::Tracking_data.employees_taking_sick_leave_day_off.push_back(0);
        Activities::Tracking_data.employees_sick_days_present.push_back(0);
      }
    } else {
      Utils::fred_abort("Invalid sick_leave_dist_method: %d", Activities::Sick_leave_dist_method);
    }
  }

  if(Global::Enable_Hospitals) {
    Activities::Hospitalization_prob = new Age_Map("Hospitalization Probability");
    Activities::Hospitalization_prob->read_from_input("hospitalization_prob");
    Activities::Outpatient_healthcare_prob = new Age_Map("Outpatient Healthcare Probability");
    Activities::Outpatient_healthcare_prob->read_from_input("outpatient_healthcare_prob");
  }

  if(Global::Enable_HAZEL) {
    Params::get_param_from_string("HAZEL_seek_hc_ramp_up_days", &Activities::HAZEL_seek_hc_ramp_up_days);
    Params::get_param_from_string("HAZEL_seek_hc_ramp_up_mult", &Activities::HAZEL_seek_hc_ramp_up_mult);
  }

  if(Global::Report_Childhood_Presenteeism) {

    Params::get_param_from_string("standard_sicktime_allocated_per_child", &Activities::Standard_sicktime_allocated_per_child);

    int count_has_school_age = 0;
    int count_has_school_age_and_unemployed_adult = 0;

    //Households with school-age children and at least one unemployed adult
    int number_places = Global::Places.get_number_of_households();
    for(int p = 0; p < number_places; ++p) {
      Person* per = NULL;
      Household* h = Global::Places.get_household_ptr(p);
      if(h->get_children() == 0) {
	      continue;
      }
      if(h->has_school_aged_child()) {
	      count_has_school_age++;
      }
      if(h->has_school_aged_child_and_unemployed_adult()) {
	      count_has_school_age_and_unemployed_adult++;
      }
    }
    Activities::Sim_based_prob_stay_home_not_needed = static_cast<double>(count_has_school_age_and_unemployed_adult) / static_cast<double>(count_has_school_age);
  }
  Activities::is_initialized = true;
}

Activities::Activities() {
  this->myself = NULL;
  this->sick_leave_available = false;
  this->my_sick_days_absent = -1;
  this->my_sick_days_present = -1;
  this->my_unpaid_sick_days_absent = -1;
  this->my_sick_leave_decision_has_been_made = false;
  this->my_sick_leave_decision = false;
  this->home_neighborhood = NULL;
  this->profile = UNDEFINED_PROFILE;
  this->schedule_updated = -1;
  this->stored_daily_activity_locations = NULL;
  this->primary_healthcare_facility = NULL;
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->is_hospitalized = false;
  this->sim_day_hospitalization_ends = -1;
  this->is_isolated = false;
  this->grade = 0;
  this->return_from_travel_sim_day = -1;
  this->link = new Person_Place_Link [Activity_index::DAILY_ACTIVITY_LOCATIONS];
  this->networks.clear();
}

void Activities::setup(Person* self, Place* house, Place* school, Place* work) {

  FRED_VERBOSE(1, "ACTIVITIES_SETUP: person %d age %d household %s\n",
	       this->myself->get_id(), this->myself->get_age(), house->get_label());

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
  int index = get_household()->get_county_index();
  Global::Places.increment_population_of_county_with_index(index, this->myself);

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

  // sick leave variables
  this->my_sick_days_absent = 0;
  this->my_sick_days_present = 0;
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
  this->initialize_sick_leave();
}

void Activities::initialize_sick_leave() {
  FRED_VERBOSE(1, "Activities::initialize_sick_leave entered\n");
  this->my_sick_days_absent = 0;
  this->my_sick_days_present = 0;
  this->my_sick_leave_decision_has_been_made = false;
  this->my_sick_leave_decision = false;
  this->sick_days_remaining = 0.0;
  this->sick_leave_available = false;

  if(!Activities::Enable_default_sick_behavior) {
    int index = Activities::get_index_of_sick_leave_dist(this->myself);
    assert(index > 0);
    if(Activities::Sick_leave_dist_method == Activities::WP_SIZE_DIST) {
      this->sick_leave_available = (Random::draw_random() < Activities::WP_size_sl_prob_vec[index]);
      if(this->sick_leave_available) {
        Activities::Tracking_data.employees_with_sick_leave[index]++;
        // compute sick days available
        // compute sick days available
        int workplace_size = 0;
        if(get_workplace() != NULL) {
          workplace_size = get_workplace()->get_size();
        } else {
          if(is_teacher()) {
            if(get_school() != NULL) {
              workplace_size = get_school()->get_staff_size();
            }
          }
        }
        if(workplace_size <= Activities::WP_size_cutoff_sl_exception) {
          this->sick_days_remaining = Activities::WP_small_mean_sl_days_available + Activities::Flu_days;
        } else {
          this->sick_days_remaining = Activities::WP_large_mean_sl_days_available + Activities::Flu_days;
        }
      } else {
        Activities::Tracking_data.employees_without_sick_leave[index]++;
      }
    } else if(Activities::Sick_leave_dist_method == Activities::HH_INCOME_QTILE_DIST) {
      this->sick_leave_available = (Random::draw_random() < Activities::HH_income_qtile_sl_prob_vec[index]);
      // compute sick days available
      this->sick_days_remaining = Activities::HH_income_qtile_mean_sl_days_available + Activities::Flu_days;
    }

    FRED_VERBOSE(1, "Activities::initialize_sick_leave size_leave_avaliable = %d\n",
		             (this->sick_leave_available ? 1 : 0));
  }
  FRED_VERBOSE(1, "Activities::initialize_sick_leave sick_days_remaining = %d\n", this->sick_days_remaining);
}

void Activities::before_run() {
  if(Global::Report_Presenteeism) {
    if(!Activities::Enable_default_sick_behavior) {
      if(Activities::Sick_leave_dist_method == Activities::WP_SIZE_DIST) {
        for(int i = 0; i < Workplace::get_workplace_size_group_count(); ++i) {
          if(i == 0) {
            FRED_STATUS(0, "Employees in Workplace[0 - %d] with sick leave: %d\n",
                        Workplace::get_workplace_size_max_by_group_id(i),
                        Activities::Tracking_data.employees_with_sick_leave[i]);
            FRED_STATUS(0, "Employees in Workplace[0 - %d] without sick leave: %d\n",
                        Workplace::get_workplace_size_max_by_group_id(i),
                        Activities::Tracking_data.employees_without_sick_leave[i]);
          } else {
            FRED_STATUS(0, "Employees in Workplace[%d - %d] with sick leave: %d\n",
                        Workplace::get_workplace_size_max_by_group_id(i - 1) + 1,
                        Workplace::get_workplace_size_max_by_group_id(i),
                        Activities::Tracking_data.employees_with_sick_leave[i]);
            FRED_STATUS(0, "Employees in Workplace[%d - %d] without sick leave: %d\n",
                        Workplace::get_workplace_size_max_by_group_id(i - 1) + 1,
                        Workplace::get_workplace_size_max_by_group_id(i),
                        Activities::Tracking_data.employees_without_sick_leave[i]);
          }
        }
      }
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    Global::Places.setup_household_income_quartile_sick_days();
  }
}

void Activities::assign_initial_profile() {
  int age = this->myself->get_age();
  if(age == 0) {
    this->profile = PRESCHOOL_PROFILE;
  } else if(get_school() != NULL) {
    this->profile = STUDENT_PROFILE;
  } else if(age < Global::SCHOOL_AGE) {
    this->profile = PRESCHOOL_PROFILE;    // child at home
  } else if(get_workplace() != NULL) {
    this->profile = WORKER_PROFILE;// worker
  } else if(Global::RETIREMENT_AGE <= age) {
    this->profile = RETIRED_PROFILE;      // retired
  } else {
    this->profile = UNEMPLOYED_PROFILE;
  }

  // weekend worker
  if((this->profile == WORKER_PROFILE && Random::draw_random() < 0.2)) {  // 20% weekend worker
    this->profile = WEEKEND_WORKER_PROFILE;
  }

  // profiles for group quarters residents
  if(get_household()->is_college()) {
    this->profile = COLLEGE_STUDENT_PROFILE;
  }
  if(get_household()->is_military_base()) {
    this->profile = MILITARY_PROFILE;
  }
  if(get_household()->is_prison()) {
    this->profile = PRISONER_PROFILE;
    FRED_VERBOSE(2, "INITIAL PROFILE AS PRISONER ID %d AGE %d SEX %c HOUSEHOLD %s\n",
		 this->myself->get_id(), age, this->myself->get_sex(), get_household()->get_label());
  }
  if(get_household()->is_nursing_home()) {
    this->profile = NURSING_HOME_RESIDENT_PROFILE;
  }
}

void Activities::update(int sim_day) {

  FRED_STATUS(1, "Activities update entered\n");

  // decide if this is a weekday:
  Activities::is_weekday = Date::is_weekday();

  if(Global::Enable_HAZEL) {
    Global::Daily_Tracker->set_index_key_pair(sim_day, SEEK_HC, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, PRIMARY_HC_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, HC_ACCEP_INS_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, HC_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, MEDICARE_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, ASTHMA_HC_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, DIABETES_HC_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, HTN_HC_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, MEDICAID_UNAV, 0);
    Global::Daily_Tracker->set_index_key_pair(sim_day, MEDICARE_UNAV, 0);
    Global::Daily_Tracker->increment_index_key_pair(sim_day, PRIVATE_UNAV, 1);
    Global::Daily_Tracker->increment_index_key_pair(sim_day, UNINSURED_UNAV, 1);
  }

  // print out absenteeism/presenteeism counts
  FRED_CONDITIONAL_VERBOSE(1, sim_day > 0,
			   "DAY %d ABSENTEEISM: work absent %d present %d %0.2f  school absent %d present %d %0.2f\n",
			   sim_day - 1, Activities::Tracking_data.daily_sick_days_absent, Activities::Tracking_data.daily_sick_days_present,
			   static_cast<double>(Activities::Tracking_data.daily_sick_days_absent) / static_cast<double>(1 + Activities::Tracking_data.daily_sick_days_absent +
														 Activities::Tracking_data.daily_sick_days_present),
			   Activities::Tracking_data.daily_school_sick_days_absent, Activities::Tracking_data.daily_school_sick_days_present,
			   static_cast<double>(Activities::Tracking_data.daily_school_sick_days_absent) /
			   static_cast<double>(1 + Activities::Tracking_data.daily_school_sick_days_absent + Activities::Tracking_data.daily_school_sick_days_present));

  // keep track of global activity counts
  Activities::Tracking_data.daily_sick_days_present = 0;
  Activities::Tracking_data.daily_sick_days_absent = 0;
  Activities::Tracking_data.daily_school_sick_days_present = 0;
  Activities::Tracking_data.daily_school_sick_days_absent = 0;

  // print school change activities
  if(Activities::Tracking_data.entered_school + Activities::Tracking_data.left_school > 0) {
    printf("DAY %d ENTERED_SCHOOL %d LEFT_SCHOOL %d\n",
	         sim_day, Activities::Tracking_data.entered_school, Activities::Tracking_data.left_school);
    Activities::Tracking_data.entered_school = 0;
    Activities::Tracking_data.left_school = 0;
  }

  FRED_STATUS(1, "Activities update completed\n");
}

void Activities::update_activities_of_infectious_person(int sim_day) {

  FRED_VERBOSE(1,"update_activities for person %d day %d\n", this->myself->get_id(), sim_day);

  // skip all scheduled activities if traveling abroad
  if(this->is_traveling_outside) {
    return;
  }

  if(Global::Enable_Isolation) {
    if(this->is_isolated) {
      // once isolated, remain isolated
      update_schedule(sim_day);
      return;
    } else {
      // enter isolation if symptomatic, with a given probability
      if(this->myself->is_symptomatic()) {
	      // are we passed the isolation delay period?
	      if(Global::Isolation_Delay <= this->myself->get_days_symptomatic()) {
	        //decide whether to enter isolation
	        if(Random::draw_random() < Global::Isolation_Rate) {
	          this->is_isolated = true;
	          update_schedule(sim_day);
	          return;
	        }
	      }
      }
    }
  }

  if(sim_day > this->schedule_updated) {
    // get list of places to visit today
    update_schedule(sim_day);

    // decide which neighborhood to visit today
    if(this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY]) {
      Place* destination_neighborhood = Global::Neighborhoods->select_destination_neighborhood(this->home_neighborhood);
      set_neighborhood(destination_neighborhood);
    }

    // if symptomatic, decide whether or not to stay home
    if(this->myself->is_symptomatic() && !this->myself->is_hospitalized()) {
      decide_whether_to_stay_home(sim_day);
      //For Symptomatics - background will be in update_schedule()
      if(Global::Enable_Hospitals) {
	      decide_whether_to_seek_healthcare(sim_day);
      }
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

  // if isolated, visit nowhere today
  if(this->is_isolated) {
    return;
  }

  if(Global::Enable_Hospitals && this->is_hospitalized && !(this->sim_day_hospitalization_ends == sim_day)) {
    // only visit the hospital
    this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
    if(Global::Enable_HAZEL) {
      Global::Daily_Tracker->increment_index_key_pair(sim_day, SEEK_HC, 1);
      Household* hh = static_cast<Household*>(this->myself->get_permanent_household());
      assert(hh != NULL);
      hh->set_count_seeking_hc(hh->get_count_seeking_hc() + 1);
    }
  } else {
    //If the hospital stay should end today, go back to normal
    if(Global::Enable_Hospitals && this->is_hospitalized && (this->sim_day_hospitalization_ends == sim_day)) {
      end_hospitalization();
    }

    // always visit the household
    this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;

    // decide if my household is sheltering
    if(Global::Enable_Household_Shelter || Global::Enable_HAZEL) {
      Household* h = static_cast<Household*>(this->myself->get_household());
      if(h->is_sheltering_today(sim_day)) {
        FRED_STATUS(1, "update_schedule on day %d\n%s\n", sim_day,
                    schedule_to_string(sim_day).c_str());
        return;
      }
    }

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

    //Decide whether to visit healthcare if ASYMPTOMATIC (Background)
    if(Global::Enable_Hospitals && !this->myself->is_symptomatic() && !this->myself->is_hospitalized()) {
      decide_whether_to_seek_healthcare(sim_day);
    }

    //Decide whether to visit hospitalized housemates
    if(Global::Enable_Hospitals && !this->myself->is_hospitalized() && static_cast<Household*>(this->myself->get_household())->has_hospitalized_member()) {

      Household* hh = static_cast<Household*>(this->myself->get_household());

      if(hh == NULL) {
        if(Global::Enable_Hospitals && this->myself->is_hospitalized() && this->myself->get_permanent_household() != NULL) {
          hh = static_cast<Household*>(this->myself->get_permanent_household());
        }
      }

      if(hh != NULL) {
        if(this->profile != PRISONER_PROFILE && this->profile != NURSING_HOME_RESIDENT_PROFILE && Random::draw_random() < Activities::Hospitalization_visit_housemate_prob) {
          set_ad_hoc(hh->get_household_visitation_hospital());
          this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = true;
        }
      }
    }

    // skip work at background absenteeism rate
    if(Global::Work_absenteeism > 0.0 && this->on_schedule[Activity_index::WORKPLACE_ACTIVITY]) {
      if(Random::draw_random() < Global::Work_absenteeism) {
        this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
        this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
      }
    }

    // skip school at background school absenteeism rate
    if(Global::School_absenteeism > 0.0 && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
      if(Random::draw_random() < Global::School_absenteeism) {
        this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
        this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
      }
    }

    if(Global::Report_Childhood_Presenteeism) {
      if(this->myself->is_adult() && this->on_schedule[Activity_index::WORKPLACE_ACTIVITY]) {
        Household* my_hh = static_cast<Household*>(this->myself->get_household());
        //my_hh->have_working_adult_use_sickleave_for_child()
        if(my_hh->has_sympt_child() &&
           my_hh->has_school_aged_child() &&
           !my_hh->has_school_aged_child_and_unemployed_adult() &&
           !my_hh->has_working_adult_using_sick_leave()) {
          for(int i = 0; i < static_cast<int>(my_hh->get_size()); ++i) {
            Person* child_check = my_hh->get_enrollee(i);
            if(child_check->is_child() &&
               child_check->is_student() &&
               child_check->is_symptomatic() &&
               child_check->get_activities()->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
              // Do I have enough sick time?
              if(this->sick_days_remaining >= Activities::Standard_sicktime_allocated_per_child) {
                if(my_hh->have_working_adult_use_sickleave_for_child(this->myself, child_check)) {
                  //Stay Home from work
                  this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
                  this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
                  my_hh->set_working_adult_using_sick_leave(true);
                  this->sick_days_remaining -= Activities::Standard_sicktime_allocated_per_child;
                }
              }
            }
          }
        }
      }
    }
  }
  FRED_STATUS(1, "update_schedule on day %d\n%s\n", sim_day, schedule_to_string(sim_day).c_str());
}

void Activities::decide_whether_to_stay_home(int sim_day) {

  assert(this->myself->is_symptomatic());
  bool stay_home = false;
  bool it_is_a_workday = (this->on_schedule[Activity_index::WORKPLACE_ACTIVITY]
        || (is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]));

  if(this->myself->is_adult()) {
    if(Activities::Enable_default_sick_behavior) {
      stay_home = default_sick_leave_behavior();
    } else {
      if(it_is_a_workday) {
        int index = Activities::get_index_of_sick_leave_dist(this->myself);
        assert(index > 0);
        if(this->my_sick_leave_decision_has_been_made) {
          if(this->is_sick_leave_available() && this->sick_days_remaining > 0.0) {
            if(this->sick_days_remaining < 1.0) { //I have sick leave, but my days are about to run out
              this->sick_days_remaining = (this->sick_days_remaining < 0.0 ? 0.0 : this->sick_days_remaining);
              if(Random::draw_random() < this->sick_days_remaining) {
                stay_home = this->my_sick_leave_decision;
                //Want to make sure that tomorrow I roll against the SLU_absent_prob
                this->my_sick_leave_decision_has_been_made = false;
                if(stay_home) {
                  this->my_sick_days_absent++;
                  Activities::Tracking_data.employees_days_used[index]++;
                  Activities::Tracking_data.employees_sick_leave_days_used[index]++;
                } else {
                  this->my_sick_days_present++;
                  Activities::Tracking_data.employees_sick_days_present[index]++;
                }
              } else {
                stay_home = (Random::draw_random() < Activities::SLU_absent_prob);
                if(stay_home) {
                  this->my_unpaid_sick_days_absent++;
                  Activities::Tracking_data.employees_days_used[index]++;
                } else {
                  this->my_sick_days_present++;
                  Activities::Tracking_data.daily_sick_days_present++;
                  Activities::Tracking_data.total_employees_sick_days_present++;
                  Activities::Tracking_data.employees_sick_days_present[index]++;
                }
              }
              this->my_sick_leave_decision = stay_home;
              this->sick_days_remaining = 0.0;
            } else {
              stay_home = this->my_sick_leave_decision;
              if(stay_home) {
                this->my_sick_days_absent++;
                Activities::Tracking_data.daily_sick_days_absent++;
                Activities::Tracking_data.total_employees_days_used++;
                Activities::Tracking_data.total_employees_sick_leave_days_used++;
                Activities::Tracking_data.employees_days_used[index]++;
                Activities::Tracking_data.employees_sick_leave_days_used[index]++;
              } else {
                this->my_sick_days_present++;
                Activities::Tracking_data.daily_sick_days_present++;
                Activities::Tracking_data.total_employees_sick_days_present++;
                Activities::Tracking_data.employees_sick_days_present[index]++;
              }
              this->sick_days_remaining -= 1.0;
            }
          } else { //Sick leave decision has been made, but I don't get paid sick days
            stay_home = this->my_sick_leave_decision;
            if(stay_home) {
              this->my_unpaid_sick_days_absent++;
              Activities::Tracking_data.daily_sick_days_absent++;
              Activities::Tracking_data.total_employees_days_used++;
              Activities::Tracking_data.employees_days_used[index]++;
            } else {
              this->my_sick_days_present++;
              Activities::Tracking_data.daily_sick_days_present++;
              Activities::Tracking_data.total_employees_sick_days_present++;
              Activities::Tracking_data.employees_sick_days_present[index]++;
            }
          }
        } else { //Sick Leave decision hasn't been made
          if(this->is_sick_leave_available() && this->sick_days_remaining > 0.0) {
            if(this->sick_days_remaining < 1.0) { //I have sick leave but I am running out of days, so need to decide what to do
              this->sick_days_remaining = (this->sick_days_remaining < 0.0 ? 0.0 : this->sick_days_remaining);
              if(Random::draw_random() < this->sick_days_remaining) {
                stay_home = (Random::draw_random() < Activities::SLA_absent_prob);
                if(stay_home) {
                  //Want to make sure that tomorrow I roll against the SLU_absent_prob
                  this->my_sick_leave_decision_has_been_made = false;
                  this->my_sick_days_absent++;
                  Activities::Tracking_data.daily_sick_days_absent++;
                  Activities::Tracking_data.total_employees_days_used++;
                  Activities::Tracking_data.total_employees_taking_day_off++;
                  Activities::Tracking_data.total_employees_sick_leave_days_used++;
                  Activities::Tracking_data.total_employees_taking_sick_leave++;
                  Activities::Tracking_data.employees_days_used[index]++;
                  Activities::Tracking_data.employees_taking_day_off[index]++;
                  Activities::Tracking_data.employees_sick_leave_days_used[index]++;
                  Activities::Tracking_data.employees_taking_sick_leave_day_off[index]++;
                } else {
                  this->my_sick_leave_decision_has_been_made = true;
                  this->my_sick_days_present++;
                  Activities::Tracking_data.daily_sick_days_present++;
                  Activities::Tracking_data.total_employees_sick_days_present++;
                  Activities::Tracking_data.employees_sick_days_present[index]++;
                }
              } else {
                stay_home = (Random::draw_random() < Activities::SLU_absent_prob);
                this->my_sick_leave_decision_has_been_made = true;
                if(stay_home) {
                  this->my_unpaid_sick_days_absent++;
                  Activities::Tracking_data.total_employees_days_used++;
                  Activities::Tracking_data.total_employees_taking_day_off++;
                  Activities::Tracking_data.employees_days_used[index]++;
                  Activities::Tracking_data.employees_taking_day_off[index]++;
                } else {
                  this->my_sick_days_present++;
                  Activities::Tracking_data.daily_sick_days_present++;
                  Activities::Tracking_data.total_employees_sick_days_present++;
                  Activities::Tracking_data.employees_sick_days_present[index]++;
                }
              }
              this->my_sick_leave_decision = stay_home;
              this->sick_days_remaining = 0.0;
            } else { //I have sick leave and I am not running out of days, so stay home or not versus SLA probability
              stay_home = (Random::draw_random() < Activities::SLA_absent_prob);
              this->my_sick_leave_decision = stay_home;
              this->my_sick_leave_decision_has_been_made = true;
              if(stay_home) {
                this->my_sick_days_absent++;
                Activities::Tracking_data.daily_sick_days_absent++;
                Activities::Tracking_data.total_employees_days_used++;
                Activities::Tracking_data.total_employees_taking_day_off++;
                Activities::Tracking_data.total_employees_sick_leave_days_used++;
                Activities::Tracking_data.total_employees_taking_sick_leave++;
                Activities::Tracking_data.employees_days_used[index]++;
                Activities::Tracking_data.employees_taking_day_off[index]++;
                Activities::Tracking_data.employees_sick_leave_days_used[index]++;
                Activities::Tracking_data.employees_taking_sick_leave_day_off[index]++;
              } else {
                this->my_sick_days_present++;
                Activities::Tracking_data.daily_sick_days_present++;
                Activities::Tracking_data.total_employees_sick_days_present++;
                Activities::Tracking_data.employees_sick_days_present[index]++;
              }
              this->sick_days_remaining -= 1.0;
            }
          } else { //I have no sick leave available, so roll against SLU probability only
            stay_home = (Random::draw_random() < Activities::SLU_absent_prob);
            this->my_sick_leave_decision = stay_home;
            this->my_sick_leave_decision_has_been_made = true;
            if(stay_home) {
              this->my_unpaid_sick_days_absent++;
              Activities::Tracking_data.total_employees_days_used++;
              Activities::Tracking_data.total_employees_taking_day_off++;
              Activities::Tracking_data.employees_days_used[index]++;
              Activities::Tracking_data.employees_taking_day_off[index]++;
            } else {
              this->my_sick_days_present++;
              Activities::Tracking_data.daily_sick_days_present++;
              Activities::Tracking_data.total_employees_sick_days_present++;
              Activities::Tracking_data.employees_sick_days_present[index]++;
            }
          }
        }
      } else {
        // it is a not work day
        stay_home = (Random::draw_random() < Activities::Default_sick_day_prob);
      }
    }
  } else {
    // sick child
    if(Global::Report_Childhood_Presenteeism) {
      bool it_is_a_schoolday = this->myself->is_student() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY];
      if(it_is_a_schoolday) {
        Household* my_hh = static_cast<Household*>(this->myself->get_household());
        assert(my_hh != NULL);
        if(this->my_sick_leave_decision_has_been_made) {
          //Child has already made decision to stay home
          stay_home = this->my_sick_leave_decision;
        } else if(my_hh->has_working_adult_using_sick_leave()) {
          //An adult has already decided to stay home
          stay_home = true;
        } else if(my_hh->has_school_aged_child_and_unemployed_adult()) {
          //The agent will stay home because someone is there to watch him/her
          double prob_diff = Activities::Sim_based_prob_stay_home_not_needed - Activities::Census_based_prob_stay_home_not_needed;
          if(prob_diff > 0) {
            //There is a prob_diff chance that the agent will NOT stay home
            stay_home = (Random::draw_random() < (1 - prob_diff));
          } else {
            //The agent will stay home because someone is there to watch him/her
            stay_home = true;
          }
        } else {
          //No one would be home so we need to force an adult to stay home, if s/he has sick time
          //First find an adult in the house
          if(my_hh->get_adults() > 0) {
            std::vector<Person*> inhab_vec = my_hh->get_inhabitants();
            for(std::vector<Person*>::iterator itr = inhab_vec.begin(); itr != inhab_vec.end(); ++itr) {
              if((*itr)->is_child()) {
                continue;
              }

              //Person is an adult, but is also a student
              if((*itr)->is_adult() && (*itr)->is_student()) {
                continue;
              }

              //Person is an adult, but isn't at home
              if(my_hh->have_working_adult_use_sickleave_for_child((*itr), this->myself)) {
                stay_home = true;
                (*itr)->get_activities()->sick_days_remaining -= Activities::Standard_sicktime_allocated_per_child;
                my_hh->set_working_adult_using_sick_leave(true);
                break;
              }
            }
          }
        }
        if(!this->my_sick_leave_decision_has_been_made) {
          // Kids will stick to that decision even after the parent goes back to work
          // i.e. some kind of external daycare is setup
          this->my_sick_leave_decision = stay_home;
          this->my_sick_leave_decision_has_been_made = true;
        }
      } else {
        //Preschool or no school today, so we use default sick behavior, for now
        stay_home = default_sick_leave_behavior();
      }
    } else {
      //use default sick behavior, for now.
      stay_home = default_sick_leave_behavior();
    }
  }

  //Update the counters for how many sick days used

  // record work absent/present decision if it is a workday
  if(it_is_a_workday) {
    Activities::Tracking_data.total_employees_days_used++;
    if(stay_home) {
      Activities::Tracking_data.daily_sick_days_absent++;
      this->my_sick_days_absent++;
    } else {
      Activities::Tracking_data.daily_sick_days_present++;
      this->my_sick_days_present++;
    }
  }

  // record school absent/present decision if it is a school day
  if(!is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
    if(stay_home) {
      Activities::Tracking_data.daily_school_sick_days_absent++;
      this->my_sick_days_absent++;
    } else {
      Activities::Tracking_data.daily_school_sick_days_present++;
      this->my_sick_days_present++;
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

void Activities::decide_whether_to_seek_healthcare(int sim_day) {

  if(Global::Enable_Hospitals) {
    bool is_a_workday = (this->on_schedule[Activity_index::WORKPLACE_ACTIVITY]
			 || (is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]));

    if(!this->is_hospitalized) {

      double rand = Random::draw_random();
      double hospitalization_prob = Activities::Hospitalization_prob->find_value(this->myself->get_real_age()); //Background probability
      double seek_healthcare_prob = Activities::Outpatient_healthcare_prob->find_value(this->myself->get_real_age()); //Background probability

      //First check to see if agent will seek health care for any active symptomatic infection
      if(this->myself->is_symptomatic()) {
        //Get specific symptomatic conditions for multiplier
        for(int condition_id = 0; condition_id < Global::Conditions.get_number_of_conditions(); ++condition_id) {
          if(this->myself->get_health()->is_infected(condition_id)) {
            Condition* condition = Global::Conditions.get_condition(condition_id);
            if(this->myself->get_health()->get_symptoms(condition_id, sim_day) > condition->get_min_symptoms_for_seek_healthcare()) {
              hospitalization_prob += condition->get_hospitalization_prob(this->myself);
              seek_healthcare_prob += condition->get_outpatient_healthcare_prob(this->myself);
            }
          }
        }
      }

      // If the agent has chronic conditions, multiply the probability by the appropriate modifiers
      if(Global::Enable_Chronic_Condition) {
        if(this->myself->has_chronic_condition()) {
          double mult = 1.0;
          if(this->myself->is_asthmatic()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::ASTHMA);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->has_COPD()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::COPD);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->has_chronic_renal_condition()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::CHRONIC_RENAL_CONDITION);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->is_diabetic()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::DIABETES);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->has_heart_condition()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::HEART_CONDITION);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->has_hypertension()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::HYPERTENSION);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->has_hypercholestrolemia()) {
            mult = Health::get_chronic_condition_hospitalization_prob_mult(this->myself->get_age(), Chronic_condition_index::HYPERCHOLESTROLEMIA);
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
          if(this->myself->get_demographics()->is_pregnant()) {
            mult = Health::get_pregnancy_hospitalization_prob_mult(this->myself->get_age());
            hospitalization_prob *= mult;
            seek_healthcare_prob *= mult;
          }
        }
      }

      if(Global::Enable_HAZEL) {
        double mult = 1.0;
        //If we are within a week after the disaster
        //Ramp up visits immediately after a disaster
        if(sim_day > Place_List::get_HAZEL_disaster_end_sim_day() && sim_day <= (Place_List::get_HAZEL_disaster_end_sim_day() + 7)) {
          int days_since_storm_end = sim_day - Place_List::get_HAZEL_disaster_end_sim_day();
          mult = 1.0 + ((1.0 / static_cast<double>(days_since_storm_end)) * 0.25);
          hospitalization_prob *= mult;
          seek_healthcare_prob *= mult;
        }

        //Multiplier by insurance type
        mult = 1.0;
        switch(this->myself->get_health()->get_insurance_type()) {
         case Insurance_assignment_index::PRIVATE:
            mult = 1.0;
            break;
          case Insurance_assignment_index::MEDICARE:
            mult = 1.037;
            break;
          case Insurance_assignment_index::MEDICAID:
            mult = .909;
            break;
          case Insurance_assignment_index::HIGHMARK:
            mult = 1.0;
            break;
          case Insurance_assignment_index::UPMC:
            mult = 1.0;
            break;
          case Insurance_assignment_index::UNINSURED:
            {
              double age = this->myself->get_real_age();
              if(age < 5.0) { //These values are hard coded for HAZEL
                mult = 1.0;
              } else if(age < 18.0) {
                mult = 0.59;
              } else if(age < 25.0) {
                mult = 0.33;
              } else if(age < 45.0) {
                mult = 0.43;
              } else if(age < 65.0) {
                mult = 0.5;
              } else {
                mult = 0.56;
              }
            }
            break;
          case Insurance_assignment_index::UNSET:
            mult = 1.0;
            break;
        }
        hospitalization_prob *= mult;
        seek_healthcare_prob *= mult;
      }

      //First check to see if agent will visit a Hospital for an overnight stay, then check for an outpatient visit
      if(rand < hospitalization_prob) {
        double draw = Random::draw_normal(3.0, 0.5);
        int length = (draw > 0.0 ? static_cast<int>(draw) + 1 : 1);
        start_hospitalization(sim_day, length);
      } else if(rand < seek_healthcare_prob) {
        Household* hh = static_cast<Household*>(this->myself->get_household());
        assert(hh != NULL);

        if(Global::Enable_HAZEL) {
          Global::Daily_Tracker->increment_index_key_pair(sim_day, SEEK_HC, 1);
          hh->set_count_seeking_hc(hh->get_count_seeking_hc() + 1);
          if(!hh->is_seeking_healthcare()) {
            hh->set_seeking_healthcare(true);
          }

          Hospital* hosp = this->myself->get_activities()->get_primary_healthcare_facility();
          assert(hosp != NULL);

          if(!hosp->should_be_open(sim_day) || (hosp->get_current_daily_patient_count() >= hosp->get_daily_patient_capacity(sim_day))) {
            //Update all of the statistics to reflect that primary care is not available
            hh->set_is_primary_healthcare_available(false);
            if(this->myself->is_asthmatic()) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, ASTHMA_HC_UNAV, 1);
            }
            if(this->myself->is_diabetic()) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, DIABETES_HC_UNAV, 1);
            }
            if(this->myself->has_hypertension()) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, HTN_HC_UNAV, 1);
            }
            if(this->myself->get_health()->get_insurance_type() == Insurance_assignment_index::MEDICAID) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, MEDICAID_UNAV, 1);
            } else if(this->myself->get_health()->get_insurance_type() == Insurance_assignment_index::MEDICARE) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, MEDICARE_UNAV, 1);
            } else if(this->myself->get_health()->get_insurance_type() == Insurance_assignment_index::PRIVATE) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, PRIVATE_UNAV, 1);
            } else if(this->myself->get_health()->get_insurance_type() == Insurance_assignment_index::UNINSURED) {
              Global::Daily_Tracker->increment_index_key_pair(sim_day, UNINSURED_UNAV, 1);
            }

            Global::Daily_Tracker->increment_index_key_pair(sim_day, PRIMARY_HC_UNAV, 1);
            hh->set_count_primary_hc_unav(hh->get_count_primary_hc_unav() + 1);

            //Now, try to Find an open health care provider that accepts agent's insurance
            hosp = Global::Places.get_random_open_healthcare_facility_matching_criteria(sim_day, this->myself, true, false);
            if(hosp == NULL) {
              hh->set_other_healthcare_location_that_accepts_insurance_available(false);
              hh->set_count_hc_accept_ins_unav(hh->get_count_hc_accept_ins_unav() + 1);
              Global::Daily_Tracker->increment_index_key_pair(sim_day, HC_ACCEP_INS_UNAV, 1);

              hosp = Global::Places.get_random_open_healthcare_facility_matching_criteria(sim_day, this->myself, false, false);
              if(hosp == NULL) {
                hh->set_is_healthcare_available(false);
                Global::Daily_Tracker->increment_index_key_pair(sim_day, HC_UNAV, 1);
              }
            }

            if(hosp != NULL) {
              assign_hospital(hosp);
              if(hosp->get_subtype() == Place::SUBTYPE_NONE) {
                //then it is an emergency room visit
                Global::Daily_Tracker->increment_index_key_pair(sim_day, ER_VISIT, 1);
              }

              this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;
              this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
              this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
              this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
              this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
              this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = true;
              this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
              this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = false;

              hosp->increment_current_daily_patient_count();

              // record work absent/present decision if it is a work day
              if(is_a_workday) {
                Activities::Tracking_data.daily_sick_days_absent++;
                this->my_sick_days_absent++;
              }

              // record school absent/present decision if it is a school day
              if(!is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
                Activities::Tracking_data.daily_school_sick_days_absent++;
                this->my_sick_days_absent++;
              }
            }
          } else {
            assign_hospital(hosp);
            if(hosp->get_subtype() == Place::SUBTYPE_NONE) {
              //then it is an emergency room visit
              Global::Daily_Tracker->increment_index_key_pair(sim_day, ER_VISIT, 1);
            }

            this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;
            this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
            this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
            this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
            this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
            this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = true;
            this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
            this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = false;

            hosp->increment_current_daily_patient_count();

            // record work absent/present decision if it is a work day
            if(is_a_workday) {
              Activities::Tracking_data.daily_sick_days_absent++;
              this->my_sick_days_absent++;
            }

            // record school absent/present decision if it is a school day
            if(!is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
              Activities::Tracking_data.daily_school_sick_days_absent++;
              this->my_sick_days_absent++;
            }
          }
        } else { //Not HAZEL so don't need to track deficits
          Hospital* hosp = static_cast<Hospital*>(this->myself->get_hospital());
          if(hosp == NULL) {
            hosp = hh->get_household_visitation_hospital();
          }
          assert(hosp != NULL);
          assign_hospital(hosp);
          this->on_schedule[Activity_index::HOUSEHOLD_ACTIVITY] = true;
          this->on_schedule[Activity_index::WORKPLACE_ACTIVITY] = false;
          this->on_schedule[Activity_index::OFFICE_ACTIVITY] = false;
          this->on_schedule[Activity_index::SCHOOL_ACTIVITY] = false;
          this->on_schedule[Activity_index::CLASSROOM_ACTIVITY] = false;
          this->on_schedule[Activity_index::NEIGHBORHOOD_ACTIVITY] = true;
          this->on_schedule[Activity_index::HOSPITAL_ACTIVITY] = true;
          this->on_schedule[Activity_index::AD_HOC_ACTIVITY] = false;

          // record work absent/present decision if it is a workday
          if(is_a_workday) {
            Activities::Tracking_data.daily_sick_days_absent++;
            this->my_sick_days_absent++;
          }

          // record school absent/present decision if it is a school day
          if(!is_teacher() && this->on_schedule[Activity_index::SCHOOL_ACTIVITY]) {
            Activities::Tracking_data.daily_school_sick_days_absent++;
            this->my_sick_days_absent++;
          }
        }
      }
    }
  }
}

void Activities::start_hospitalization(int sim_day, int length_of_stay) {
  assert(length_of_stay > 0);
  if(Global::Enable_Hospitals) {
    assert(!this->myself->is_hospitalized());
    //If agent is traveling, return home first
    if(this->is_traveling || this->is_traveling_outside) {
      stop_traveling();
    }

    //First see if this agent has a preferred hospital
    Hospital* hosp = static_cast<Hospital*>(get_daily_activity_location(Activity_index::HOSPITAL_ACTIVITY));
    Household* hh = static_cast<Household*>(this->myself->get_household());
    assert(hh != NULL);

    //If not, then use the household's hospital
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

    if(Global::Enable_HAZEL) {
      Global::Daily_Tracker->increment_index_key_pair(sim_day, SEEK_HC, 1);
      hh->set_count_seeking_hc(hh->get_count_seeking_hc() + 1);
      if(!hosp->should_be_open(sim_day) || (hosp->get_occupied_bed_count() >= hosp->get_bed_count(sim_day))) {
        hh->set_is_primary_healthcare_available(false);
        hh->set_count_primary_hc_unav(hh->get_count_primary_hc_unav() + 1);
        Global::Daily_Tracker->increment_index_key_pair(sim_day, PRIMARY_HC_UNAV, 1);

        //Find an open healthcare provider
        hosp = Global::Places.get_random_open_hospital_matching_criteria(sim_day, this->myself, true, false);
        if(hosp == NULL) {
          hh->set_other_healthcare_location_that_accepts_insurance_available(false);
          hh->set_count_hc_accept_ins_unav(hh->get_count_hc_accept_ins_unav() + 1);
          Global::Daily_Tracker->increment_index_key_pair(sim_day, HC_ACCEP_INS_UNAV, 1);
          hosp = Global::Places.get_random_open_hospital_matching_criteria(sim_day, this->myself, false, false);
          if(hosp == NULL) {
            hh->set_is_healthcare_available(false);
            Global::Daily_Tracker->increment_index_key_pair(sim_day, HC_UNAV, 1);
          }
        }
      }

      if(hosp != NULL) {
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
        Global::Daily_Tracker->increment_index_key_pair(sim_day, ER_VISIT, 1);

        //Set the flag for the household
        hh->set_household_has_hospitalized_member(true);
      }
    } else {
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
}

void Activities::end_hospitalization() {
  if(Global::Enable_Hospitals) {
    this->is_hospitalized = false;
    this->sim_day_hospitalization_ends = -1;
    Hospital* tmp_hosp = static_cast<Hospital*>(this->myself->get_hospital());
    assert(tmp_hosp != NULL);
    tmp_hosp->decrement_occupied_bed_count();
    restore_daily_activity_locations();

    //Set the flag for the household
    static_cast<Household*>(this->myself->get_household())->set_household_has_hospitalized_member(false);
  }
}

bool Activities::default_sick_leave_behavior() {
  bool stay_home = false;
  if(this->my_sick_leave_decision_has_been_made) {
    stay_home = this->my_sick_leave_decision;
  } else {
    stay_home = (Random::draw_random() < Activities::Default_sick_day_prob);
    this->my_sick_leave_decision = stay_home;
    this->my_sick_leave_decision_has_been_made = true;
  }
  return stay_home;
}

void Activities::print_schedule(int day) {
  FRED_STATUS(0, "%s\n", this->schedule_to_string(day).c_str());
}

void Activities::print() {
  FRED_STATUS(0, "%s\n", this->to_string().c_str());
}

void Activities::assign_school() {
  int grade = this->myself->get_age();
  FRED_VERBOSE(1, "assign_school entered for person %d age %d grade %d\n",
	       this->myself->get_id(), this->myself->get_age(), grade);

  Household* hh = static_cast<Household*>(this->myself->get_household());

  if(hh == NULL) {
    if(Global::Enable_Hospitals && this->myself->is_hospitalized() && this->myself->get_permanent_household() != NULL) {
      hh = static_cast<Household*>(this->myself->get_permanent_household());
    }
  }
  assert(hh != NULL);
  Place* school = Global::Places.select_school(hh->get_county_index(), grade);
  assert(school != NULL);
  FRED_VERBOSE(1, "assign_school %s selected for person %d age %d\n",
	       school->get_label(), this->myself->get_id(), this->myself->get_age());
  set_school(school);
  set_classroom(NULL);
  assign_classroom();
  FRED_VERBOSE(1, "assign_school finished for person %d age %d: school %s classroom %s\n",
               this->myself->get_id(), this->myself->get_age(),
	             get_school()->get_label(), get_classroom()->get_label());
  return;
}

void Activities::assign_classroom() {
  if(School::get_max_classroom_size() == 0) {
    return;
  }
  assert(get_school() != NULL && get_classroom() == NULL);
  FRED_VERBOSE(1, "assign classroom entered\n");

  School* school = static_cast<School*>(get_school());
  Place* place = school->select_classroom_for_student(this->myself);
  if (place == NULL) {
    FRED_VERBOSE(0, "CLASSROOM_WARNING: assign classroom returns NULL: person %d age %d school %s\n",
                this->myself->get_id(), this->myself->get_age(), school->get_label());
  }
  set_classroom(place);
  FRED_VERBOSE(1,"assign classroom finished\n");
}

void Activities::assign_workplace() {
  Neighborhood_Patch* patch = NULL;
  if(Global::Enable_Hospitals && this->is_hospitalized) {
    patch = get_permanent_household()->get_patch();
  } else {
    patch = get_household()->get_patch();
  }
  assert(patch != NULL);
  Place* p = patch->select_workplace();
  change_workplace(p);
}

void Activities::assign_office() {
  if(get_workplace() != NULL && get_office() == NULL && get_workplace()->is_workplace()
     && Workplace::get_max_office_size() > 0) {
    Place* place = static_cast<Workplace*>(get_workplace())->assign_office(this->myself);
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

void Activities::update_profile() {
  int age = this->myself->get_age();

  // profiles for group quarters residents
  if(get_household()->is_college()) {
    if(this->profile != COLLEGE_STUDENT_PROFILE) {
      FRED_VERBOSE(1, "CHANGING PROFILE TO COLLEGE_STUDENT FOR PERSON %d AGE %d\n", this->myself->get_id(), age);
      this->profile = COLLEGE_STUDENT_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household_ptr(get_household()->get_index())->get_group_quarters_workplace());
    }
    return;
  }
  if(get_household()->is_military_base()) {
    if(this->profile != MILITARY_PROFILE) {
      FRED_VERBOSE(1, "CHANGING PROFILE TO MILITARY FOR PERSON %d AGE %d barracks %s\n", this->myself->get_id(), age, get_household()->get_label());
      this->profile = MILITARY_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household_ptr(get_household()->get_index())->get_group_quarters_workplace());
    }
    return;
  }
  if(get_household()->is_prison()) {
    if(this->profile != PRISONER_PROFILE) {
      FRED_VERBOSE(1,"CHANGING PROFILE TO PRISONER FOR PERSON %d AGE %d prison %s\n", this->myself->get_id(), age, get_household()->get_label());
      this->profile = PRISONER_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household_ptr(get_household()->get_index())->get_group_quarters_workplace());
    }
    return;
  }
  if(get_household()->is_nursing_home()) {
    if(this->profile != NURSING_HOME_RESIDENT_PROFILE) {
      FRED_VERBOSE(1,"CHANGING PROFILE TO NURSING HOME FOR PERSON %d AGE %d nursing_home %s\n", this->myself->get_id(), age, get_household()->get_label());
      this->profile = NURSING_HOME_RESIDENT_PROFILE;
      change_school(NULL);
      change_workplace(Global::Places.get_household_ptr(get_household()->get_index())->get_group_quarters_workplace());
    }
    return;
  }

  // updates for students finishing college
  if(this->profile == COLLEGE_STUDENT_PROFILE && get_household()->is_college() == false) {
    if(Random::draw_random() < 0.25) {
      // time to leave college for good
      change_school(NULL);
      change_workplace(NULL);
      // get a job
      this->profile = WORKER_PROFILE;
      assign_workplace();
      initialize_sick_leave();
      FRED_VERBOSE(1, "CHANGING PROFILE FROM COLLEGE STUDENT TO WORKER: id %d age %d sex %c HOUSE %s WORKPLACE %s OFFICE %s\n",
                   this->myself->get_id(), age, this->myself->get_sex(), get_household()->get_label(), get_workplace()->get_label(), get_office()->get_label());
    }
    return;
  }

  // update based on age

  if(this->profile == PRESCHOOL_PROFILE && Global::SCHOOL_AGE <= age && age < Global::ADULT_AGE) {
    FRED_VERBOSE(1,"CHANGING PROFILE TO STUDENT FOR PERSON %d AGE %d\n", this->myself->get_id(), age);
    this->profile = STUDENT_PROFILE;
    change_school(NULL);
    change_workplace(NULL);
    assign_school();
    assert(get_school() && get_classroom());
    FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d ENTERING SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                 this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
		             get_school()->get_orig_size(), get_classroom()->get_label());
    Activities::Tracking_data.entered_school++;
    return;
  }

  // rules for current students
  if(this->profile == STUDENT_PROFILE) {
    if(get_school() != NULL) {
      School* s = static_cast<School*>(get_school());
      // check if too old for current school
      if(s->get_max_grade() < age) {
        FRED_VERBOSE(1, "PERSON %d AGE %d TOO OLD FOR SCHOOL %s\n", this->myself->get_id(), age, s->get_label());
        if(age < Global::ADULT_AGE) {
          // find another school
          change_school(NULL);
          assign_school();
          assert(get_school() && get_classroom());
          FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d CHANGING TO SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                       this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
                       get_school()->get_orig_size(), get_classroom()->get_label());
        } else {
          // time to leave school
          FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d LEAVING SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                       this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
                       get_school()->get_orig_size(), get_classroom()->get_label());
          change_school(NULL);
          Activities::Tracking_data.left_school++;
          // get a job
          this->profile = WORKER_PROFILE;
          assign_workplace();
          initialize_sick_leave();
          FRED_VERBOSE(1, "CHANGING PROFILE FROM STUDENT TO WORKER: id %d age %d sex %c WORKPLACE %s OFFICE %s\n",
                       this->myself->get_id(), age, this->myself->get_sex(), get_workplace()->get_label(), get_office()->get_label());
        }
        return;
      }

      // not too old for current school.
      // make sure we're in an appropriate classroom
      Classroom* c = static_cast<Classroom*>(get_classroom());
      assert(c != NULL);

      // check if too old for current classroom
      if(c->get_age_level() != age) {
        // stay in this school if (1) the school offers this grade and (2) the grade is not too overcrowded (<150%)
        if(s->get_students_in_grade(age) < 1.5 * s->get_orig_students_in_grade(age)) {
          FRED_VERBOSE(1, "CHANGE_GRADES: PERSON %d AGE %d IN SCHOOL %s\n",
                       this->myself->get_id(), age, s->get_label());
          // re-enroll in current school -- this will assign an appropriate grade and classroom.
          change_school(s);
          assert(get_school() && get_classroom());
          FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d MOVE TO NEXT GRADE IN SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                       this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
                       get_school()->get_orig_size(), get_classroom()->get_label());
        } else {
          FRED_VERBOSE(1, "CHANGE_SCHOOLS: PERSON %d AGE %d NO ROOM in GRADE IN SCHOOL %s\n",
                       this->myself->get_id(), age, s->get_label());
          // find another school
          change_school(NULL);
          assign_school();
          assert(get_school() && get_classroom());
          FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d CHANGE TO NEW SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                       this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
                       get_school()->get_orig_size(), get_classroom()->get_label());
        }
        return;
      }

      // current school and classroom are ok
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d STAYING IN SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                   this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
                   get_school()->get_orig_size(), get_classroom()->get_label());
    } else {
      // no current school
      if(age < Global::ADULT_AGE) {
	      FRED_VERBOSE(1, "ADD_A_SCHOOL: PERSON %d AGE %d HAS NO SCHOOL\n", this->myself->get_id(), age);
	      change_school(NULL);
	      assign_school();
	      assert(get_school() && get_classroom());
	      Activities::Tracking_data.entered_school++;
	      FRED_VERBOSE(1, "STUDENT_UPDATE PERSON %d AGE %d ADDING SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
	                   this->myself->get_id(), age, get_school()->get_label(), get_school()->get_size(),
		                 get_school()->get_orig_size(), get_classroom()->get_label());
      } else {
	      // time to leave school
	      FRED_VERBOSE(1, "LEAVING_SCHOOL: PERSON %d AGE %d NO FORMER SCHOOL\n", this->myself->get_id(), age);
	      change_school(NULL);
	      // get a job
	      this->profile = WORKER_PROFILE;
	      assign_workplace();
	      initialize_sick_leave();
	      FRED_VERBOSE(1, "CHANGING PROFILE FROM STUDENT TO WORKER: id %d age %d sex %c WORKPLACE %s OFFICE %s\n",
	                   this->myself->get_id(), age, this->myself->get_sex(), get_workplace()->get_label(), get_office()->get_label());
      }
    }
    return;
  }

  // conversion to civilian life
  if(this->profile == PRISONER_PROFILE) {
    change_school(NULL);
    change_workplace(NULL);
    this->profile = WORKER_PROFILE;
    assign_workplace();
    initialize_sick_leave();
    FRED_VERBOSE(1, "CHANGING PROFILE FROM PRISONER TO WORKER: id %d age %d sex %c WORKPLACE %s OFFICE %s\n",
                 this->myself->get_id(), age, this->myself->get_sex(), get_workplace()->get_label(), get_office()->get_label());
    return;
  }

  // worker updates
  if(this->profile == WORKER_PROFILE) {
    if(get_workplace() == NULL) {
      assign_workplace();
      initialize_sick_leave();
      FRED_STATUS(1, "UPDATED  WORKER: id %d age %d sex %c\n%s\n", this->myself->get_id(),
                  age, this->myself->get_sex(), this->to_string().c_str());
    }
  }

  if(this->profile != RETIRED_PROFILE && Global::RETIREMENT_AGE <= age) {
    if(Random::draw_random()< 0.5 ) {
      FRED_STATUS(1, "CHANGING PROFILE TO RETIRED: id %d age %d sex %c\n", this->myself->get_id(), age, this->myself->get_sex());
      FRED_STATUS(1, "to_string: %s\n", this->to_string().c_str());
      // quit working
      if(is_teacher()) {
        change_school(NULL);
      }
      change_workplace(NULL);
      this->profile = RETIRED_PROFILE;
      initialize_sick_leave(); // no sick leave available if retired
      FRED_STATUS(1, "CHANGED BEHAVIOR PROFILE TO RETIRED: id %d age %d sex %c\n%s\n",
                  this->myself->get_id(), age, this->myself->get_sex(), this->to_string().c_str() );
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
    Household* my_hh = static_cast<Household*>(this->myself->get_household());
    if(my_hh != NULL) {
      my_hh->set_hh_schl_aged_chld_unemplyd_adlt_chng(true);
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
    Household* my_hh = static_cast<Household*>(this->myself->get_household());
    if(my_hh != NULL) {
      my_hh->set_hh_schl_aged_chld_unemplyd_adlt_chng(true);
    }
  }
  FRED_STATUS(1, "stop traveling: id = %d\n", this->myself->get_id());
}

bool Activities::become_a_teacher(Place* school) {
  bool success = false;
  FRED_VERBOSE(0, "become_a_teacher: person %d age %d\n", this->myself->get_id(), this->myself->get_age());
  // print(self);
  if(get_school() != NULL) {
    if(Global::Verbose > 0) {
      FRED_WARNING("become_a_teacher: person %d age %d ineligible -- already goes to school %d %s\n",
		   this->myself->get_id(), this->myself->get_age(), get_school()->get_id(), get_school()->get_label());
    }
    this->profile = STUDENT_PROFILE;
  } else {
    // set profile
    this->profile = TEACHER_PROFILE;
    // join the school
    FRED_VERBOSE(0, "set school to %s\n", school->get_label());
    set_school(school);
    set_classroom(NULL);
    success = true;
  }
  
  // withdraw from this workplace and any associated office
  Place* workplace = this->myself->get_workplace();
  FRED_VERBOSE(0, "leaving workplace %d %s\n", workplace->get_id(), workplace->get_label());
  change_workplace(NULL);
  FRED_VERBOSE(0, "become_a_teacher finished for person %d age %d\n", this->myself->get_id(),
	      this->myself->get_age());
  // print(self);
  return success;
}

void Activities::change_household(Place* place) {
  assert(place != NULL);
  set_household(place);
  set_neighborhood(place->get_patch()->get_neighborhood());
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

unsigned char Activities::get_deme_id() {
  Place* p;
  if(this->is_traveling_outside) {
    p = get_stored_household();
  } else {
    p = get_household();
  }
  assert(p->is_household());
  return static_cast<Household*>(p)->get_deme_id();
}

void Activities::move_to_new_house(Place* house) {

  FRED_VERBOSE(1, "move_to_new_house person %d house %s\n", this->myself->get_id(), house->get_label());

  // everyone must have a household
  assert(house != NULL); 

  bool is_former_group_quarters_resident = get_household()->is_group_quarters();
  if(is_former_group_quarters_resident || house->is_group_quarters()) {
    FRED_VERBOSE(1, "MOVE STARTED GROUP QUARTERS: person %d profile %c oldhouse %s newhouse %s\n",
                 this->myself->get_id(), this->myself->get_profile(), get_household()->get_label(), house->get_label());
  }
  // re-assign school and work activities
  change_household(house);

  if(is_former_group_quarters_resident || house->is_group_quarters()) {
    // this will re-assign school and work activities
    this->update_profile();
    FRED_VERBOSE(1, "MOVE FINISHED GROUP QUARTERS: person %d profile %c oldhouse %s newhouse %s\n",
                 this->myself->get_id(), this->myself->get_profile(), get_household()->get_label(), house->get_label());
  }
}

void Activities::terminate() {
  if(this->get_travel_status()) {
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
  int index = get_household()->get_county_index();
  Global::Places.decrement_population_of_county_with_index(index, this->myself);

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
  if (place) {
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
  FRED_VERBOSE(0, "JOINING NETWORK: id = %d\n", this->myself->get_id());
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return;
    }
  }
  Person_Network_Link* network_link = new Person_Network_Link(this->myself, network);
  this->networks.push_back(network_link);
}

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
  Utils::fred_abort("network not found");
  return false;
}

int Activities::get_out_degree(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_out_degree();
    }
  }
  Utils::fred_abort("network not found");
  return 0;
}

int Activities::get_in_degree(Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_in_degree();
    }
  }
  Utils::fred_abort("network not found");
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
  Utils::fred_abort("network not found");
}

Person* Activities::get_end_of_link(int n, Network* network) {
  int size = this->networks.size();
  for(int i = 0; i < size; ++i) {
    if(this->networks[i]->get_network() == network) {
      return this->networks[i]->get_end_of_link(n);
    }
  }
  Utils::fred_abort("network not found");
  return NULL;
}

int Activities::get_index_of_sick_leave_dist(Person* per) {
  if(!Activities::Enable_default_sick_behavior) {
    if(Activities::Sick_leave_dist_method == Activities::WP_SIZE_DIST) {
      int workplace_size = 0;
      if(per->get_workplace() != NULL) {
        workplace_size = per->get_workplace()->get_size();
      } else {
        if(per->is_teacher()) {
          workplace_size = per->get_school()->get_staff_size();
        }
      }
      // is sick leave available?
      if(workplace_size > 0) {
        for(int i = 0; i < Workplace::get_workplace_size_group_count(); ++i) {
          if(workplace_size <= Workplace::get_workplace_size_max_by_group_id(i)) {
            return i;
          }
        }
      }
    } else if(Activities::Sick_leave_dist_method == Activities::HH_INCOME_QTILE_DIST) {
      Household* hh = static_cast<Household*>(per->get_household());
      switch(hh->get_income_quartile()) {
        case Global::Q1:
          return 0;
        case Global::Q2:
          return 1;
        case Global::Q3:
          return 2;
        case Global::Q4:
          return 3;
      }
    }
  }
  return -1;
}

