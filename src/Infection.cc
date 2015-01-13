/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

// Infection.cc
// ------------
// Stores the infectivity and symptomaticity trajectories that determine the transition dates for this person.

#include <stdexcept>
#include "limits.h"

#include "Infection.h"
#include "Evolution.h"
#include "Transmission.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Workplace.h"
#include "Random.h"
#include "Disease.h"
#include "Health.h"
#include "IntraHost.h"
#include "Activities.h"
#include <map>
#include <vector>
#include <float.h>
#include "Utils.h"

using std::out_of_range;

Infection::Infection(Disease *disease, Person* infector, Person* host, Place* place, int day) {
  
  // flag for health updates
  Global::Pop.set_mask_by_index( fred::Update_Health, host->get_pop_index() );

  // general
  this->disease = disease;
  this->id = disease->get_id();
  this->infector = infector;
  this->host = host;
  this->place = place;
  this->trajectory = NULL;
  infectee_count = 0;
  age_at_exposure = host->get_age();

  isSusceptible = true;
  trajectory_infectivity_threshold = 0;
  trajectory_symptomaticity_threshold = 0;

  // parameters
  infectivity_multp = 1.0;
  infectivity = 0.0;
  susceptibility = 0.0;
  symptoms = 0.0;

  // chrono
  exposure_date = day;
  recovery_period = disease->get_days_recovered();

  offset = 0;

  if (symptomatic_date != -1) {
    will_be_symptomatic = true;
  } else {
    will_be_symptomatic = false;
  }

  // Determine if this infection produces an immune response
  immune_response = disease->gen_immunity_infection(host->get_age());
  //assert(immune_response == true);

  report_infection(day);
  // host->set_changed();
}

Infection::~Infection() {
  delete trajectory;

  for (std::vector< Transmission * >::iterator itr = transmissions.begin(); itr != transmissions.end(); ++itr) {
    delete (*itr);
  }
}

void Infection::determine_transition_dates() {
  // returns the first date that the agent changes state
  bool was_latent = true;
  bool was_incubating = true;

  latent_period = 0;
  asymptomatic_period = 0;
  symptomatic_period = 0;
  incubation_period = 0;

  infectious_date = -1;
  symptomatic_date = -1;
  asymptomatic_date = -1;
  recovery_date = exposure_date + trajectory->get_duration();

  Trajectory::iterator trj_it = Trajectory::iterator(trajectory);

  while (trj_it.has_next()) {
    bool infective = (trj_it.next().infectivity > trajectory_infectivity_threshold);
    bool symptomatic = (trj_it.next().symptomaticity > trajectory_symptomaticity_threshold);
    bool asymptomatic = (infective && !(symptomatic));

    if (infective && was_latent) {
      latent_period = trj_it.get_current();
      infectious_date = exposure_date + latent_period; // TODO

      if (asymptomatic & was_latent) {
        asymptomatic_date = infectious_date;
      }

      was_latent = false;
    }

    if (symptomatic && was_incubating) {
      incubation_period = trj_it.get_current();
      symptomatic_date = exposure_date + incubation_period;
      was_incubating = false;
    }

    if (symptomatic) {
      symptomatic_period++;
    }

    if (asymptomatic) {
      asymptomatic_period++;
    }
  }
}

void Infection::update(int today) {
  if(trajectory == NULL) return;

  // offset used for dummy infections only
  int days_post_exposure = today - exposure_date + offset;

  Trajectory::point trajectory_point = trajectory->get_data_point(days_post_exposure);
  infectivity = trajectory_point.infectivity;
  symptoms = trajectory_point.symptomaticity;

  if (today == get_infectious_date()) {
    host->become_infectious(disease);
  }

  if (today == get_symptomatic_date()) {
    host->become_symptomatic(disease);
  }

  if (today == get_recovery_date()) {
    host->recover(disease);
  }
 
  if (today == get_unsusceptible_date()) {
    host->become_unsusceptible(disease);
    isSusceptible = false;
  }
  
  if(today != get_recovery_date()) {
    vector<int> strains;
    trajectory->get_all_strains(strains);
    if ( Global::Report_Prevalence ) {
      host->addPrevalence(disease->get_id(), strains);
    }
  }
}

void Infection::modify_symptomatic_period(double multp, int today) {
  // negative multiplier
  if (multp < 0)
    throw out_of_range("cannot modify: negative multiplier");

  // after symptomatic period
  if (today >= get_recovery_date())
    throw out_of_range("cannot modify: past symptomatic period");

  // before symptomatic period
  else if (today < get_symptomatic_date()) {
    trajectory->modify_symp_period(symptomatic_date, symptomatic_period * multp);
    determine_transition_dates();
  }

  // during symptomatic period
  // if days_left becomes 0, we make it 1 so that update() sees the new dates
  else {
    //int days_into = today - get_symptomatic_date();
    int days_left = get_recovery_date() - today;
    days_left *= multp;
    trajectory->modify_symp_period(today - exposure_date + offset, days_left);
    determine_transition_dates();
  }
}

void Infection::modify_asymptomatic_period(double multp, int today) {
  // negative multiplier
  if (multp < 0)
    throw out_of_range("cannot modify: negative multiplier");

  // after asymptomatic period
  if (today >= get_symptomatic_date()) {
    printf("ERROR: Person %d %d %d\n", host->get_id(), today, get_symptomatic_date());
    throw out_of_range("cannot modify: past asymptomatic period");
  }

  // before asymptomatic period
  else if (today < get_infectious_date()) {
    trajectory->modify_asymp_period(exposure_date, asymptomatic_period * multp, get_symptomatic_date());
    determine_transition_dates();
  }

  // during asymptomatic period
  else {
    //int days_into = today - get_infectious_date();
    int days_left = get_symptomatic_date() - today;
    trajectory->modify_asymp_period(today - exposure_date, days_left * multp, get_symptomatic_date());
    determine_transition_dates();
  }
}

void Infection::modify_infectious_period(double multp, int today) {
  if (today < get_symptomatic_date())
    modify_asymptomatic_period(multp, today);

  modify_symptomatic_period(multp, today);
}

void Infection::modify_develops_symptoms(bool symptoms, int today) {
  if ((today >= get_symptomatic_date() && get_asymptomatic_date() == -1) || (today >= get_recovery_date()) )
    throw out_of_range("cannot modify: past symptomatic period");

  if (will_be_symptomatic != symptoms) {
    symptomatic_period = will_be_symptomatic ? disease->get_days_symp() : 0;
    trajectory->modify_develops_symp(get_symptomatic_date(), symptomatic_period);
    determine_transition_dates();
  }
}

void Infection::print() const {
  printf("Infection of disease type: %i in person %i\n"
         "periods:  latent %i, asymp: %i, symp: %i recovery: %i \n"
         "dates: exposed: %i, infectious: %i, symptomatic: %i, recovered: %i susceptible: %i\n"
         "will have symp? %i, suscept: %.3f infectivity: %.3f "
         "infectivity_multp: %.3f symptms: %.3f\n",
         disease->get_id(),
         host->get_id(),
         latent_period,
         asymptomatic_period,
         symptomatic_period,
         recovery_period,
         exposure_date,
         get_infectious_date(),
         get_symptomatic_date(),
         get_recovery_date(),
         get_susceptible_date(),
         will_be_symptomatic,
         susceptibility,
         infectivity,
         infectivity_multp,
         symptoms);
}

// static
Infection *Infection::get_dummy_infection(Disease *s, Person* host, int day) {
  Infection* i = new Infection(s, NULL, host, NULL, day);
  i->dummy = true;
  i->offset = i->infectious_date - day;
  return i;
}

void Infection::transmit(Person *infectee, Transmission *transmission) {
  int day = transmission->get_exposure_date() - exposure_date + offset;
  map< int, double > * loads = trajectory->getInoculum( day );
  transmission->set_initial_loads( loads );
  infectee->become_exposed( this->disease, transmission );
}

void Infection::setTrajectory(Trajectory *trajectory) {
  this->trajectory = trajectory;
  this->determine_transition_dates();
}

void Infection::report_infection(int day) const {
  if (Global::Infectionfp == NULL) return;

  int place_id = place == NULL? -1 : place->get_id();
  char place_type = place == NULL? 'X' : place->get_type();
  int place_size = place == NULL? -1: place->get_size();
  if (place_type == 'O' || place_type == 'C') {
    Place *container = place->get_container();
    place_size = container->get_size();
  }

  fprintf(Global::Infectionfp, "day %d dis %d host %d age %.3f sick_leave %d"
          " infector %d inf_age %.3f inf_sympt %d inf_sick_leave %d at %c place %d size %d  ",
          day, id,
          host->get_id(),
          host->get_real_age(),
          host->is_sick_leave_available(),
          infector == NULL ? -1 : infector->get_id(),
          infector == NULL ? -1 : infector->get_real_age(),
          infector == NULL ? -1 : infector->is_symptomatic(),
          infector == NULL ? -1 : infector->is_sick_leave_available(),
    place_type, place_id, place_size);

  if (Global::Track_infection_events > 1)
    fprintf(Global::Infectionfp,
            "| PERIODS  latent %d asymp %d symp %d recovery %d ",
            latent_period,
            asymptomatic_period,
            symptomatic_period,
            recovery_period);

  if (Global::Track_infection_events > 2)
    fprintf(Global::Infectionfp,
            "| DATES exp %d inf %d symp %d rec %d sus %d ",
            exposure_date,
            get_infectious_date(),
            get_symptomatic_date(),
            get_recovery_date(),
            get_susceptible_date());

  if (Global::Track_infection_events > 3)
    fprintf(Global::Infectionfp,
            "| will_be_symp? %d susc %.3f infect %.3f "
            "inf_multp %.3f symptms %.3f ",
            will_be_symptomatic,
            susceptibility,
            infectivity,
            infectivity_multp,
            symptoms);

  fprintf(Global::Infectionfp, "\n");
  // flush performed at the end of every day so that it doesn't gum up multithreading
  //fflush(Global::Infectionfp);
}

int Infection::get_num_past_infections()
{ 
  return host->get_num_past_infections(id); 
}
  
Past_Infection *Infection::get_past_infection(int i)
{ 
  return host->get_past_infection(id, i); 
}


