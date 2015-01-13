/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Epidemic.cc
//

#include "Epidemic.h"
#include "Disease.h"

#include <stdio.h>
#include <new>
#include <iostream>
#include <vector>
using namespace std;
#include "Random.h"
#include "Params.h"
#include "Person.h"
#include "Global.h"
#include "Infection.h"
#include "Place_List.h"
#include "Place.h"
#include "Timestep_Map.h"
#include "Multistrain_Timestep_Map.h"
#include "Transmission.h"
#include "Date.h"
#include "Grid.h"
#include "Household.h"
#include "Utils.h"
#include "Seasonality.h"
#include "Evolution.h"
#include "Workplace.h"
#include "Activities.h"

Epidemic::Epidemic(Disease *dis, Timestep_Map* _primary_cases_map) {
  disease = dis;
  id = disease->get_id();
  primary_cases_map = _primary_cases_map;
  primary_cases_map->print(); 
  new_cases = new int [Global::Days];
  infectees = new int [Global::Days];
  for (int i = 0; i < Global::Days; i++) {
    new_cases[i] = infectees[i] = 0;
  }

  inf_households.reserve( Global::Places.get_number_of_places( HOUSEHOLD ) );
  inf_neighborhoods.reserve( Global::Places.get_number_of_places( NEIGHBORHOOD ) );
  inf_classrooms.reserve( Global::Places.get_number_of_places( CLASSROOM ) );
  inf_schools.reserve( Global::Places.get_number_of_places( SCHOOL ) );
  inf_workplaces.reserve( Global::Places.get_number_of_places( WORKPLACE ) );
  inf_offices.reserve( Global::Places.get_number_of_places( OFFICE ) );
  
  inf_households.clear();
  inf_neighborhoods.clear();
  inf_classrooms.clear();
  inf_schools.clear();
  inf_workplaces.clear();
  inf_offices.clear();
  
  attack_rate = 0.0;
  total_incidents = 0;
  clinical_incidents = 0;
  total_clinical_incidents = 0;
  incident_infections = 0;
  symptomatic_count = 0;
  exposed_count = removed_count = immune_count = 0;
  daily_infections_list.clear();

  tot_infected_employees_small = 0;
  tot_infected_employees_med = 0;
  tot_infected_employees_large = 0;
  tot_infected_employees_xl = 0;

  tot_infections_at_work = 0;
  tot_infections_at_work_small = 0;
  tot_infections_at_work_med = 0;
  tot_infections_at_work_large = 0;
  tot_infections_at_work_xl = 0;
  tot_pres = 0;
  tot_pres_small = 0;
  tot_pres_med = 0;
  tot_pres_large = 0;
  tot_pres_xl = 0;
  tot_pres_wo_sl = 0;
  tot_pres_small_wo_sl = 0;
  tot_pres_med_wo_sl = 0;
  tot_pres_large_wo_sl = 0;
  tot_pres_xl_wo_sl = 0; 

  tot_infectee_sl = 0; 
  tot_infectee_wo_sl = 0;
  tot_infectee_sl_inf_at_work = 0;
  tot_infectee_sl_inf_other = 0;
  tot_infectee_wo_sl_inf_at_work = 0;
  tot_infectee_wo_sl_inf_other = 0;

  place_person_list_reserve_size = 1;
}

Epidemic::~Epidemic() {
  delete primary_cases_map;
}

void Epidemic::become_susceptible(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  Global::Pop.set_mask_by_index( fred::Susceptible, person->get_pop_index() );
}

void Epidemic::become_unsusceptible(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  if ( Global::Pop.check_mask_by_index( fred::Susceptible, person->get_pop_index() ) ) {
    Global::Pop.clear_mask_by_index( fred::Susceptible, person->get_pop_index() );
  }
  else {
    FRED_VERBOSE( 0, "WARNING: become_unsusceptible: person %d not removed from \
        susceptible_list\n", person->get_id() );
  }
}

void Epidemic::become_exposed(Person *person) {
  #pragma omp atomic
  ++exposed_count;
  #pragma omp atomic
  ++incident_infections;
  // TODO the daily infections list may end up containing defunct pointers if
  // enable_deaths is in effect (whether or not we are running in parallel mode).
  // Make daily reports and purge list after each report to fix this.
  fred::Scoped_Lock lock( mutex );
  daily_infections_list.push_back(person);
}

void Epidemic::become_infectious(Person *person) {
#pragma omp atomic
  exposed_count--;
  // operations on bloque (underlying container for population) are thread-safe
  Global::Pop.set_mask_by_index( fred::Infectious, person->get_pop_index() );
}

void Epidemic::become_uninfectious(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  if ( Global::Pop.check_mask_by_index( fred::Infectious, person->get_pop_index() ) ) {
    Global::Pop.clear_mask_by_index( fred::Infectious, person->get_pop_index() );
  }
  else {
    FRED_VERBOSE( 0, "WARNING: become_uninfectious: person %d not removed from \
        infectious_list\n",person->get_id() );
  }
}

void Epidemic::become_symptomatic(Person *person) {
#pragma omp atomic
  ++symptomatic_count;
#pragma omp atomic
  ++clinical_incidents;
}

void Epidemic::become_removed(Person *person, bool susceptible, bool infectious, bool symptomatic) {
  // operations on bloque (underlying container for population) are thread-safe
  if (susceptible) {
    if ( Global::Pop.check_mask_by_index( fred::Susceptible, person->get_pop_index() ) ) {
      Global::Pop.clear_mask_by_index( fred::Susceptible, person->get_pop_index() );
      FRED_VERBOSE( 1, "OK: become_removed: person %d removed from \
          susceptible_list\n",person->get_id() );
    }
    else {
      FRED_VERBOSE( 0, "WARNING: become_removed: person %d not removed \
          from susceptible_list\n",person->get_id() );
    }
  }
  if (infectious) {
    if ( Global::Pop.check_mask_by_index( fred::Infectious, person->get_pop_index() ) ) {
      Global::Pop.clear_mask_by_index( fred::Infectious, person->get_pop_index() );
      FRED_VERBOSE( 1, "OK: become_removed: person %d removed from \
          infectious_list\n",person->get_id() );
    }
    else {
      FRED_VERBOSE( 0, "WARNING: become_removed: person %d not removed from \
          infectious_list\n",person->get_id() );
    }
  }
  if (symptomatic) {
#pragma omp atomic
    --( symptomatic_count );
  }
#pragma omp atomic
  ++( removed_count );
}

void Epidemic::become_immune(Person *person, bool susceptible, bool infectious, bool symptomatic) {
  if (susceptible) {
    if ( Global::Pop.check_mask_by_index( fred::Susceptible, person->get_pop_index() ) ) {
      Global::Pop.clear_mask_by_index( fred::Susceptible, person->get_pop_index() );
      FRED_VERBOSE( 1, "OK: become_immune: person %d removed from \
          susceptible_list\n",person->get_id() );
    }
    else {
      FRED_VERBOSE( 0, "WARNING: become_immune: person %d not removed from \
          susceptible_list\n",person->get_id() );
    }
  }
  if (infectious) {
    if ( Global::Pop.check_mask_by_index( fred::Infectious, person->get_pop_index() ) ) {
      Global::Pop.clear_mask_by_index( fred::Infectious, person->get_pop_index() );
      FRED_VERBOSE( 1, "OK: become_immune: person %d removed from \
          infectious_list\n", person->get_id() );
    }
    else {
      FRED_VERBOSE( 0, "WARNING: become_immune: person %d not removed from \
          infectious_list\n", person->get_id() );
    }
  }

  if (symptomatic) {
#pragma omp atomic
    --symptomatic_count;
  }
#pragma omp atomic
  ++removed_count;
}

void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic update stats\n","");

  if (day == 0) {
    N_init = N = disease->get_population()->get_pop_size();
  }
  else {
    N = disease->get_population()->get_pop_size();
  }

  new_cases[day] = incident_infections;
  total_incidents += incident_infections;
  total_clinical_incidents += clinical_incidents;
  attack_rate = (100.0*total_incidents)/N_init;
  clinical_attack_rate = (100.0*total_clinical_incidents)/N_init;

  // get reproductive rate for the cohort exposed RR_delay days ago
  // unless RR_delay == 0
  RR = 0.0;         // reproductive rate for a fixed cohort of infectors
  cohort_size = 0;  // size of the cohort exposed on cohort_day
  if (0 < Global::RR_delay && Global::RR_delay <= day) {
    int cohort_day = day - Global::RR_delay;    // exposure day for cohort
    cohort_size = new_cases[cohort_day];        // size of cohort
    if (cohort_size > 0) {
      // compute reproductive rate for this cohort
      RR = (double)infectees[cohort_day] /(double)cohort_size;
    }
  }

  int susceptible_count = Global::Pop.size( fred::Susceptible ); 
  int infectious_count = Global::Pop.size( fred::Infectious );

  double average_seasonality_multiplier = 1.0;
  if (Global::Enable_Seasonality) {
    average_seasonality_multiplier = Global::Clim->get_average_seasonality_multiplier(disease->get_id());
  }

  char buffer[ 256 ];
  int nchar_used = sprintf(buffer,
			   "Day %3d Date %s Wkday %s Year %d Week %2d Str %d S %7d E %7d I %7d I_s %7d R %7d M %7d C %7d CI %7d AR %5.2f CAR %5.2f RR %4.2f N %7d",
			   day,
			   Global::Sim_Current_Date->get_YYYYMMDD().c_str(),
			   Global::Sim_Current_Date->get_day_of_week_string().c_str(), 
			   Global::Sim_Current_Date->get_epi_week_year(), 
			   Global::Sim_Current_Date->get_epi_week(),
			   id, susceptible_count, exposed_count, infectious_count,
			   symptomatic_count, removed_count, immune_count,
			   incident_infections, clinical_incidents, attack_rate, 
			   clinical_attack_rate, RR, N);

  fprintf( Global::Outfp, "%s", buffer );
  FRED_STATUS(0, "%s", buffer);

  if (Global::Enable_Seasonality) {
    fprintf(Global::Outfp, " SM %2.4f", average_seasonality_multiplier);
    FRED_STATUS(0, " SM %2.4f", average_seasonality_multiplier);
  } 

  if (Global::Track_Multi_Strain_Stats) {
    disease->get_evolution()->print_stats(day);
  }

  // Print Residual Immunuties
  if (Global::Track_Residual_Immunity) {
    Evolution *evol = disease->get_evolution();
    for(int i=0; i<disease->get_num_strains(); i++){
      double res_immunity = 0.0;
      Population *pop = disease->get_population();
      for(int j=0; j<pop->get_pop_size(); j++){
        res_immunity = evol->residual_immunity(pop->get_person_by_index(j), i, day);
      }
      res_immunity /= pop->get_pop_size();
      if(res_immunity != 0) {
        //fprintf(Global::Outfp, " ResM_%d %lf", i, res_immunity);
        fprintf(Global::Outfp, " S_%d %lf", i, 1.0-res_immunity);
        //FRED_STATUS(0, " ResM_%d %lf", i, res_immunity);
        FRED_STATUS(0, " S_%d %lf", i, 1.0-res_immunity);
      }
    }
  }

  fprintf(Global::Outfp, "\n");  
  fflush(Global::Outfp);

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "\n");
    fflush(Global::Statusfp);

    if (Global::Report_Presenteeism) { report_presenteeism(day); }
    if (Global::Report_Place_Of_Infection) { report_place_of_infection(day); }
    if (Global::Report_Age_Of_Infection) { report_age_of_infection(day); }

    // prepare for next day
    incident_infections = clinical_incidents = 0;
    daily_infections_list.clear();
  }
}

void::Epidemic::report_age_of_infection(int day) {
  int age_count[21];				// age group counts
  for (int i = 0; i < 21; i++) age_count[i] = 0;
  for (int i = 0; i < incident_infections; i++) {
    Person * infectee = daily_infections_list[i];
    int age_group = infectee->get_age() / 5;
    if (age_group > 20) age_group = 20;
    age_count[age_group]++;
  }
  Utils::fred_log("\nDay %d INF_AGE: ", day);
  for (int i = 0; i <= 20; i++) {
    Utils::fred_report(" A%d %d", i*5, age_count[i]);
  }
  Utils::fred_log("\n");
}

void::Epidemic::report_place_of_infection(int day) {
  // type of place of infection
  int X = 0;
  int H = 0;
  int N = 0;
  int S = 0;
  int C = 0;
  int W = 0;
  int O = 0;
  for (int i = 0; i < incident_infections; i++) {
    Person * infectee = daily_infections_list[i];
    char c = infectee->get_infected_place_type(id);
    switch(c) {
      case 'X': X++; break;
      case 'H': H++; break;
      case 'N': N++; break;
      case 'S': S++; break;
      case 'C': C++; break;
      case 'W': W++; break;
      case 'O': O++; break;
    }
  }

  Utils::fred_log("\nDay %d INF_PLACE: ", day);
  Utils::fred_report(" X %d H %d Nbr %d Sch %d", X, H, N, S);
  Utils::fred_report(" Cls %d Wrk %d Off %d ", C, W, O);
  Utils::fred_log("\n");
}

void Epidemic::report_presenteeism(int day) {
  // daily totals
  int infections_in_pop = 0;
  int presenteeism_small = 0;
  int presenteeism_med = 0;
  int presenteeism_large = 0;
  int presenteeism_xlarge = 0;
  int presenteeism_small_with_sl = 0;
  int presenteeism_med_with_sl = 0;
  int presenteeism_large_with_sl = 0;
  int presenteeism_xlarge_with_sl = 0;
  int infections_at_work = 0;
  int infections_at_work_sl = 0;
  int tot_emp_sl = Activities::employees_small_with_sick_leave + Activities::employees_med_with_sick_leave + Activities::employees_large_with_sick_leave + Activities::employees_xlarge_with_sick_leave;

  // company size limits
  static int small;
  static int medium;
  static int large;
  if (day == 0) {
    small = Workplace::get_small_workplace_size();
    medium = Workplace::get_medium_workplace_size();
    large = Workplace::get_large_workplace_size();
  }

  for (int i = 0; i < incident_infections; i++) {
    Person * infectee = daily_infections_list[i];
    char c = infectee->get_infected_place_type(id);
    infections_in_pop++;

    Place * infectees_workplace = infectee->get_workplace();
    if(infectees_workplace != NULL) {
      int inf_wp_size =  infectees_workplace->get_size();

      if (inf_wp_size < small) {                 // small workplace
        tot_infected_employees_small++;
      } else if (inf_wp_size < medium) {         // medium workplace
        tot_infected_employees_med++;
      } else if (inf_wp_size < large) {          // large workplace
        tot_infected_employees_large++;
      } else {                                    // xlarge workplace
        tot_infected_employees_xl++;
      }
      
      //Increment the totals for infectee being infected either with or without sl
      if(infectee->is_sick_leave_available()) {
        tot_infectee_sl++;
      } else {
        tot_infectee_wo_sl++;
      }

      if (c != 'W' && c != 'O') {
        //Infectee was infected elsewhere
        if(infectee->is_sick_leave_available()) {
          tot_infectee_sl_inf_other++;
        } else {
          tot_infectee_wo_sl_inf_other++;
        }
      } else {
        //Infectee WAS infected at work, so increment these counts
        if(infectee->is_sick_leave_available()) {
          tot_infectee_sl_inf_at_work++;
        } else {
          tot_infectee_wo_sl_inf_at_work++;
        }
      }
    }

    // presenteeism requires that place of infection is work or office
    if (c != 'W' && c != 'O') {
      continue;
    }

    infections_at_work++;

    // get the work place size (note we don't care about the office size)
    Place * work = infectee->get_workplace();
    assert(work != NULL);
    int size = work->get_size();
	  
    if (size < small) {			// small workplace
      tot_infections_at_work_small++;
    } else if (size < medium) {		// medium workplace
      tot_infections_at_work_med++;
    } else if (size < large) {		// large workplace
      tot_infections_at_work_large++;
    } else {					// xlarge workplace
      tot_infections_at_work_xl++;
    }
	  
    // presenteeism requires that the infector have symptoms
    Person * infector = infectee->get_infector(this->id);
    assert(infector != NULL);
	  
    // determine whether sick leave was available to infector
    bool infector_has_sick_leave = infector->is_sick_leave_available();
    if(infector_has_sick_leave)
	  infections_at_work_sl++;
	  
    if (infector->is_symptomatic()) {

      if (size < small) {			// small workplace
        presenteeism_small++;
        if (infector_has_sick_leave)
          presenteeism_small_with_sl++;
      } else if (size < medium) {		// medium workplace
        presenteeism_med++;
        if (infector_has_sick_leave)
          presenteeism_med_with_sl++;
      } else if (size < large) {		// large workplace
        presenteeism_large++;
        if (infector_has_sick_leave)
          presenteeism_large_with_sl++;
      } else {					// xlarge workplace
        presenteeism_xlarge++;
        if (infector_has_sick_leave)
          presenteeism_xlarge_with_sl++;
      }
    }
  } // end loop over infectees

  // raw counts
  int presenteeism = presenteeism_small + presenteeism_med
    + presenteeism_large + presenteeism_xlarge;
  int presenteeism_with_sl = presenteeism_small_with_sl + presenteeism_med_with_sl
    + presenteeism_large_with_sl + presenteeism_xlarge_with_sl;
	
  //For Odds Ratio Calculation
  int daily_presenteeism_wo_sl = presenteeism - presenteeism_with_sl;
  int daily_asymptomatic_cases = infections_at_work - presenteeism;
  int daily_asymptomatic_cases_sl = infections_at_work_sl - presenteeism_with_sl;
  int daily_asympotmatic_cases_wo_sl = daily_asymptomatic_cases - daily_asymptomatic_cases_sl;

  presenteeism_wo_sl += daily_presenteeism_wo_sl; //A
  asympotmatic_cases_wo_sl += daily_asympotmatic_cases_wo_sl; //B
  presenteeism_sl += presenteeism_with_sl; //C
  asymptomatic_cases_sl += daily_asymptomatic_cases_sl; //D

  tot_infections_at_work += infections_at_work;

  tot_pres += presenteeism;
  tot_pres_small += presenteeism_small;
  tot_pres_med += presenteeism_med;
  tot_pres_large += presenteeism_large;
  tot_pres_xl += presenteeism_xlarge;
 
  tot_pres_wo_sl += daily_presenteeism_wo_sl; 
  tot_pres_small_wo_sl += presenteeism_small - presenteeism_small_with_sl;
  tot_pres_med_wo_sl += presenteeism_med - presenteeism_med_with_sl;
  tot_pres_large_wo_sl += presenteeism_large - presenteeism_large_with_sl;
  tot_pres_xl_wo_sl += presenteeism_xlarge - presenteeism_xlarge_with_sl;

  Utils::fred_log("\nDay %d PRESENTEE: ",day);
  Utils::fred_report(" small %d ", presenteeism_small);
  Utils::fred_report("small_n %d ", Workplace::get_workers_in_small_workplaces());
  Utils::fred_report("small_pct %9.2f ", 100.0 * (double)presenteeism_small / (double)Workplace::get_workers_in_small_workplaces());
  Utils::fred_report("med %d ", presenteeism_med);
  Utils::fred_report("med_n %d ", Workplace::get_workers_in_medium_workplaces());
  Utils::fred_report("med_pct %9.2f ", 100.0 * (double)presenteeism_med / (double)Workplace::get_workers_in_medium_workplaces());
  Utils::fred_report("large %d ", presenteeism_large);
  Utils::fred_report("large_n %d ", Workplace::get_workers_in_large_workplaces());
  Utils::fred_report("large_pct %9.2f ", 100.0 * (double)presenteeism_large / (double)Workplace::get_workers_in_large_workplaces());
  Utils::fred_report("xlarge %d ", presenteeism_xlarge);
  Utils::fred_report("xlarge_n %d ", Workplace::get_workers_in_xlarge_workplaces());
  Utils::fred_report("xlarge_pct %9.2f ", 100.0 * (double)presenteeism_xlarge / (double)Workplace::get_workers_in_xlarge_workplaces());
  Utils::fred_report("pres %d ", presenteeism);
  Utils::fred_report("pres_sl %d ", presenteeism_with_sl);
  Utils::fred_report("pres_wo_sl %d ", daily_presenteeism_wo_sl);
  Utils::fred_report("asymp_cases_sl %d ", daily_asymptomatic_cases_sl);
  Utils::fred_report("asymp_cases_wo_sl %d ", daily_asympotmatic_cases_wo_sl);
  Utils::fred_report("inf_at_work %d ", infections_at_work);
  Utils::fred_report("inf_at_work_sl %d ", infections_at_work_sl);
  Utils::fred_report("tot_emp %d ", Workplace::get_total_workers());
  Utils::fred_report("inf_at_work_pct %9.2f ", 100.0 * (double)tot_infections_at_work / (double)Workplace::get_total_workers());
  Utils::fred_report("pres_pct %9.2f ", 100.0 * (double)(presenteeism_wo_sl + presenteeism_sl)  / (double)Workplace::get_total_workers());
	
  Utils::fred_report("tot_inf_at_work %d ", tot_infections_at_work);
	
  Utils::fred_report("tot_inf_at_work_small_pct %9.2f ", 100.0 * (double)tot_infections_at_work_small / (double)Workplace::get_workers_in_small_workplaces());
  Utils::fred_report("tot_inf_at_work_med_pct %9.2f ", 100.0 * (double)tot_infections_at_work_med / (double)Workplace::get_workers_in_medium_workplaces());
  Utils::fred_report("tot_inf_at_work_large_pct %9.2f ", 100.0 * (double)tot_infections_at_work_large / (double)Workplace::get_workers_in_large_workplaces());
  Utils::fred_report("tot_inf_at_work_xl_pct %9.2f ", 100.0 * (double)tot_infections_at_work_xl / (double)Workplace::get_workers_in_xlarge_workplaces());

  Utils::fred_report("tot_pres_small_pct %9.2f ", 100.0 * (double)tot_pres_small / (double)Workplace::get_workers_in_small_workplaces());
  Utils::fred_report("tot_pres_med_pct %9.2f ", 100.0 * (double)tot_pres_med / (double)Workplace::get_workers_in_medium_workplaces());
  Utils::fred_report("tot_pres_large_pct %9.2f ", 100.0 * (double)tot_pres_large / (double)Workplace::get_workers_in_large_workplaces());
  Utils::fred_report("tot_pres_xl_pct %9.2f ", 100.0 * (double)tot_pres_xl / (double)Workplace::get_workers_in_xlarge_workplaces());

  Utils::fred_report("tot_pres %d ", tot_pres);
  Utils::fred_report("tot_pres_small %d ", tot_pres_small);
  Utils::fred_report("tot_pres_med %d ", tot_pres_med);
  Utils::fred_report("tot_pres_large %d ", tot_pres_large);
  Utils::fred_report("tot_pres_xl %d ", tot_pres_xl);

  Utils::fred_report("tot_infected_employees_small %d ", tot_infected_employees_small);
  Utils::fred_report("tot_infected_employees_med %d ", tot_infected_employees_med);
  Utils::fred_report("tot_infected_employees_large %d ", tot_infected_employees_large);
  Utils::fred_report("tot_infected_employees_xl %d ", tot_infected_employees_xl);

  Utils::fred_report("tot_pres_wo_sl %d ", tot_pres_wo_sl);
  Utils::fred_report("tot_pres_small_wo_sl %d ", tot_pres_small_wo_sl);
  Utils::fred_report("tot_pres_med_wo_sl %d ", tot_pres_med_wo_sl);
  Utils::fred_report("tot_pres_large_wo_sl %d ", tot_pres_large_wo_sl);
  Utils::fred_report("tot_pres_xl_wo_sl %d ", tot_pres_xl_wo_sl);

  Utils::fred_report("tot_pres_wo_sl_per_tot_pres %9.2f ", (double)tot_pres_wo_sl / (double)tot_pres);
  Utils::fred_report("tot_pres_small_wo_sl_per_tot_pres_small %9.2f ", (double)tot_pres_small_wo_sl / (double)tot_pres_small);
  Utils::fred_report("tot_pres_med_wo_sl_per_tot_pres_med %9.2f ", (double)tot_pres_med_wo_sl / (double)tot_pres_med);
  Utils::fred_report("tot_pres_large_wo_sl_per_tot_pres_large %9.2f ", (double)tot_pres_large_wo_sl / (double)tot_pres_large);
  Utils::fred_report("tot_pres_xl_wo_sl_per_tot_pres_xl %9.2f ", (double)tot_pres_xl_wo_sl / (double)tot_pres_xl);

  Utils::fred_report("tot_pres_small_wo_sl_pct %9.2f ", 100.0 * (double)tot_pres_small_wo_sl / (double)Workplace::get_workers_in_small_workplaces());
  Utils::fred_report("tot_pres_med_wo_sl_pct %9.2f ", 100.0 * (double)tot_pres_med_wo_sl / (double)Workplace::get_workers_in_medium_workplaces());
  Utils::fred_report("tot_pres_large_wo_sl_pct %9.2f ", 100.0 * (double)tot_pres_large_wo_sl / (double)Workplace::get_workers_in_large_workplaces());
  Utils::fred_report("tot_pres_xl_wo_sl_pct %9.2f ", 100.0 * (double)tot_pres_xl_wo_sl / (double)Workplace::get_workers_in_xlarge_workplaces());
	
  Utils::fred_report("tot_pres_per_tot_inf_sm_pct %9.2f ", 100.0 * (double)tot_pres_small / (double)tot_infections_at_work_small);
  Utils::fred_report("tot_pres_per_tot_inf_med_pct %9.2f ", 100.0 * (double)tot_pres_med / (double)tot_infections_at_work_med);
  Utils::fred_report("tot_pres_per_tot_inf_large_pct %9.2f ", 100.0 * (double)tot_pres_large / (double)tot_infections_at_work_large);
  Utils::fred_report("tot_pres_per_tot_inf_xl_pct %9.2f ", 100.0 * (double)tot_pres_xl / (double)tot_infections_at_work_xl);
  Utils::fred_report("tot_pres_per_tot_inf_pct %9.2f ", 100.0 * (double)tot_pres / (double)tot_infections_at_work);
	
  Utils::fred_report("odds_ratio %9.2f ", ((double)presenteeism_wo_sl * (double)asymptomatic_cases_sl)
					   / ((double)asympotmatic_cases_wo_sl * (double)presenteeism_sl));
  Utils::fred_report("tot_infectee_sl %d ", tot_infectee_sl);	
  Utils::fred_report("tot_infectee_wo_sl %d ", tot_infectee_wo_sl);
  Utils::fred_report("tot_infectee_sl_inf_at_work %d ", tot_infectee_sl_inf_at_work);  
  Utils::fred_report("tot_infectee_sl_inf_other %d ", tot_infectee_sl_inf_other);
  Utils::fred_report("tot_infectee_wo_sl_inf_at_work %d ", tot_infectee_wo_sl_inf_at_work);
  Utils::fred_report("tot_infectee_wo_sl_inf_other %d ", tot_infectee_wo_sl_inf_other);
  Utils::fred_report("tot_emp_sl %d ", tot_emp_sl);
  Utils::fred_report("tot_infectee_sl_inf_at_work_per_tot_emp_sl_pct %9.2f ",  100.0 * (double)tot_infectee_sl_inf_at_work / (double)tot_emp_sl);
  Utils::fred_report("tot_infectee_sl_inf_other_per_tot_emp_sl_pct %9.2f ",  100.0 * (double)tot_infectee_sl_inf_other / (double)tot_emp_sl);
  Utils::fred_log("N %d\n", N);
}



void Epidemic::add_infectious_place( Place *place, char type ) {
  switch (type) {
    case HOUSEHOLD:
      {
        fred::Scoped_Lock lock( household_mutex );
        inf_households.push_back(place);
      }
      break;

    case NEIGHBORHOOD:
      {
        fred::Scoped_Lock lock( neighborhood_mutex );
        inf_neighborhoods.push_back(place);
      }
      break;

    case CLASSROOM:
      {
        fred::Scoped_Lock lock( classroom_mutex );
        inf_classrooms.push_back(place);
      }
      break;

    case SCHOOL:
      {
        fred::Scoped_Lock lock( school_mutex );
        inf_schools.push_back(place);
      }
      break;

    case WORKPLACE:
      {
        fred::Scoped_Lock lock( workplace_mutex );
        inf_workplaces.push_back(place);
      }
      break;

    case OFFICE:
      {
        fred::Scoped_Lock lock( office_mutex );
        inf_offices.push_back(place);
      }
      break;
  }
}

/* NOT CURRENTLY USED...
 * 
void Epidemic::get_infectious_places(int day) {
  vector<Person *>::iterator itr;
  vector<Place *>::iterator it;

  int places = Global::Places.get_number_of_places();
  for (int p = 0; p < places; p++) {
    Place * place = Global::Places.get_place_at_position(p);
    if (place->is_open(day) && place->should_be_open(day, id)) {
      switch (place->get_type()) {
        case HOUSEHOLD:
          inf_households.push_back(place);
          break;

        case NEIGHBORHOOD:
          inf_neighborhoods.push_back(place);
          break;

        case CLASSROOM:
          inf_classrooms.push_back(place);
          break;

        case SCHOOL:
          inf_schools.push_back(place);
          break;

        case WORKPLACE:
          inf_workplaces.push_back(place);
          break;

        case OFFICE:
          inf_offices.push_back(place);
          break;
      }
    }
  }
}
*/

void Epidemic::get_primary_infections(int day){
  Population *pop = disease->get_population();
  N = pop->get_pop_size();

  Multistrain_Timestep_Map * ms_map = ( ( Multistrain_Timestep_Map * ) primary_cases_map );

  for (Multistrain_Timestep_Map::iterator ms_map_itr = ms_map->begin(); ms_map_itr != ms_map->end(); ms_map_itr++) {

    Multistrain_Timestep_Map::Multistrain_Timestep * mst = *ms_map_itr;

    if ( mst->is_applicable( day, Global::Epidemic_offset ) ) {

      int extra_attempts = 1000;
      int successes = 0;

      vector < Person * > people;

      if ( mst->has_location() ) {
        vector < Place * > households = Global::Cells->get_households_by_distance(mst->get_lat(), mst->get_lon(), mst->get_radius());
        for (vector < Place * >::iterator hi = households.begin(); hi != households.end(); hi++) {
          vector <Person *> hs = ((Household *) (*hi))->get_inhabitants();  
          // This cast is ugly and should be fixed.  
          // Problem is that Place::update (which clears susceptible list for the place) is called before Epidemic::update.
          // If households were stored as Household in the Cell (rather than the base Place class) this wouldn't be needed.
          people.insert(people.end(), hs.begin(), hs.end());
        }
      }     

      for ( int i = 0; i < mst->get_num_seeding_attempts(); i++ ) {  
        int r, n;
        Person * person;

        // each seeding attempt is independent
        if ( mst->get_seeding_prob() < 1 ) { 
          if ( RANDOM() > mst->get_seeding_prob() ) { continue; }
        }
        // if a location is specified in the timestep map select from that area, otherwise pick a person from the entire population
        if ( mst->has_location() ) {
          r = IRAND( 0, people.size()-1 );
          person = people[r];
        } else if (Global::Seed_by_age) {
          person = pop->select_random_person_by_age(Global::Seed_age_lower_bound, Global::Seed_age_upper_bound);
        } else {
          n = IRAND(0, N-1);
          person = pop->get_person_by_index(n);
        }

        if (person == NULL) { // nobody home
          Utils::fred_warning("Person selected for seeding in Epidemic update is NULL.\n");
          continue;
        }

        if (person->get_health()->is_susceptible(id)) {
          Transmission *transmission = new Transmission(NULL, NULL, day);
          transmission->set_initial_loads(disease->get_primary_loads(day));
          person->become_exposed(this->disease, transmission);
          successes++;
        }

        if (successes < mst->get_min_successes() && i == (mst->get_num_seeding_attempts() - 1) && extra_attempts > 0 ) {
          extra_attempts--;
          i--;
        }
      }

      if (successes < mst->get_min_successes()) {
        Utils::fred_warning(
            "A minimum of %d successes was specified, but only %d successful transmissions occurred.",
            mst->get_min_successes(),successes);
      }
    }
  }

}

void Epidemic::transmit(int day){
  Population *pop = disease->get_population();

  // import infections from unknown sources
  get_primary_infections(day);

  int infectious_places;
  infectious_places =  (int) inf_households.size();
  infectious_places += (int) inf_neighborhoods.size();
  infectious_places += (int) inf_schools.size();
  infectious_places += (int) inf_classrooms.size();
  infectious_places += (int) inf_workplaces.size();
  infectious_places += (int) inf_offices.size();

  FRED_STATUS(0, "Number of infectious places        => %9d\n", infectious_places );
  FRED_STATUS(0, "Number of infectious households    => %9d\n", (int) inf_households.size());
  FRED_STATUS(0, "Number of infectious neighborhoods => %9d\n", (int) inf_neighborhoods.size());
  FRED_STATUS(0, "Number of infectious schools       => %9d\n", (int) inf_schools.size());
  FRED_STATUS(0, "Number of infectious classrooms    => %9d\n", (int) inf_classrooms.size());
  FRED_STATUS(0, "Number of infectious workplaces    => %9d\n", (int) inf_workplaces.size());
  FRED_STATUS(0, "Number of infectious offices       => %9d\n", (int) inf_offices.size());
  
  #pragma omp parallel
  {
    // schools (and classrooms)
    #pragma omp for schedule(dynamic,10)
    for ( int i = 0; i < inf_schools.size(); ++i ) {
      inf_schools[ i ]->spread_infection( day, id );
    }

    #pragma omp for schedule(dynamic,10)
    for ( int i = 0; i < inf_classrooms.size(); ++i ) {
      inf_classrooms[ i ]->spread_infection( day, id );
    }

    // workplaces (and offices)
    #pragma omp for schedule(dynamic,10)
    for ( int i = 0; i < inf_workplaces.size(); ++i ) {
      inf_workplaces[ i ]->spread_infection( day, id );
    }
    #pragma omp for schedule(dynamic,10)
    for ( int i = 0; i < inf_offices.size(); ++i ) {
      inf_offices[ i ]->spread_infection( day, id );
    }

    // neighborhoods (and households)
    #pragma omp for schedule(dynamic,100)
    for ( int i = 0; i < inf_neighborhoods.size(); ++i ) {
      inf_neighborhoods[ i ]->spread_infection( day, id );
    }
    #pragma omp for schedule(dynamic,100)
    for ( int i = 0; i < inf_households.size(); ++i ) {
      inf_households[ i ]->spread_infection( day, id );
    }
  }

  inf_households.clear();
  inf_neighborhoods.clear();
  inf_classrooms.clear();
  inf_schools.clear();
  inf_workplaces.clear();
  inf_offices.clear();
}



void Epidemic::update(int day) {
  Activities::update(day);
  for (int d = 0; d < Global::Diseases; d++) {
    Disease * disease = Global::Pop.get_disease(d);
    Epidemic * epidemic = disease->get_epidemic();
    epidemic->find_infectious_places(day, d);
    epidemic->add_susceptibles_to_infectious_places(day, d);
    epidemic->transmit(day);
  }
}

void Epidemic::update_infectious_activities::operator() ( Person & person ) {
  person.get_activities()->update_infectious_activities( day, disease_id );
}

void Epidemic::find_infectious_places( int day, int disease_id ) {
  FRED_STATUS(1, "find_infectious_places entered\n", "");

  update_infectious_activities update_functor( day, disease_id );
  Global::Pop.apply( fred::Infectious, update_functor );

  FRED_STATUS(1, "find_infectious_places finished\n", "");
}

void Epidemic::update_susceptible_activities::operator() ( Person & person ) {
  person.get_activities()->update_susceptible_activities( day, disease_id );
}

void Epidemic::add_susceptibles_to_infectious_places(int day, int disease_id) {
  FRED_STATUS(1, "add_susceptibles_to_infectious_places entered\n");

  update_susceptible_activities update_functor( day, disease_id );
  Global::Pop.parallel_apply( fred::Susceptible, update_functor );

  FRED_STATUS(1, "add_susceptibles_to_infectious_places finished\n");
}

void Epidemic::infectious_sampler::operator() ( Person & person ) {
  if ( RANDOM() < prob ) {
    #pragma omp critical(EPIDEMIC_INFECTIOUS_SAMPLER)
    samples->push_back( &person );
  }
}

void Epidemic::get_infectious_samples(vector<Person *> &samples, double prob = 1.0) {
  if(prob > 1.0) prob = 1.0;
  if(prob <= 0.0) return;
  samples.clear();
  infectious_sampler sampler;
  sampler.samples = &samples;
  sampler.prob = prob;
  Global::Pop.parallel_apply( fred::Infectious, sampler );
}

int Epidemic::get_num_infectious() {
  return Global::Pop.size( fred::Infectious );
}
