/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


#include "Evolution.h"
#include "Infection.h"
#include "Transmission.h"
#include "Trajectory.h"
#include "Infection.h"
#include "Global.h"
#include "ODEIntraHost.h"
#include "Antiviral.h"
#include "Health.h"
#include "AV_Health.h"
#include "Person.h"

#include <map>

using namespace std;

void Evolution :: setup(Disease *disease) {
  this->disease = disease;
}

Infection *Evolution::transmit(Infection *infection, Transmission & transmission, Person *infectee) {
  if(infection == NULL){
    infection = new Infection(disease, transmission.get_infector(), infectee,
           transmission.get_infected_place(), transmission.get_exposure_date());
    Trajectory * trajectory = disease->get_trajectory( infection, transmission.get_initial_loads() );
    infection->setTrajectory(trajectory);
    infection->set_susceptibility_period(0);
    infection->report_infection(transmission.get_exposure_date());
  }
  else{
    // Fail repeated infections by default??
    cout << "REPEATED INFECTION" << endl;
    return NULL;
  }
  return infection;
}

inline double Evolution::residual_immunity(Person *person, int challenge_strain, int day) {
  return double( !( person->get_health()->is_susceptible( disease->get_id() ) ) );
}

void Evolution::avEffect(Antiviral *av, Health *health, int disease, int cur_day, AV_Health *av_health) {
  // If this is the first day of AV Course
  if(cur_day == av_health->get_av_start_day()) {
    av->modify_susceptiblilty(health, disease);

    // If you are already exposed, we need to modify your infection
    if((health->get_exposure_date(disease) > -1) && (cur_day > health->get_exposure_date(disease))) {
      if(Global::Debug > 3) cout << "reducing an already exposed person\n";

        av->modify_infectivity(health, disease);
        //av->modify_symptomaticity(health, disease, cur_day);
    }
  }

  // If today is the day you got exposed, prophilaxis
  if(cur_day == health->get_exposure_date(disease)) {
    if(Global::Debug > 3) cout << "reducing agent on the day they are exposed\n";

      av->modify_infectivity(health, disease);
      av->modify_symptomaticity(health, disease, cur_day);
  }

  // If this is the last day of the course
  if(cur_day == av_health->get_av_end_day()) {
    if(Global::Debug > 3) cout << "resetting agent to original state\n";

    av->modify_susceptiblilty(health, disease);

    if(cur_day >= health->get_exposure_date(disease)) {
      av->modify_infectivity(health, disease);
    }
  }

  // do evolutions...
}

void Evolution::print() {}
void Evolution::reset(int run) {}

Transmission::Loads * Evolution::get_primary_loads(int day) {
  Transmission::Loads *loads = new Transmission::Loads;
  loads->insert( pair<int, double> (1, 1) );
  return loads;
}

Transmission::Loads * Evolution::get_primary_loads(int day, int strain) {
  Transmission::Loads *loads = new Transmission::Loads;
  loads->insert( pair<int, double> (strain, 1) );
  return loads;
}

double Evolution::antigenic_diversity(Person *p1, Person *p2) {
  Infection *inf1 = p1->get_health()->get_infection(disease->get_id());
  Infection *inf2 = p2->get_health()->get_infection(disease->get_id());

  if(!inf1 || !inf2) return 0;

  vector<int> str1; inf1->get_strains(str1);
  vector<int> str2; inf1->get_strains(str2);

  // TODO how to handle multiple strains???

  return antigenic_distance(str1.at(0), str2.at(0));
}
