/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Infection.cc


#include "Disease.h"
#include "Global.h"
#include "HIV_Infection.h"
#include "Household.h"
#include "Infection.h"
#include "Mixing_Group.h"
#include "Natural_History.h"
#include "Neighborhood_Patch.h"
#include "Person.h"
#include "Place_List.h"
#include "Random.h"
#include "Neighborhood_Patch.h"
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

// if primary infection, infector and place are null.
Infection::Infection(Disease* _disease, Person* _infector, Person* _host, Mixing_Group* _mixing_group, int day) {
  this->disease = _disease;
  this->infector = _infector;
  this->host = _host;
  this->mixing_group = _mixing_group;
  this->exposure_date = day;
  this->infectious_start_date = -1;
  this->infectious_end_date = -1;
  this->symptoms_start_date = -1;
  this->symptoms_end_date = -1;
  this->immunity_end_date = -1;
  this->will_develop_symptoms = false;
  this->infection_is_fatal_today = false;
}

/**
 * This static factory method is used to get an instance of a specific
 * Infection that tracks patient-specific data that depends on the
 * natural history model associated with the disease.
 *
 * @param a pointer to the disease causing this infection.
 * @return a pointer to a specific Infection object of a possible derived class
 */

Infection* Infection::get_new_infection(Disease* disease, Person* infector, Person* host, Mixing_Group* mixing_group, int day) {
  if(strcmp(disease->get_natural_history_model(), "basic") == 0) {
    return new Infection(disease, infector, host, mixing_group, day);
  }
  
  if(strcmp(disease->get_natural_history_model(), "hiv") == 0) {
    return new HIV_Infection(disease, infector, host, mixing_group, day);
  }
  
  Utils::fred_abort("Infection::get_new_infection -- unknown natural history model: %s",
		    disease->get_natural_history_model());
  return NULL;
}


/*
 * The Infection base class defines a SEIR model.
 * For other models, define a dervived class based on the Infection base class.
 */


void Infection::setup() {

  FRED_VERBOSE(1, "infection::setup entered\n");

  // decide if this host will develop symptoms
  double prob_symptoms = this->disease->get_natural_history()->get_probability_of_symptoms(this->host->get_age());
  this->will_develop_symptoms = (Random::draw_random() < prob_symptoms);

  // set transition date for becoming susceptible after this infection
  int my_duration_of_immunity = this->disease->get_natural_history()->get_duration_of_immunity(this->host);
  // my_duration_of_immunity <= 0 means "immune forever"
  if(my_duration_of_immunity > 0) {
    this->immunity_end_date = this->exposure_date + my_duration_of_immunity;
  } else {
    this->immunity_end_date = NEVER;
  }

  double incubation_period = 0.0;
  double symptoms_duration = 0.0;

  // determine dates for symptoms
  int symptoms_distribution_type = disease->get_natural_history()->get_symptoms_distribution_type();
  if(symptoms_distribution_type == LOGNORMAL) {
    incubation_period = disease->get_natural_history()->get_real_incubation_period(this->host);
    symptoms_duration = disease->get_natural_history()->get_symptoms_duration(this->host);

    // find symptoms dates (assuming symptoms will occur)
    this->symptoms_start_date = this->exposure_date + round(incubation_period);
    this->symptoms_end_date = this->exposure_date + round(incubation_period + symptoms_duration);
  } else {
    // distribution type == CDF
    int my_incubation_period = disease->get_natural_history()->get_incubation_period(this->host);
    assert(my_incubation_period > 0); // FRED needs at least one day to become symptomatic
    this->symptoms_start_date = this->exposure_date + my_incubation_period;
      
    int my_duration_of_symptoms = disease->get_natural_history()->get_duration_of_symptoms(this->host);
    // duration_of_symptoms <= 0 would mean "symptomatic forever"
    if(my_duration_of_symptoms > 0) {
      this->symptoms_end_date = this->symptoms_start_date + my_duration_of_symptoms;
    } else {
      this->symptoms_end_date = NEVER;
    }
  }

  // determine dates for infectiousness
  int infectious_distribution_type = disease->get_natural_history()->get_infectious_distribution_type();
  if(infectious_distribution_type == OFFSET_FROM_START_OF_SYMPTOMS ||
     infectious_distribution_type == OFFSET_FROM_SYMPTOMS) {
    // set infectious dates based on offset
    double infectious_start_offset = disease->get_natural_history()->get_infectious_start_offset(this->host);
    double infectious_end_offset = disease->get_natural_history()->get_infectious_end_offset(this->host);

    // apply the offset
    this->infectious_start_date = this->symptoms_start_date + round(infectious_start_offset);
    if(infectious_distribution_type == OFFSET_FROM_START_OF_SYMPTOMS) {
      this->infectious_end_date = this->symptoms_start_date + round(infectious_end_offset);
    } else {
      this->infectious_end_date = this->symptoms_end_date + round(infectious_end_offset);
    }
  } else if(infectious_distribution_type == LOGNORMAL) {
    double latent_period = disease->get_natural_history()->get_real_latent_period(this->host);
    double infectious_duration = disease->get_natural_history()->get_infectious_duration(this->host);
    this->infectious_start_date = this->exposure_date + round(latent_period);
    this->infectious_end_date = this->exposure_date + round(latent_period + infectious_duration);
  } else {
    // distribution type == CDF
    int my_latent_period = disease->get_natural_history()->get_latent_period(this->host);
    if(my_latent_period < 0) {
      this->infectious_start_date = NEVER;
      this->infectious_end_date = NEVER;
    } else  {
      assert(my_latent_period > 0); // FRED needs at least one day to become infectious
      this->infectious_start_date = this->exposure_date + my_latent_period;
      
      int my_duration_of_infectiousness = disease->get_natural_history()->get_duration_of_infectiousness(this->host);
      // my_duration_of_infectiousness <= 0 means "infectious forever"
      if(my_duration_of_infectiousness > 0) {
	      this->infectious_end_date = this->infectious_start_date + my_duration_of_infectiousness;
      } else {
	      this->infectious_end_date = NEVER;
      }
    }
  }

  // code for testing ramps
  if (Global::Test > 1) {
    for (int i = 0; i <= 1000; i++) {
      double dur = (this->infectious_end_date-this->infectious_start_date+1);
      double t = 0.001*i*dur;
      double start_full = this->disease->get_natural_history()->get_full_infectivity_start();
      double end_full = this->disease->get_natural_history()->get_full_infectivity_end();
      double x = t / dur;
      double result;
      if(x < start_full) {
	result = exp(x/start_full - 1.0);
      } else if(x < end_full) {
	result = 1.0;
      } else {
	result = exp(-3.5*(x-end_full)/(1.0-end_full));
      }
      printf("RAMP: %f %f %d %d \n", t, result, this->infectious_start_date, this->infectious_end_date);
    }
    Global::Test = 0;
    abort();
  }

  if (Global::Verbose > 1) {
    printf("INFECTION day %d incub %0.2f symp_onset %d symp_dur %0.2f symp_dur %2d symp_start_date %d inf_start_date %d inf_end_date %d inf_onset %d inf_dur %d ",
	   this->exposure_date, incubation_period, this->symptoms_start_date-this->exposure_date,
	   symptoms_duration, this->symptoms_end_date-this->symptoms_start_date,
	   this->symptoms_start_date, this->infectious_start_date,  this->infectious_end_date, 
	   this->infectious_start_date-this->exposure_date,
	   this->infectious_end_date-this->infectious_start_date);
    for (int d = this->infectious_start_date; d <= this->infectious_end_date; d++) {
      printf("%f ", get_infectivity(d));
    }
    printf("\n");
    fflush(stdout);
  }

  // adjust symptoms date if asymptomatic
  if(this->will_develop_symptoms == false) {
    this->symptoms_start_date = NEVER;
    this->symptoms_end_date = NEVER;
  }
  return;

  // print();
}



void Infection::print() {
  printf("INF: Infection of disease type: %d in person %d "
	 "dates: exposed: %d, infectious_start: %d, infectious_end: %d "
	 "symptoms_start: %d, symptoms_end: %d\n",
	 this->disease->get_id(), this->host->get_id(),
	 this->exposure_date, this->infectious_start_date, this->infectious_end_date, 
	 this->symptoms_start_date, this->symptoms_end_date);
  fflush(stdout);
}

void Infection::report_infection(int day) {
  if(Global::Infectionfp == NULL) {
    return;
  }

  int mixing_group_id = (this->mixing_group == NULL ? -1 : this->mixing_group->get_id());
  char mixing_group_type = (this->mixing_group == NULL ? 'X' : this->mixing_group->get_type());
  char mixing_group_subtype = 'X';
  if(this->mixing_group != NULL && dynamic_cast<Place*>(this->mixing_group) != NULL) {
    Place* place = dynamic_cast<Place*>(this->mixing_group);
    if(place->is_group_quarters()) {
      if(place->is_college()) {
        mixing_group_subtype = 'D';
      }
      if(place->is_prison()) {
        mixing_group_subtype = 'J';
      }
      if(place->is_nursing_home()) {
        mixing_group_subtype = 'L';
      }
      if(place->is_military_base()) {
        mixing_group_subtype = 'B';
      }
    }
  }
  int mixing_group_size = (this->mixing_group == NULL ? -1 : this->mixing_group->get_container_size());
  std::stringstream infStrS;
  infStrS.precision(3);
  infStrS << fixed << "day " << day << " dis " << this->disease->get_disease_name() << " host " << this->host->get_id()
	  << " age " << this->host->get_real_age()
	  << " | DATES exp " << this->exposure_date
	  << " inf " << get_infectious_start_date() << " " << get_infectious_end_date()
	  << " symp " << get_symptoms_start_date() << " " << get_symptoms_end_date()
	  << " rec " << get_infectious_end_date() << " sus " << get_immunity_end_date()
	  << " infector_exp_date " << (this->infector == NULL ? -1 : this->infector->get_exposure_date(this->disease->get_id()))
	  << " | ";

  if(Global::Track_infection_events > 1) {
    infStrS << " sick_leave " << this->host->is_sick_leave_available()
	    << " infector " << (this->infector == NULL ? -1 : this->infector->get_id()) << " inf_age "
	    << (this->infector == NULL ? -1 : this->infector->get_real_age()) << " inf_sympt "
	    << (this->infector == NULL ? -1 : this->infector->is_symptomatic()) << " inf_sick_leave "
	    << (this->infector == NULL ? -1 : this->infector->is_sick_leave_available())
	    << " at " << mixing_group_type << " mixing_group " <<  mixing_group_id << " subtype " << mixing_group_subtype;
    infStrS << " size " << mixing_group_size << " is_teacher " << (int)this->host->is_teacher();
    
    if(dynamic_cast<Place*>(this->mixing_group) != NULL) {
      Place* place = dynamic_cast<Place*>(this->mixing_group);
      if(mixing_group_type != 'X') {
        fred::geo lat = place->get_latitude();
        fred::geo lon = place->get_longitude();
        infStrS << " lat " << lat;
        infStrS << " lon " << lon;
      } else {
        infStrS << " lat " << -999;
        infStrS << " lon " << -999;
      }
      double host_lat = this->host->get_household()->get_latitude();
      double host_lon = this->host->get_household()->get_longitude();
      infStrS << " home_lat " << host_lat;
      infStrS << " home_lon " << host_lon;
      infStrS << " | ";
    }
  }

  if(Global::Track_infection_events > 2) {
    if(mixing_group_type != 'X'  && this->infector != NULL) {
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
      infStrS << " infctr_census_tract -1";
      infStrS << " host_census_tract -1";
    } else {

      Household* hh = static_cast<Household*>(this->infector->get_household());
      if(hh == NULL) {
        if(Global::Enable_Hospitals && this->infector->is_hospitalized() && this->infector->get_permanent_household() != NULL) {
          hh = static_cast<Household*>(this->infector->get_permanent_household());
        }
      }
      int census_tract_index = (hh == NULL ? -1 : hh->get_census_tract_index());
      long int census_tract = (census_tract_index == -1 ? -1 : Global::Places.get_census_tract_with_index(census_tract_index));
      infStrS << " infctr_census_tract " << census_tract;


      hh = static_cast<Household*>(this->host->get_household());
      if(hh == NULL) {
        if(Global::Enable_Hospitals && this->host->is_hospitalized() && this->host->get_permanent_household() != NULL) {
          hh = static_cast<Household*>(this->host->get_permanent_household());
        }
      }
      census_tract_index = (hh == NULL ? -1 : hh->get_census_tract_index());
      census_tract = (census_tract_index == -1 ? -1 : Global::Places.get_census_tract_with_index(census_tract_index));
      infStrS << " host_census_tract " << census_tract;
    }
    infStrS << " | ";
  }
  if(Global::Track_infection_events > 3){
    Neighborhood_Patch* pt = this->host->get_household()->get_patch();
    if(pt != NULL){
      double patch_lat = Geo::get_latitude(pt->get_center_y());
      double patch_lon = Geo::get_longitude(pt->get_center_x());
      int patch_pop = pt->get_popsize();
      infStrS << " patch_lat " << patch_lat;
      infStrS << " patch_lon " << patch_lon;
      infStrS << " patch_pop " << patch_pop;
      infStrS << " | ";
    }
  }
  infStrS << "\n";
  fprintf(Global::Infectionfp, "%s", infStrS.str().c_str());
}


void Infection::update(int today) {
  // if host is symptomatic, determine if infection is fatal today.
  // if so, set flag and terminate infection update.
  if(this->disease->is_case_fatality_enabled() && is_symptomatic(today)) {
    int days_symptomatic = today - this->symptoms_start_date;
    if(Global::Enable_Chronic_Condition) {
      if(this->disease->is_fatal(this->host, get_symptoms(today), days_symptomatic)) {
	      set_fatal_infection();
      }
    } else {
      if(this->disease->is_fatal(this->host->get_real_age(), get_symptoms(today), days_symptomatic)) {
	      set_fatal_infection();
      }
    }
  }
}

double Infection::get_infectivity(int day) {
  if(day < this->infectious_start_date || this->infectious_end_date <= day) {
    FRED_VERBOSE(0,"INFECTION: day %d OUT OF BOUNDS inf_start %d inf_end %d result 0.0\n",
		 day, this->infectious_start_date, this->infectious_end_date);
    return 0.0;
  }

  // day is during infectious period
  double start_full = this->disease->get_natural_history()->get_full_infectivity_start();
  double end_full = this->disease->get_natural_history()->get_full_infectivity_end();

  // assumes total duration of infectiousness starts one day before infectious_start_date:
  int total_duration = this->infectious_end_date - this->infectious_start_date + 1;
  int days_infectious = day - this->infectious_start_date + 1;
  double fraction = static_cast<double>(days_infectious) / static_cast<double>(total_duration);
  double result = 0.0;
  if(fraction < start_full) {
    result = exp(fraction/start_full - 1.0);
  } else if(fraction <= end_full) {
    result = 1.0;
  } else {
    result = exp(-3.5*(fraction-end_full)/(1.0-end_full));
  }
  FRED_VERBOSE(1,"INFECTION: day %d days_infectious %d / %d  fract %f start_full %f end_full %f result %f\n", 
	       day, days_infectious, total_duration, fraction, start_full, end_full, result);

  if (this->will_develop_symptoms == false) {
    result *= this->disease->get_natural_history()->get_asymptomatic_infectivity();
  }
  return result;
}

double Infection::get_symptoms(int day) {
  if(day < this->symptoms_start_date || this->symptoms_end_date <= day) {
    return 0.0;
  }

  // day is during symptoms period
  double start_full = this->disease->get_natural_history()->get_full_symptoms_start();
  double end_full = this->disease->get_natural_history()->get_full_symptoms_end();

  // assumes total duration of symptoms starts one day before symptoms_start_date:
  int total_duration = this->symptoms_end_date - this->symptoms_start_date + 1;

  int days_symptomatic = day - this->symptoms_start_date + 1;
  double fraction = static_cast<double>(days_symptomatic) / static_cast<double>(total_duration);
  double result = 0.0;
  if(fraction < start_full) {
    result = exp(fraction/start_full - 1.0);
  } else if(fraction <= end_full) {
    result = 1.0;
  } else {
    result = exp(-3.5*(fraction-end_full)/(1.0-end_full));
  }
  return result;
}


