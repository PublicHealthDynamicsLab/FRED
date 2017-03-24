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
// File: Health.cc
//
#include <new>
#include <stdexcept>

#include "Age_Map.h"
#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Health.h"
#include "HIV_Infection.h"
#include "Household.h"
#include "Natural_History.h"
#include "Mixing_Group.h"
#include "Person.h"
#include "Place.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "Sexual_Transmission_Network.h"
#include "Utils.h"

// static variables
Age_Map* Health::pregnancy_hospitalization_prob_mult = NULL;
Age_Map* Health::pregnancy_case_fatality_prob_mult = NULL;
bool Health::is_initialized = false;
double Health::Hh_income_susc_mod_floor = 0.0;
double Health::health_insurance_distribution[Insurance_assignment_index::UNSET];
int Health::health_insurance_cdf_size = 0;


// static method called in main (Fred.cc)

void Health::initialize_static_variables() {
  //Setup the static variables if they aren't already initialized
  if(!Health::is_initialized) {

    int temp_int = 0;

    if(Global::Enable_Health_Insurance) {

      Health::health_insurance_cdf_size = Params::get_param_vector((char*)"health_insurance_distribution", Health::health_insurance_distribution);

      // convert to cdf
      double stotal = 0;
      for(int i = 0; i < Health::health_insurance_cdf_size; ++i) {
        stotal += Health::health_insurance_distribution[i];
      }
      if(stotal != 100.0 && stotal != 1.0) {
        Utils::fred_abort("Bad distribution health_insurance_distribution params_str\nMust sum to 1.0 or 100.0\n");
      }
      double cumm = 0.0;
      for(int i = 0; i < Health::health_insurance_cdf_size; ++i) {
        Health::health_insurance_distribution[i] /= stotal;
        Health::health_insurance_distribution[i] += cumm;
        cumm = Health::health_insurance_distribution[i];
      }
    }

    Health::is_initialized = true;
  }
}

Insurance_assignment_index::e Health::get_health_insurance_from_distribution() {
  if(Global::Enable_Health_Insurance && Health::is_initialized) {
    int i = Random::draw_from_distribution(Health::health_insurance_cdf_size, Health::health_insurance_distribution);
    return Health::get_insurance_type_from_int(i);
  } else {
    return Insurance_assignment_index::UNSET;
  }
}

Health::Health() {
  this->myself = NULL;
  this->alive = true;
  this->days_symptomatic = 0;
  this->previous_infection_serotype = 0;
  this->insurance_type = Insurance_assignment_index::UNSET;
  this->health_condition = NULL;
  this->hiv_infection = NULL;
}

void Health::setup(Person* self) {
  this->myself = self;
  FRED_VERBOSE(1, "Health::setup for person %d\n", myself->get_id());
  this->alive = true;
  
  this->number_of_conditions = Global::Conditions.get_number_of_conditions();
  FRED_VERBOSE(1, "Health::setup conditions %d\n", this->number_of_conditions);

  this->health_condition = new health_condition_t [this->number_of_conditions];

  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    this->health_condition[condition_id].natural_history = static_cast<Natural_History*>(Global::Conditions.get_condition(condition_id)->get_natural_history());
    this->health_condition[condition_id].state = -1;
    this->health_condition[condition_id].last_transition_day = -1;
    this->health_condition[condition_id].next_transition_day = -1;
    this->health_condition[condition_id].onset_day = -1;
    this->health_condition[condition_id].symptoms_level = 0;
    this->health_condition[condition_id].symptoms_start_date = -1;
    this->health_condition[condition_id].is_fatal = false;
    this->health_condition[condition_id].is_immune = false;
    this->health_condition[condition_id].is_infected = false;
    this->health_condition[condition_id].is_recovered = false;
    this->health_condition[condition_id].susceptibility = 1.0;
    this->health_condition[condition_id].infectivity = 0.0;
    this->health_condition[condition_id].infector = NULL;
    this->health_condition[condition_id].mixing_group = NULL;
    this->health_condition[condition_id].number_of_infectees = 0;
    this->health_condition[condition_id].susceptibility_modifier = new double [this->number_of_conditions];
    this->health_condition[condition_id].transmission_modifier = new double [this->number_of_conditions];
    this->health_condition[condition_id].symptoms_modifier = new double [this->number_of_conditions];
    this->health_condition[condition_id].is_transmission_modified = false;
    this->health_condition[condition_id].is_susceptibility_modified = false;
    this->health_condition[condition_id].is_symptoms_modified = false;
    for(int j = 0; j < this->number_of_conditions; ++j) {
      this->health_condition[condition_id].transmission_modifier[j] = 1.0;
      this->health_condition[condition_id].susceptibility_modifier[j] = 1.0;
      this->health_condition[condition_id].symptoms_modifier[j] = 1.0;
    }
  }
  this->days_symptomatic = 0;
  this->previous_infection_serotype = -1;

}

Health::~Health() {
  delete[] this->health_condition;
}

void Health::become_susceptible(int condition_id) {
  if(is_susceptible(condition_id)) {
    FRED_VERBOSE(0, 
		 "HEALTH WARNING: %s person %d age %d is already SUSCEPTIBLE for %s\n",
		 Date::get_date_string().c_str(),
		 myself->get_id(), myself->get_age(),
		 Global::Conditions.get_condition_name(condition_id).c_str());
    return;
  }
  set_susceptibility(condition_id, 1.0);
  assert(is_susceptible(condition_id));
  clear_recovered(condition_id);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is SUSCEPTIBLE for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::become_exposed(int condition_id, Person* infector, Mixing_Group* mixing_group, int day) {
  
  FRED_VERBOSE(1, "HEALTH: become_exposed: person %d is exposed to condition %d day %d\n",
	       myself->get_id(), condition_id, day);
  
  if(is_infected(condition_id)) {
    Utils::fred_abort("DOUBLE EXPOSURE: person %d dis_id %d day %d\n", myself->get_id(), condition_id, day);
  }
    
  if(mixing_group == NULL) {
    if (Global::Enable_Health_Records) {
      fprintf(Global::HealthRecordfp,
	      "HEALTH RECORD: %s day %d person %d age %d is an IMPORTED EXPOSURE to %s\n",
	      Date::get_date_string().c_str(),
	      Global::Simulation_Day,
	      myself->get_id(), myself->get_age(),
	      Global::Conditions.get_condition_name(condition_id).c_str());
    }
  }
  else {
    if (Global::Enable_Health_Records) {
      fprintf(Global::HealthRecordfp,
	      "HEALTH RECORD: %s day %d person %d age %d is EXPOSED to %s at %c from person %d\n",
	      Date::get_date_string().c_str(),
	      Global::Simulation_Day,
	      myself->get_id(), myself->get_age(),
	      Global::Conditions.get_condition_name(condition_id).c_str(),
	      mixing_group->get_type(),
	      infector->get_id());
    }
  }

  set_infector(condition_id, infector);
  set_mixing_group(condition_id, mixing_group);
  set_onset_day(condition_id, day);
  set_infection(condition_id);
  become_unsusceptible(condition_id);
  if(this->myself->get_household() != NULL) {
    this->myself->get_household()->set_exposed(condition_id);
    this->myself->set_exposed_household(this->myself->get_household()->get_index());
  } else if(Global::Enable_Hospitals && this->myself->is_hospitalized()) {
    if(this->myself->get_permanent_household() != NULL) {
      this->myself->get_permanent_household()->set_exposed(condition_id);
      this->myself->set_exposed_household(this->myself->get_permanent_household()->get_index());
    }
  }

  // special case: exposed to HIV
  if (Global::Conditions.get_condition(condition_id)->is_hiv()) {
    FRED_VERBOSE(0, "HEALTH: new HIV_infection person %d day %d\n",
		 myself->get_id(), day);
    hiv_infection = new HIV_Infection(condition_id, this->myself);
    hiv_infection->setup();
  }


  if(Global::Enable_Transmission_Network) {
    FRED_VERBOSE(1, "Joining transmission network: %d\n", myself->get_id());
    myself->join_network(Global::Transmission_Network);
  }

  if(strcmp("sexual", Global::Conditions.get_condition(condition_id)->get_transmission_mode()) == 0) {
    Global::Sexual_Partner_Network->add_person_to_sexual_partner_network(myself);
  }

  if(Global::Enable_Vector_Transmission && this->number_of_conditions > 1) {
    // special check for multi-serotype dengue:
    if(this->previous_infection_serotype == -1) {
      // remember this infection's serotype
      this->previous_infection_serotype = condition_id;
      // after the first infection, become immune to other two serotypes.
      for(int sero = 0; sero < Global::Conditions.get_number_of_conditions(); ++sero) {
        // if (sero == previous_infection_serotype) continue;
        if(sero == condition_id) {
          continue;
        }
        FRED_STATUS(1, "DENGUE: person %d now immune to serotype %d\n",
		    myself->get_id(), sero);
        become_unsusceptible(sero);
      }
    } else {
      // after the second infection, become immune to other two serotypes.
      for(int sero = 0; sero < Global::Conditions.get_number_of_conditions(); ++sero) {
        if(sero == this->previous_infection_serotype) {
          continue;
        }
        if(sero == condition_id) {
          continue;
        }
        FRED_STATUS(1, "DENGUE: person %d now immune to serotype %d\n",
		    myself->get_id(), sero);
        become_unsusceptible(sero);
      }
    }
  }
}

void Health::become_unsusceptible(int condition_id) {
  set_susceptibility(condition_id, 0.0);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is UNSUSCEPTIBLE for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::become_infectious(int condition_id) {
  int household_index = this->myself->get_exposed_household_index();
  Household* hh = Global::Places.get_household(household_index);
  assert(hh != NULL);
  hh->set_human_infectious(condition_id);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is INFECTIOUS for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::become_noninfectious(int condition_id) {
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is NONINFECTIOUS for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::become_symptomatic(int condition_id) {
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is SYMPTOMATIC for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::resolve_symptoms(int condition_id) {
  set_symptoms_level(condition_id, Global::NO_SYMPTOMS);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d RESOLVES SYMPTOMS for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}


void Health::recover(int condition_id, int day) {
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is RECOVERED from %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
  clear_infection(condition_id);
  set_recovered(condition_id);
  int household_index = myself->get_exposed_household_index();
  Household* h = Global::Places.get_household(household_index);
  h->set_recovered(condition_id);
  h->reset_human_infectious();
  myself->reset_neighborhood();
  become_removed(condition_id, day);
}

void Health::become_removed(int condition_id, int day) {
  set_symptoms_level(condition_id, Global::NO_SYMPTOMS);
  set_susceptibility(condition_id, 0.0);
  set_infectivity(condition_id, 0.0);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is REMOVED for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}

void Health::become_immune(int condition_id) {
  Global::Conditions.get_condition(condition_id)->become_immune(myself, is_susceptible(condition_id), is_infectious(condition_id), is_symptomatic(condition_id));
  set_immunity(condition_id);
  set_symptoms_level(condition_id, Global::NO_SYMPTOMS);
  set_susceptibility(condition_id, 0.0);
  set_infectivity(condition_id, 0.0);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is IMMUNE for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
}


void Health::become_case_fatality(int condition_id, int day) {
  FRED_VERBOSE(0, "CONDITION %s is FATAL: day %d person %d\n",
	       Global::Conditions.get_condition_name(condition_id).c_str(),
	       day, myself->get_id());
  set_case_fatality(condition_id);
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: %s day %d person %d age %d is CASE_FATALITY for %s\n",
	    Date::get_date_string().c_str(),
	    Global::Simulation_Day,
	    myself->get_id(), myself->get_age(),
	    Global::Conditions.get_condition_name(condition_id).c_str());
  }
  become_removed(condition_id, day);

  // update household counts
  Mixing_Group* hh = myself->get_household();
  if(hh == NULL) {
    if(Global::Enable_Hospitals && myself->is_hospitalized() && myself->get_permanent_household() != NULL) {
      hh = myself->get_permanent_household();
    }
  }
  if(hh != NULL) {
    hh->increment_case_fatalities(day, condition_id);
  }
  
  // queue removal from population
  Global::Pop.prepare_to_die(day, myself);
}

void Health::update_condition(int day, int condition_id) {

  /* the rest applies only to HIV_Infections -- TBA

  if(is_infected(condition_id) == false) {
    return;
  }
  
  FRED_VERBOSE(1, "update_infection %d on day %d person %d\n", condition_id, day, myself->get_id());
  this->infection[condition_id]->update(day);

  // case_fatality?
  if(day > 0 && this->infection[condition_id]->is_fatal(day)) {
    become_case_fatality(condition_id, day);
  }
  
  FRED_VERBOSE(1,"update_infection %d FINISHED on day %d person %d\n",
	       condition_id, day, myself->get_id());

  */

} // end Health::update_infection //


//Modify Operators

double Health::get_transmission_modifier(int condition_id) const {
  double result = 1.0;
  if(1 || this->health_condition[condition_id].is_transmission_modified) {
    for(int i = 0; i < this->number_of_conditions; ++i) {
      result *= this->health_condition[condition_id].transmission_modifier[i];
    }
  }
  return result;
}

double Health::get_susceptibility_modifier(int condition_id) const {
  double result = 1.0;
  if(1 || this->health_condition[condition_id].is_susceptibility_modified) {
    for(int i = 0; i < this->number_of_conditions; ++i) {
      result *= this->health_condition[condition_id].susceptibility_modifier[i];
    }
  }
  return result;
}


double Health::get_symptoms_modifier(int condition_id) const {
  double result = 1.0;
  if(1 || this->health_condition[condition_id].is_symptoms_modified) {
    for(int i = 0; i < this->number_of_conditions; ++i) {
      result *= this->health_condition[condition_id].symptoms_modifier[i];
    }
  }
  return result;
}


void Health::infect(Person* infectee, int condition_id, Mixing_Group* mixing_group, int day) {

  FRED_STATUS(1, "person %d INFECTS person %d mixing_group = %s day = %d\n",
	      myself->get_id(), infectee->get_id(), mixing_group ? mixing_group->get_label() : "NULL", day);

  infectee->become_exposed(condition_id, myself, mixing_group, day);

  this->health_condition[condition_id].number_of_infectees++;
  
  int exp_day = get_onset_day(condition_id);
  assert(0 <= exp_day);
  Condition* condition = Global::Conditions.get_condition(condition_id);
  condition->increment_cohort_infectee_count(exp_day);

  FRED_STATUS(1, "person %d INFECTS person %d infectees = %d\n",
	      myself->get_id(), infectee->get_id(), get_number_of_infectees(condition_id));

  if(Global::Enable_Transmission_Network) {
    FRED_VERBOSE(1, "Creating link in transmission network: %d -> %d\n", myself->get_id(), infectee->get_id());
    myself->create_network_link_to(infectee, Global::Transmission_Network);
  }
}

void Health::update_mixing_group_counts(int day, int condition_id, Mixing_Group* mixing_group) {
  // this is only called for people with an active infection
  assert(is_infected(condition_id));

  // mixing group must exist to update
  if(mixing_group == NULL) {
    return;
  }

  // update infection counters
  if(is_newly_infected(day, condition_id)) {
    mixing_group->increment_new_infections(day, condition_id);
  }
  mixing_group->increment_current_infections(day, condition_id);

  // update symptomatic infection counters
  if(is_symptomatic(condition_id)) {
    if(get_symptoms_start_date(condition_id)==day) {
      mixing_group->increment_new_symptomatic_infections(day, condition_id);
    }
    mixing_group->increment_current_symptomatic_infections(day, condition_id);
  }
}

void Health::terminate(int day) {
  FRED_VERBOSE(1, "TERMINATE HEALTH for person %d day %d\n",
	       this->myself->get_id(), day);

  for(int condition_id = 0; condition_id < Global::Conditions.get_number_of_conditions(); ++condition_id) {
    Global::Conditions.get_condition(condition_id)->terminate_person(myself, day);

    // update our health record
    become_removed(condition_id, day);
  }
  this->alive = false;
}

void Health::set_state(int condition_id, int state, int day) {
  this->health_condition[condition_id].state = state;
  set_last_transition_day(condition_id, day);

  int symp_level = this->health_condition[condition_id].natural_history->get_symptoms_level(state);
  if (symp_level != Global::NO_SYMPTOMS && get_symptoms_start_date(condition_id)==-1) {
    set_symptoms_start_date(condition_id, day);
  }
  set_symptoms_level(condition_id, symp_level);

  double infectivity = health_condition[condition_id].natural_history->get_infectivity(state);
  set_infectivity(condition_id, infectivity);

  double susceptibility = health_condition[condition_id].natural_history->get_susceptibility(state);
  set_susceptibility(condition_id, susceptibility);

  FRED_VERBOSE(1, "set_state person %d state %d symp %d inf %f sus %f is_infected %d is_susc %d is_symp %d is _inf %d\n",
	       myself->get_id(), state, symp_level, infectivity, susceptibility,
	       is_infected(condition_id),
	       is_susceptible(condition_id),
	       is_symptomatic(condition_id),
	       is_infectious(condition_id));

  /*
  for (int i = 0; i < Global::Conditions.get_number_of_conditions(); ++i) {
    int istate = this->health_condition[i].state;
    int mod_state = this->health_condition[condition_id].natural_history->get_state_modifier(state,i,istate);
    if (mod_state != istate) {
      FRED_VERBOSE(0, "MOD_STATE cond %d state %d modifies cond %d state %d to state %d\n",
		   condition_id, state, i, istate, mod_state);
    }
  }
  */

  for (int i = 0; i < Global::Conditions.get_number_of_conditions(); ++i) {
    this->health_condition[i].transmission_modifier[condition_id] = this->health_condition[condition_id].natural_history->get_transmission_modifier(state,i);
    this->health_condition[i].susceptibility_modifier[condition_id] = this->health_condition[condition_id].natural_history->get_susceptibility_modifier(state,i);
  }

}

int Health::get_mixing_group_id(int condtion_id) const {
  return get_mixing_group(condtion_id) ? get_mixing_group(condtion_id)->get_id() : -1;
}

char* Health::get_mixing_group_label(int condtion_id) const {
  return get_mixing_group(condtion_id) ? get_mixing_group(condtion_id)->get_label() : (char*) "X";
}

char Health::get_mixing_group_type(int condition_id) const {
  return get_mixing_group(condition_id) ? get_mixing_group(condition_id)->get_type() : 'X' ;
}
