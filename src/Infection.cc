/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Infection.cc

#include "Infection.h"
#include "Disease.h"
#include "Global.h"
#include "Household.h"
#include "Person.h"
#include "Place.h"
#include "Place_List.h"

#define NEVER (-1)

//
// Terminology:
//
// latent period = time between exposure and infectiousness
//
// incubation period = time between exposure and appearance of first symptoms
//
// Refs:
// https://en.wikipedia.org/wiki/Incubation_period
//
// http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2817319/
//
// Estimated epidemiologic parameters and morbidity associated with
// pandemic H1N1 influenza CMAJ. 2010 Feb 9; 182(2): 131–136.
// Ashleigh R. Tuite, MSc, MHSc, Amy L. Greer, MSc, PhD, Michael Whelan,
// MSc, Anne-Luise Winter, BScN, MHSc, Brenda Lee, MHSc, Ping Yan, PhD,
// Jianhong Wu, PhD, Seyed Moghadas, PhD, David Buckeridge, MD, PhD,
// Babak Pourbohloul, PhD, and David N. Fisman, MD, MPH
//
//
// Note: the details for a particular disease is handled by the
// Natural_History class through the methods:
// 
// get_latent_period()
// get_duration_of_infectiousness()
//
// get_incubation_period()
// get_duration_of_symptoms()
//
// The Infection class merely reports the resulting transition dates.
//

Infection::Infection(Disease* _disease, Person* _infector, Person* _host, Place* _place, int day) {

  // FRED_VERBOSE(0,"Infection constructor entered\n");

  this->disease = _disease;
  this->infector = _infector;
  this->host = _host;
  this->place = _place;
  this->exposure_date = day;
  this->infectious_start_date = -1;
  this->infectious_end_date = -1;
  this->symptoms_start_date = -1;
  this->symptoms_end_date = -1;
  this->immunity_end_date = -1;

  set_transition_dates();
}

void Infection::set_transition_dates() {

  // set transition dates for infectiousness
  int my_latent_period = disease->get_latent_period(this->host);
  if (my_latent_period < 0) {
    this->infectious_start_date = NEVER;
    this->infectious_end_date = NEVER;
  }
  else  {
    assert(my_latent_period > 0); // FRED needs at least one day to become infectious
    this->infectious_start_date = this->exposure_date + my_latent_period;
    
    int my_duration_of_infectiousness = disease->get_duration_of_infectiousness(this->host);
    // my_duration_of_infectiousness <= 0 would mean "never infectious"
    assert(my_duration_of_infectiousness > 0);
    this->infectious_end_date = this->infectious_start_date + duration_of_infectiousness;
  }
  
  // set transition dates for having symptoms
  int my_incubation_period = disease->get_incubation_period(this->host);
  if (my_incubation_period < 0) {
    this->symptoms_start_date = NEVER;
    this->symptoms_end_date = NEVER;
  }
  else {
    assert(my_incubation_period > 0); // FRED needs at least one day to become symptomatic
    this->symptoms_start_date = this->exposure_date + my_incubation_period;

    int my_duration_of_symptoms = disease->get_duration_of_symptoms(this->host);
    // duration_of_symptoms <= 0 would mean "never symptomatic"
    assert(my_duration_of_symptoms > 0);
    this->symptoms_end_date = this->symptoms_start_date + my_duration_of_symptoms;
  }

  // set transition date for becoming susceptible after this infection
  int my_duration_of_immunity = this->disease->get_duration_of_immunity(this->host);
  if (my_duration_of_immunity < 0) {
    // my_duration_of_immunity < 0 means "immune forever"
    this->immunity_end_date = NEVER;
  }
  else {
    assert(my_duration_of_immunity > 0); // FRED needs at least one day to become susceptible again
    this->immunity_end_date = this->exposure_date + my_duration_of_immunity;
  }

}

void Infection::update(int day) {
  /*
  if (day < this->infectious_start_date) {
    int new_infectious_start_date = this->disease->update_infectious_start_date(this->host, this->day, this->exposure_date, this->infectious_start_date);
    // note: the following could be wrapped into the above method in disease
    if (new_infectious_start_date != this->infectious_start_date) {
      this->disease->get_epidemic()->change_infectious_start_event(this->host, this->infectious_start_date, new_infectious_start_date);
    }
  }
  */
}

bool Infection::is_infectious(int day) const {
  if (this->infectious_start_date != NEVER) {
    return (this->infectious_start_date <= day && day < this->infectious_end_date);
  }
  else {
    return false;
  }
}

bool Infection::is_symptomatic(int day) const {
  if (this->symptoms_start_date != NEVER) {
    return (this->symptoms_start_date <= day && day < this->symptoms_end_date);
  }
  else {
    return false;
  }
}


void Infection::print() const {
  printf("INF: Infection of disease type: %d in person %d "
	 "dates: exposed: %d, infectious_start: %d, infectious_end: %d "
	 "symptoms_start: %d, symptoms_end: %d\n",
	 this->disease->get_id(), this->host->get_id(),
	 this->exposure_date, this->infectious_start_date, this->infectious_end_date, 
	 this->symptoms_start_date, this->symptoms_end_date);
}

void Infection::report_infection(int day) const {
  if(Global::Infectionfp == NULL) {
    return;
  }

  int place_id = (this->place == NULL ? -1 : this->place->get_id());
  char place_type = (this->place == NULL ? 'X' : this->place->get_type());
  char place_subtype = 'X';
  if(this->place != NULL && this->place->is_group_quarters()) {
    if(this->place->is_college()) {
      place_subtype = 'D';
    }
    if(this->place->is_prison()) {
      place_subtype = 'J';
    }
    if(this->place->is_nursing_home()) {
      place_subtype = 'L';
    }
    if(this->place->is_military_base()) {
      place_subtype = 'B';
    }
  }
  int place_size = (this->place == NULL ? -1 : this->place->get_container_size());
  std::stringstream infStrS;
  infStrS.precision(3);
  infStrS << fixed << "day " << day << " dis " << this->disease->get_id() << " host " << this->host->get_id()
	  << " age " << this->host->get_real_age() << " sick_leave " << this->host->is_sick_leave_available()
	  << " infector " << (this->infector == NULL ? -1 : this->infector->get_id()) << " inf_age "
	  << (this->infector == NULL ? -1 : this->infector->get_real_age()) << " inf_sympt "
	  << (this->infector == NULL ? -1 : this->infector->is_symptomatic()) << " inf_sick_leave "
	  << (this->infector == NULL ? -1 : this->infector->is_sick_leave_available())
	  << " at " << place_type << " place " <<  place_id << " subtype " << place_subtype;
  infStrS << " size " << place_size << " is_teacher " << (int)this->host->is_teacher();

  if (place_type != 'X') {
    fred::geo lat = this->place->get_latitude();
    fred::geo lon = this->place->get_longitude();
    infStrS << " lat " << lat;
    infStrS << " lon " << lon;
  }
  else {
    infStrS << " lat " << -999;
    infStrS << " lon " << -999;
  }
  double host_lat = this->host->get_household()->get_latitude();
  double host_lon = this->host->get_household()->get_longitude();
  infStrS << " home_lat " << host_lat;
  infStrS << " home_lon " << host_lon;
  infStrS << " infector_exp_date " << (this->infector == NULL ? -1 : this->infector->get_exposure_date(disease->get_id()));
  infStrS << " | DATES exp " << this->exposure_date << " inf " << get_infectious_start_date() << " symp "
	  << get_symptoms_start_date() << " rec " << get_infectious_end_date() << " sus "
	  << get_immunity_end_date();

  if(Global::Track_infection_events > 1) {
    if(place_type != 'X'  && infector != NULL) {
      double host_x = this->host->get_x();
      double host_y = this->host->get_y();
      double infector_x = this->infector->get_x();
      double infector_y = this->infector->get_y();
      double distance = sqrt((host_x-infector_x)*(host_x-infector_x) +
			     (host_y-infector_y)*(host_y-infector_y));
      infStrS << " dist " << distance;
    } else {
      infStrS << " dist -1 ";
    }
    //Add Census Tract information. If there was no infector, censustract is -1
    if(this->infector == NULL) {
      infStrS << " census_tract -1";
    } else {

      Household* hh = static_cast<Household*>(this->infector->get_household());
      if(hh == NULL) {
        if(Global::Enable_Hospitals && this->infector->is_hospitalized() && this->infector->get_permanent_household() != NULL) {
          hh = static_cast<Household*>(this->infector->get_permanent_household());;
        }
      }
      int census_tract_index = (hh == NULL ? -1 : hh->get_census_tract_index());
      long int census_tract = (census_tract_index == -1 ? -1 : Global::Places.get_census_tract_with_index(census_tract_index));
      infStrS << " census_tract " << census_tract;
    }
  }
  infStrS << "\n";
  fprintf(Global::Infectionfp, "%s", infStrS.str().c_str());
}

double Infection::get_infectivity(int day) const {
  return (is_infectious(day) ? 1.0 : 0.0);
}

double Infection::get_symptoms(int day) const {
  return (is_symptomatic(day) ? 1.0 : 0.0);
}



