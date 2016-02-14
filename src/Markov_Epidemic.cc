/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Markov_Epidemic.cc

#include <math.h>
#include "Events.h"
#include "Date.h"
#include "Condition.h"
#include "Global.h"
#include "Health.h"
#include "Markov_Epidemic.h"
#include "Markov_Natural_History.h"
#include "Markov_Chain.h"
#include "Params.h"
#include "Person.h"
#include "Population.h"
#include "Place.h"
#include "Place_List.h"
#include "Random.h"
#include "Utils.h"


Markov_Epidemic::Markov_Epidemic(Condition* _condition) :
  Epidemic(_condition) {
}


void Markov_Epidemic::setup() {

  Epidemic::setup();

  // initialize Markov specific-variables here:

  this->markov_chain = static_cast<Markov_Natural_History*>(this->condition->get_natural_history())->get_markov_chain();
  // FRED_VERBOSE(0, "Markov_Epidemic(%s)::setup\n", this->condition->get_condition_name());
  this->number_of_states = this->markov_chain->get_number_of_states();
  FRED_VERBOSE(0, "Markov_Epidemic::setup states = %d\n", this->number_of_states);

  // this->people_in_state = new person_vector_t [this->number_of_states];
  this->count = new int [this->number_of_states];
  this->transition_to_state_event_queue = new Events* [this->number_of_states];

  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_to_state_event_queue[i] = new Events;
  }

  // optional parameters
  this->target_locations = 0;
  Params::disable_abort_on_failure();
  int n = 0;
  Params::get_indexed_param(this->condition->get_condition_name(),"target_locations",&n);
  if (n > 0) {
    this->coordinates = new double [n];
    Params::get_indexed_param_vector(this->condition->get_condition_name(), "target_locations", this->coordinates);
    // the number of location is half the number of coordinates
    this->target_locations = n/2;
  }

  this->use_bayesian_init = 0;
  char bayesian_initfile[FRED_STRING_SIZE];
  sprintf(bayesian_initfile, "none");
  Params::get_indexed_param(this->condition->get_condition_name(),"bayesian_initfile", bayesian_initfile);
  if (strcmp(bayesian_initfile, "none") != 0) {
    this->use_bayesian_init = 1;
    int age,sex,race,county;
    double nonuser, asymp, problem, death_asymp, death_problem;
    FILE* fp;
    fp = Utils::fred_open_file(bayesian_initfile);
    fscanf(fp, "%*s ");
    while (fscanf(fp, "%d %d %d %d %lf %lf %lf %lf %lf ",
		  &age,&sex,&race,&county,
		  &nonuser, &asymp,
		  &problem,
		  &death_asymp,
		  &death_problem) == 9) {
      pnon[age][sex][race][county] = nonuser;
      pasymp[age][sex][race][county] = asymp;
      pproblem[age][sex][race][county] = problem;
      dasymp[age][sex][race][county] = death_asymp;
      dproblem[age][sex][race][county] = death_problem;
    }
    fclose(fp);
  }

  this->ramp = 0.0;
  Params::get_indexed_param(this->condition->get_condition_name(),"ramp", &this->ramp);

  Params::set_abort_on_failure();

  FRED_VERBOSE(0, "%.9f %.9f %.9f %.9f %.9f\n",
	       pnon[13][2][3][65],
	       pasymp[13][2][3][65],
	       pproblem[13][2][3][65],
	       dasymp[13][2][3][65],
	       dproblem[13][2][3][65]);
  FRED_VERBOSE(0, "Markov_Epidemic(%s)::setup finished\n", this->condition->get_condition_name());
}


int Markov_Epidemic::get_age_code(int age) {
  if (age < 1) {
    return 1;
  }
  if (85 <= age) {
    return 13;
  }
  if (age < 25) {
    return 2 + age/5;
  }
  return 7 + (age-25)/10;
}

int Markov_Epidemic::get_sex_code(char sex) {
  return (sex == 'F') ? 1 : 2; 
}

int Markov_Epidemic::get_race_code(int race) {
  if (race == Global::WHITE) {
    return 3;
  }
  if (race == Global::AFRICAN_AMERICAN) {
    return 1;
  }
  return 2;
}

int Markov_Epidemic:: get_county_code(int fips) {
  switch (fips) {
  case 42001:
    return 1;
    break;

  case 42003:
    return 2;
    break;

  case 42005:
    return 3;
    break;

  case 42007:
    return 4;
    break;

  case 42009:
    return 5;
    break;

  case 42011:
    return 6;
    break;

  case 42013:
    return 7;
    break;

  case 42015:
    return 8;
    break;

  case 42017:
    return 9;
    break;

  case 42019:
    return 10;
    break;

  case 42021:
    return 11;
    break;

  case 42023:
    return 12;
    break;

  case 42025:
    return 13;
    break;

  case 42027:
    return 14;
    break;

  case 42029:
    return 15;
    break;

  case 42031:
    return 16;
    break;

  case 42033:
    return 17;
    break;

  case 42035:
    return 18;
    break;

  case 42037:
    return 19;
    break;

  case 42039:
    return 20;
    break;

  case 42041:
    return 21;
    break;

  case 42043:
    return 22;
    break;

  case 42045:
    return 23;
    break;

  case 42047:
    return 24;
    break;

  case 42049:
    return 25;
    break;

  case 42051:
    return 26;
    break;

  case 42053:
    return 27;
    break;

  case 42055:
    return 28;
    break;

  case 42057:
    return 29;
    break;

  case 42059:
    return 30;
    break;

  case 42061:
    return 31;
    break;

  case 42063:
    return 32;
    break;

  case 42065:
    return 33;
    break;

  case 42067:
    return 34;
    break;

  case 42069:
    return 35;
    break;

  case 42071:
    return 36;
    break;

  case 42073:
    return 37;
    break;

  case 42075:
    return 38;
    break;

  case 42077:
    return 39;
    break;

  case 42079:
    return 40;
    break;

  case 42081:
    return 41;
    break;

  case 42083:
    return 42;
    break;

  case 42085:
    return 43;
    break;

  case 42087:
    return 44;
    break;

  case 42089:
    return 45;
    break;

  case 42091:
    return 46;
    break;

  case 42093:
    return 47;
    break;

  case 42095:
    return 48;
    break;

  case 42097:
    return 49;
    break;

  case 42099:
    return 50;
    break;

  case 42101:
    return 51;
    break;

  case 42103:
    return 52;
    break;

  case 42105:
    return 53;
    break;

  case 42107:
    return 54;
    break;

  case 42109:
    return 55;
    break;

  case 42111:
    return 56;
    break;

  case 42113:
    return 57;
    break;

  case 42115:
    return 58;
    break;

  case 42117:
    return 59;
    break;

  case 42119:
    return 60;
    break;

  case 42121:
    return 61;
    break;

  case 42123:
    return 62;
    break;

  case 42125:
    return 63;
    break;

  case 42127:
    return 64;
    break;

  case 42129:
    return 65;
    break;

  case 42131:
    return 66;
    break;

  case 42133:
    return 67;
    break;

  }

  return -1;
}


void Markov_Epidemic::prepare() {

  Epidemic::prepare();

  FRED_VERBOSE(0, "Markov_Epidemic(%s)::prepare\n", this->condition->get_condition_name());

  for (int i = 0; i < this->number_of_states; i++) {
    // this->people_in_state[i].reserve(Global::Pop.get_pop_size());
    // this->people_in_state[i].clear();
    this->count[i] = 0;
  }

  // initialize the population
  double prob[3];
  int day = 0;
  int popsize = Global::Pop.get_pop_size();
  for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
    Person* person = Global::Pop.get_person_by_index(p);
    int state = -1;
    if(person == NULL) {
      continue;
    }
    if (this->use_bayesian_init) {
      int age = get_age_code(person->get_age());
      int sex = get_sex_code(person->get_sex());
      int race = get_race_code(person->get_race());
      int county = get_county_code(person->get_household()->get_household_fips());
      prob[0] = pnon[age][sex][race][county];
      prob[1] = pasymp[age][sex][race][county];
      prob[2] = pproblem[age][sex][race][county];
      prob[2] = 1.0 - prob[0] - prob[1];
      double r = Random::draw_random();
      double sum = 0.0;
      for (int i = 0; i <= 2; i++) {
	sum += prob[i];
	if (r < sum) {
	  state = i;
	  break;
	}
      }
      printf("BAYESIAN INIT: person %d age %d sex %d race %d county %d state %d %f %f %f r = %.9f sum = %.9f\n", person->get_id(),age,sex,race,county,state,prob[0],prob[1],prob[2],r, sum); 

      // ignore state 2 if target_locations
      if (this->target_locations > 0 && state == 2) {
	state = 0;
      }
    }
    else {
      double age = person->get_real_age();
      state = this->markov_chain->get_initial_state(age);
    }
    assert(state > -1);
    transition_person(person, day, state);
  }

  if (Global::Verbose > 0) {
    printf("Markov Chain states: \n");
    for (int i = 0; i < this->number_of_states; i++) {
      printf(" | %d %s = %d", i, this->markov_chain->get_state_name(i).c_str(), this->count[i]);
    }
    printf("\n");
  }

  FRED_VERBOSE(0, "Markov_Epidemic(%s)::prepare finished\n", this->condition->get_condition_name());
}


void Markov_Epidemic::markov_updates(int day) {
  FRED_VERBOSE(0, "Markov_Epidemic(%s)::update for day %d\n", this->condition->get_condition_name(), day);

  // handle scheduled transitions to each state
  for (int state = 0; state < this->number_of_states; state++) {

    int size = this->transition_to_state_event_queue[state]->get_size(day);
    FRED_VERBOSE(1, "MARKOV_TRANSITION_TO_STATE %d day %d %s size %d\n", state, day, Date::get_date_string().c_str(), size);
    
    for(int i = 0; i < size; ++i) {
      Person* person = this->transition_to_state_event_queue[state]->get_event(day, i);
      transition_person(person, day, state);
    }

    this->transition_to_state_event_queue[state]->clear_events(day);
  }
  FRED_VERBOSE(1, "Markov_Epidemic(%s)::markov_update finished for day %d\n", this->condition->get_condition_name(), day);
  return;
}



void Markov_Epidemic::update(int day) {
  Epidemic::update(day);
}


void Markov_Epidemic::transition_person(Person* person, int day, int state) {

  int old_state = person->get_health_state(this->id);
  double age = person->get_real_age();

  FRED_VERBOSE(1, "TRANSITION_PERSON day %d id %d age %0.2f old_state %d state %d age group %d old_age_group %d\n", 
	       day, person->get_id(), age, old_state, state, 
	       this->markov_chain->get_age_group(age), this->markov_chain->get_age_group(age-1)); 

  if (this->ramp > 0.0) {
    if (state > 0 && day < 365*5) {
      double prob = 1.0 - 0.5*(365*5.0-day)/(5.0*365.0);
      if (Random::draw_random() < prob) {
	state = 0;
      }
    }
  }

  if (state == old_state && 1 <= age &&
      this->markov_chain->get_age_group(age) == this->markov_chain->get_age_group(age-1)) { 
    // this is a birthday check-in and no age group change has occurred.
    // FRED_VERBOSE(0, "birthday check-in -- no age group change\n");
    return;
  }

  // cancel any scheduled transition
  int next_state = person->get_next_health_state(this->id);
  int transition_day = person->get_next_health_transition_day(this->id);
  if (0 <= next_state && day < transition_day) {
    this->transition_to_state_event_queue[next_state]->delete_event(transition_day, person);
  }
  
  // change active list if necessary
  if (old_state != state) {
    if (0 <= old_state) {
      // delete from old list
      count[old_state]--;
      /*
      for (int j = 0; j < people_in_state[old_state].size(); j++) {
	if (people_in_state[old_state][j] == person) {
	  people_in_state[old_state][j] = people_in_state[old_state].back();
	  people_in_state[old_state].pop_back();
	}
      }
      */
    }
    if (0 <= state) {
      // add to active people list
      count[state]++;
      // people_in_state[state].push_back(person);
    }
  }

  // update person's state
  if (old_state != state) {
    person->set_health_state(this->id, state, day);
  }

  // update next event list
  if (this->target_locations > 0) {
    // use distance from suppliers
    int adjustment_state = 2;
    double adjustment = 1.0;

    double my_lat = person->get_household()->get_latitude();
    double my_lon = person->get_household()->get_longitude();

    // find min distance to a target location
    double min_dist = 99999999999.0;
    for (int loc = 0; loc  < this->target_locations; loc++) {
      double target_lat = this->coordinates[2*loc];
      double target_lon = this->coordinates[2*loc+1];
      double dist = Geo::xy_distance(my_lat, my_lon, target_lat, target_lon);
      if (dist < min_dist) {
	min_dist = dist;
      }
    }
    // adjustment = 0.5 when min_dist = 30km
    adjustment = 900.0/(900.0+min_dist*min_dist);
    this->markov_chain->get_next_state(day, age, state, &next_state, &transition_day, adjustment_state, adjustment);
  }
  else {
    this->markov_chain->get_next_state(day, age, state, &next_state, &transition_day);
  }

  FRED_VERBOSE(1, "TRANSITION_PERSON %d get_next_state = %d trans_day %d\n",
	       person->get_id(), next_state, transition_day);

  this->transition_to_state_event_queue[next_state]->add_event(transition_day, person);
    
  // update person's next state
  person->set_next_health_state(this->id, next_state, transition_day);

  FRED_CONDITIONAL_STATUS(0, Global::Enable_Health_Records,
	       "HEALTH RECORD: %s day %d person %d age %.1f %s CONDITION CHANGES from %s (%d) to %s (%d), next_state %s (%d) scheduled %d days from now (%d)\n",
	       Date::get_date_string().c_str(), day,
	       person->get_id(), age, this->condition->get_condition_name(),
			  old_state > -1? this->markov_chain->get_state_name(old_state).c_str(): "Unset", old_state, 
	       this->markov_chain->get_state_name(state).c_str(), state, 
	       this->markov_chain->get_state_name(next_state).c_str(), next_state, 
	       transition_day-day, transition_day);

  // the remainder of this method only applies to conditions that cause infection
  if (this->causes_infection == false) {
    return;
  }

  // update epidemic counters and person's health record

  if (old_state <= 0 && state != 0) {
    // infect the person
    person->become_exposed(this->id, NULL, NULL, day);
    // notify the epidemic
    Epidemic::become_exposed(person, day);
  }
  
  if (this->condition->get_natural_history()->get_symptoms(state) > 0.0 && person->is_symptomatic(this->id)==false) {
    Epidemic::become_symptomatic(person, day);
  }

  if (this->condition->get_natural_history()->get_infectivity(state) > 0.0 && person->is_infectious(this->id)==false) {
    Epidemic::become_infectious(person, day);
  }

  if (this->condition->get_natural_history()->get_symptoms(state) == 0.0 && person->is_symptomatic(this->id)) {
    Epidemic::become_asymptomatic(person, day);
  }

  if (this->condition->get_natural_history()->get_infectivity(state) == 0.0 && person->is_infectious(this->id)) {
    Epidemic::become_noninfectious(person, day);
  }

  if (old_state > 0 && state == 0) {
    Epidemic::recover(person, day);
  }

  if (this->condition->get_natural_history()->is_fatal(state)) {
    // update person's health record
    person->become_case_fatality(day, this->condition);
  }
  // FRED_VERBOSE(0, "transition_person %d day %d to state %d finished\n", person->get_id(), day, state);
}


void Markov_Epidemic::report_condition_specific_stats(int day) {
  FRED_VERBOSE(1, "Markov Epidemic %s report day %d \n",this->condition->get_condition_name(),day);
  for (int i = 0; i < this->number_of_states; i++) {
    char str[80];
    strcpy(str,this->markov_chain->get_state_name(i).c_str());
    Utils::track_value(day, str, count[i]);
  }
}


void Markov_Epidemic::end_of_run() {

  // print end-of-run statistics here:
  printf("Markov_Epidemic finished\n");

}


void Markov_Epidemic::terminate_person(Person* person, int day) {
  FRED_VERBOSE(1, "MARKOV EPIDEMIC TERMINATE person %d day %d %s\n",
	       person->get_id(), day, Date::get_date_string().c_str());

  // delete from state list
  int state = person->get_health_state(this->id);
  if (0 <= state) {
    count[state]--;
    /*
    for (int j = 0; j < people_in_state[state].size(); j++) {
      if (people_in_state[state][j] == person) {
	people_in_state[state][j] = people_in_state[state].back();
	people_in_state[state].pop_back();
      }
    }
    */
    FRED_VERBOSE(1, "MARKOV EPIDEMIC TERMINATE person %d day %d %s removed from state %d\n",
		 person->get_id(), day, Date::get_date_string().c_str(), state);
  }

  // cancel any scheduled transition
  int next_state = person->get_next_health_state(this->id);
  int transition_day = person->get_next_health_transition_day(this->id);
  if (0 <= next_state && day <= transition_day) {
    // printf("person %d delete_event for state %d transition_day %d\n", person->get_id(), next_state, transition_day);
    this->transition_to_state_event_queue[next_state]->delete_event(transition_day, person);
  }

  person->set_health_state(this->id, -1, day);

  // notify Epidemic
  Epidemic::terminate_person(person, day);

  FRED_VERBOSE(1, "MARKOV EPIDEMIC TERMINATE finished\n");
}


