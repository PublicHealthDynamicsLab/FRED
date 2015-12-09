/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Drug_Use_Epidemic.cc

#include "Drug_Use_Epidemic.h"
#include "Drug_Use_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Utils.h"
#include "Population.h"
#include "Random.h"

Drug_Use_Epidemic::Drug_Use_Epidemic(Disease* _disease) :
  Epidemic(_disease) {
}


void Drug_Use_Epidemic::setup() {

  Epidemic::setup();

  // initialize Drug_Use specific-variables here:
  user_count = 0;
  non_users = 0;
  asymptomatic_users = 0;
  symptomatic_users = 0;
  pct_init_nonusers = 32675.0 / 34653.0;
  pct_init_asymp = 861.0 / 34653.0;
  pct_init_symp = 1117.0 / 34653.0;

}


void Drug_Use_Epidemic::update(int day) {

  if (day == 0) {
    // set initial users
    for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
      Person* person = Global::Pop.get_person_by_index(p);
      if(person == NULL) {
	continue;
      }
      double r = Random::draw_random();
      if (r < pct_init_asymp + pct_init_symp) {
	// infect the person
	person->become_exposed(this->id, NULL, NULL, day);
	// notify the epidemic
	Epidemic::become_exposed(person, day);
	user_count++;
	if (r < pct_init_symp) {
	  // person is symptomatic user
	  person->become_symptomatic(this->disease);
	  symptomatic_users++;
	}
	else {
	  // person is asymptomatic user
	  asymptomatic_users++;
	}
      }
      else {
	non_users++;
      }
    }
  }

  if (day > 0 && day % 7 == 0) {
    // update everyone by Markov transition probs
    for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
      Person* person = Global::Pop.get_person_by_index(p);
      if(person == NULL) {
	continue;
      }
      if (person->is_infected(this->id)) {
	if (person->is_symptomatic(this->id)) {
	  // double asymp = 0.000443480809493245;
	  // double non = 0.00053295537144841;
	  double asymp = 0.00308452512501248;
	  double non = 0.00374162758438606;
	  double r = Random::draw_random();
	  if (r < asymp + non) {
	    symptomatic_users--;
	    if (r < non) {
	      person->recover(this->disease);
	      non_users++;
	    }
	    else {
	      person->resolve_symptoms(this->disease);
	      asymptomatic_users++;
	    }
	  }
	}
	else {
	  // double non = 0.0013342186851783;
	  // double problem = 0.000781869521788936; 
	  double non = 0.0093125951705425;
	  double problem = 0.0054379938290301;
	  double r = Random::draw_random();
	  if (r < non + problem) {
	    asymptomatic_users--;
	    if (r < non) {
	      person->recover(this->disease);
	      non_users++;
	    }
	    else {
	      person->become_symptomatic(this->disease);
	      symptomatic_users++;
	    }
	  }
	}
      }
      else {
	// double asymp = 0.0000428491526283835;
	// double problem = 0.0000261325327029317;
	double asymp = 0.000299041331934375;
	double problem = 0.000183558492087282;
	double r = Random::draw_random();
	if (r < asymp + problem) {
	  non_users--;
	  // infect the person
	  person->become_exposed(this->id, NULL, NULL, day);
	  // notify the epidemic
	  Epidemic::become_exposed(person, day);
	  if (r < problem) {
	    // person is symptomatic user
	    person->become_symptomatic(this->disease);
	    symptomatic_users++;
	  }
	  else {
	    // person is asymptomatic user
	    asymptomatic_users++;
	  }
	}
      }
    }
  }

  Epidemic::update(day);

}


void Drug_Use_Epidemic::report_disease_specific_stats(int day) {

  // put values that should appear in outfile here:

  // this is just an example for testing:
  // track_value(day, (char*)"Drug_Use", user_count);
  track_value(day, (char*)"Non", non_users);
  track_value(day, (char*)"Asymp", asymptomatic_users);
  track_value(day, (char*)"Symp", symptomatic_users);

  // print additional daily output here:

}

void Drug_Use_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Drug_Use_Epidemic finished\n");

}


