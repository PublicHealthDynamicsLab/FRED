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
// File: Demographics.cc
//
#include <limits>

#include "Demographics.h"
#include "Events.h"
#include "Population.h"
#include "Age_Map.h"
#include "Person.h"
#include "Random.h"
#include "Global.h"
#include "Date.h"
#include "Utils.h"
#include "Place_List.h"
#include "Household.h"

class Global;

const double Demographics::MEAN_PREG_DAYS = 280.0; //40 weeks
const double Demographics::STDDEV_PREG_DAYS = 7.0; //1 week

// birth and death stats
int Demographics::births_today = 0;
int Demographics::births_ytd = 0;
int Demographics::total_births = 0;
int Demographics::deaths_today = 0;
int Demographics::deaths_ytd = 0;
int Demographics::total_deaths = 0;

std::vector<Person*> Demographics::birthday_vecs[367]; //0 won't be used | day 1 - 366
std::map<Person*, int> Demographics::birthday_map;

std::vector<int> Demographics::fips_codes;

Events* Demographics::conception_queue = new Events;
Events* Demographics::maternity_queue = new Events;
Events* Demographics::mortality_queue = new Events;


void Demographics::initialize_static_variables() {
  // clear birthday lists
  Demographics::birthday_map.clear();
  for(int i = 0; i < 367; ++i) {
    Demographics::birthday_vecs[i].reserve(100);
    Demographics::birthday_vecs[i].clear();
  }
}


Demographics::Demographics() {
  this->init_age = -1;
  this->age = -1;
  this->sex = 'n';
  this->birthday_sim_day = -1;
  this->deceased_sim_day = -1;
  this->conception_sim_day = -1;
  this->maternity_sim_day = -1;
  this->pregnant = false;
  this->deceased = false;
  this->relationship = -1;
  this->race = -1;
  this->number_of_children = -1;
}

void Demographics::setup(Person* self, short int _age, char _sex,
			 short int _race, short int rel, int day, bool is_newborn) {

  int self_index = self->get_pop_index();

  // adjust age for those over 89 (due to binning in the synthetic pop)
  if(_age > 89) {
    _age = 90;
    while(this->age < Demographics::MAX_AGE && Random::draw_random() < 0.6) _age++;
  }

  // set demographic variables
  this->init_age = _age;
  this->age = this->init_age;
  this->sex = _sex;
  this->race = _race;
  this->relationship = rel;
  this->deceased_sim_day = -1;
  this->conception_sim_day = -1;
  this->maternity_sim_day = -1;
  this->pregnant = false;
  this->deceased = false;
  this->number_of_children = 0;

  if(is_newborn) {
    // today is birthday
    this->birthday_sim_day = day;
  } else {
    // set the agent's birthday relative to simulation day
    this->birthday_sim_day = day - 365 * this->age;
    // adjust for leap years:
    this->birthday_sim_day -= (this->age / 4);
    // pick a random birthday in the previous year
    this->birthday_sim_day -= Random::draw_random_int(1,365);
  }
}

Demographics::~Demographics() {
}

void Demographics::initialize_demographic_dynamics(Person* self) {

  FRED_VERBOSE(1, "demographic dynamics: id = %d age = %d\n", self->get_id(), this->age);

  int day = Global::Simulation_Day;;

  // add self to birthday list
  Demographics::add_to_birthday_list(self);

  // will this person die in the next year?
  double age_specific_probability_of_death = 0.0;
  if(Demographics::MAX_AGE <= this->age) {
    age_specific_probability_of_death = 1.0;
    FRED_STATUS(1, "DAY %d DEATH BY MAX_AGE RULE\n", day);
  } else {
    // look up mortality in the mortality rate tables
    int fips = self->get_household()->get_county_fips();
    age_specific_probability_of_death = Global::Places.get_county(fips)->get_mortality_rate(this->age, this->sex);
  }
  
  if(Random::draw_random() <= age_specific_probability_of_death) {
    //Yes, so set the death day (in simulation days)
    this->deceased_sim_day = (day + Random::draw_random_int(1, 364));
    Demographics::add_mortality_event(this->deceased_sim_day, self);
    FRED_STATUS(1, "MORTALITY EVENT ADDDED today %d id %d age %d decease %d\n",
		day, self->get_id(), age, deceased_sim_day);
  }
  
  // set pregnancy status
  int fips = self->get_household()->get_county_fips();
  double pregnancy_rate = Global::Places.get_county(fips)->get_pregnancy_rate(this->age);
  if(this->sex == 'F' &&
     Demographics::MIN_PREGNANCY_AGE <= this->age &&
     this->age <= Demographics::MAX_PREGNANCY_AGE &&
     self->lives_in_group_quarters() == false &&
     Random::draw_random() < pregnancy_rate) {
    
    // decide if already pregnant
    int current_day_of_year = Date::get_day_of_year(day);
    int days_since_birthday = current_day_of_year - Date::get_day_of_year(this->birthday_sim_day);
    if(days_since_birthday < 0) {
      days_since_birthday += 365;
    }
    double fraction_of_year = static_cast<double>(days_since_birthday) / 366.0;
    
    if(Random::draw_random() < fraction_of_year) {
      // already pregnant
      this->conception_sim_day = day - Random::draw_random_int(0,days_since_birthday); 
      int length_of_pregnancy = (int) (Random::draw_normal(Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5);
      this->maternity_sim_day = this->conception_sim_day + length_of_pregnancy;
      if(this->maternity_sim_day > 0) {
	      Demographics::add_maternity_event(maternity_sim_day, self);
	      FRED_STATUS(1, "MATERNITY EVENT ADDDED today %d id %d age %d due %d\n",
		                day, self->get_id(),age,maternity_sim_day);
	      this->pregnant = true;
	      this->conception_sim_day = -1;
      } else {
	      // already gave birth before start of sim
	      this->pregnant = false;
	      this->conception_sim_day = -1;
      }
    } else {
      // will conceive before next birthday
      this->conception_sim_day = day + Random::draw_random_int(1, 365 - days_since_birthday);
      Demographics::add_conception_event(this->conception_sim_day, self);
      FRED_STATUS(1, "CONCEPTION EVENT ADDDED today %d id %d age %d conceive %d house %s\n",
		  day, self->get_id(),age,conception_sim_day,self->get_household()->get_label());
    }
  } // end test for pregnancy
}


int Demographics::get_day_of_year_for_birthday_in_nonleap_year() {
  int day_of_year = Date::get_day_of_year(this->birthday_sim_day);
  int year = Date::get_year(this->birthday_sim_day);
  if(Date::is_leap_year(year) && 59 < day_of_year) {
    day_of_year--;
  }
  return day_of_year;
}

void Demographics::cancel_mortality(Person* self) {
  assert(this->deceased_sim_day > -1);
  Demographics::delete_mortality_event(this->deceased_sim_day, self);
  FRED_STATUS(0, "MORTALITY EVENT DELETED\n");
  this->deceased_sim_day = -1;
}

void Demographics::cancel_conception(Person* self) {
  assert(this->conception_sim_day > -1);
  Demographics::delete_conception_event(this->conception_sim_day, self);
  FRED_STATUS(0, "CONCEPTION EVENT DELETED\n");
  this->conception_sim_day = -1;
}

void Demographics::become_pregnant(int day, Person* self) {
  // No pregnancies in group quarters
  if(self->lives_in_group_quarters()) {
    FRED_STATUS(0, "GQ PREVENTS PREGNANCY today %d id %d age %d\n",
		day, self->get_id(), self->get_age());
    this->conception_sim_day = -1;
    return;
  }
  int length_of_pregnancy = (int) (Random::draw_normal(Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5);
  this->maternity_sim_day = this->conception_sim_day + length_of_pregnancy;
  Demographics::add_maternity_event(maternity_sim_day, self);
  FRED_STATUS(1, "MATERNITY EVENT ADDDED today %d id %d age %d due %d\n",
	            day, self->get_id(),age,maternity_sim_day);
  this->pregnant = true;
  this->conception_sim_day = -1;
}


void Demographics::cancel_pregnancy(Person* self ) {
  assert(this->pregnant == true);
  Demographics::delete_maternity_event(maternity_sim_day, self);
  FRED_STATUS(0, "MATERNITY EVENT DELETED\n");
  this->maternity_sim_day = -1;
  this->pregnant = false;
}

void Demographics::update_birth_stats(int day, Person* self) {
  // NOTE: This is called by Person::give_birth() to update stats.
  // The baby is actually created in Person::give_birth()
  this->pregnant = false;
  this->maternity_sim_day = -1;
  this->number_of_children++;
  Demographics::births_today++;
  Demographics::births_ytd++;
  Demographics::total_births++;

  if(Global::Report_County_Demographic_Information) {
    Place* hh = self->get_household();
    int fips = 0;
    if(hh != NULL) {
      fips = hh->get_county_fips();
    } else if(Global::Enable_Hospitals && self->is_hospitalized()) {
      hh = self->get_permanent_household();
      if(hh != NULL) {
        fips = hh->get_county_fips();
      }
    } else if(Global::Enable_Travel) {
      hh = self->get_permanent_household();
      if(hh != NULL) {
        fips = hh->get_county_fips();
      }
    }
    assert(fips != 0);
  }

  if(Global::Birthfp != NULL) {
    // report births
    fprintf(Global::Birthfp, "day %d mother %d age %d\n",
	    day, self->get_id(), self->get_age());
    fflush(Global::Birthfp);
  }
}


void Demographics::birthday(Person* self, int day ) {

  if(!Global::Enable_Population_Dynamics) {
    return;
  }

  FRED_STATUS(2, "Birthday entered for person %d with (previous) age %d\n", self->get_id(), self->get_age());

  int fips = self->get_household()->get_county_fips();
  //The count of agents at the current age is decreased by 1
  Global::Places.decrement_population_of_county(fips, self);
  // change age
  this->age++;
  //The count of agents at the new age is increased by 1
  Global::Places.increment_population_of_county(fips, self);

  // will this person die in the next year?
  double age_specific_probability_of_death = 0.0;
  if(Demographics::MAX_AGE <= this->age) {
    age_specific_probability_of_death = 1.0;
    // printf("DAY %d DEATH BY MAX_AGE RULE\n", day);
    /*
      } else if (self->is_nursing_home_resident()) {
      age_specific_probability_of_death = 0.25;
      }
    */
  } else {
    // look up mortality in the mortality rate tables
    age_specific_probability_of_death = Global::Places.get_county(fips)->get_mortality_rate(this->age, this->sex);
  }
  if(this->deceased == false && this->deceased_sim_day == -1 &&
       Random::draw_random() <= age_specific_probability_of_death) {
    
    // Yes, so set the death day (in simulation days)
    this->deceased_sim_day = (day + Random::draw_random_int(0,364));
    Demographics::add_mortality_event(deceased_sim_day, self);
    FRED_STATUS(1, "MORTALITY EVENT ADDDED today %d id %d age %d decease %d\n", day, self->get_id(), age, deceased_sim_day);
  } else {
    FRED_STATUS(2, "SURVIVER: AGE %d deceased_sim_day = %d\n", age, this->deceased_sim_day);
  }
  
  // Will this person conceive in the coming year year?
  double pregnancy_rate = Global::Places.get_county(fips)->get_pregnancy_rate(this->age);
  if(this->sex == 'F' &&
     Demographics::MIN_PREGNANCY_AGE <= this->age &&
     this->age <= Demographics::MAX_PREGNANCY_AGE &&
     this->conception_sim_day == -1 && this->maternity_sim_day == -1 &&
     self->lives_in_group_quarters() == false &&
     Random::draw_random() < pregnancy_rate) {
    
    assert(this->pregnant == false);
    
    // ignore small distortion due to leap years
    this->conception_sim_day = day + Random::draw_random_int(1, 365);
    Demographics::add_conception_event(this->conception_sim_day, self);
    FRED_STATUS(1, "CONCEPTION EVENT ADDDED today %d id %d age %d conceive %d house %s\n",
		day, self->get_id(),age,conception_sim_day,self->get_household()->get_label());
  }

  // become responsible for health decisions when reaching adulthood
  if(this->age == Global::ADULT_AGE && self->is_health_decision_maker() == false) {
    FRED_STATUS(2, "Become_health_decision_maker\n");
    self->become_health_decision_maker(self);
  }
  
  // update any state-base health conditions
  self->update_health_conditions(day);

  FRED_STATUS(2, "Birthday finished for person %d with new age %d\n", self->get_id(), self->get_age());
}

void Demographics::terminate(Person* self) {
  int day = Global::Simulation_Day;
  FRED_STATUS(1, "Demographics::terminate day %d person %d age %d\n",
	      day, self->get_id(), this->age);

  // cancel any planned pregnancy
  if(day <= this->conception_sim_day) {
    FRED_STATUS(0, "DEATH CANCELS PLANNED CONCEPTION: today %d person %d age %d conception %d\n",
	              day, self->get_id(), this->age, this->conception_sim_day);
    cancel_conception(self);
  }

  // cancel any current pregnancy
  if(this->pregnant) {
    FRED_STATUS(0, "DEATH CANCELS PREGNANCY: today %d person %d age %d due %d\n",
	              day, self->get_id(), this->age, this->maternity_sim_day);
    cancel_pregnancy(self);
  }

  // remove from the birthday lists
  if(Global::Enable_Population_Dynamics) {
    Demographics::delete_from_birthday_list(self);
  }

  // cancel any future mortality event
  if(day < this->deceased_sim_day) {
    FRED_STATUS(0, "DEATH CANCELS FUTURE MORTALITY: today %d person %d age %d mortality %d\n",
	              day, self->get_id(), this->age, this->deceased_sim_day);
    cancel_mortality(self);
  }

  // update death stats
  Demographics::deaths_today++;
  Demographics::deaths_ytd++;
  Demographics::total_deaths++;

  if(Global::Deathfp != NULL) {
    // report deaths
    fprintf(Global::Deathfp, "day %d person %d age %d\n",
	    day, self->get_id(), self->get_age());
    fflush(Global::Deathfp);
  }
}


void Demographics::print() {
}

double Demographics::get_real_age() const {
  return double(Global::Simulation_Day - this->birthday_sim_day) / 365.25;
}

//////// Static Methods

int Demographics::find_fips_code(int n) {
  int size = Demographics::fips_codes.size();
  for(int i = 0; i < size; ++i) {
    if(Demographics::fips_codes[i] == n) {
      return i;
    }
  }
  return -1;
}

void Demographics::update(int day) {

  // reset counts of births and deaths
  Demographics::births_today = 0;
  Demographics::deaths_today = 0;

  if (!Global::Enable_Population_Dynamics) {
    return;
  }

  Demographics::update_people_on_birthday_list(day);

  // initiate pregnancies
  // FRED_VERBOSE(0, "conception queue\n");
  int size = Demographics::conception_queue->get_size(day);
  for(int i = 0; i < size; ++i) {
    Person* person = Demographics::conception_queue->get_event(day, i);
    person->get_demographics()->become_pregnant(day, person);
  }
  Demographics::conception_queue->clear_events(day);

  // add newborns to the population
  // FRED_VERBOSE(0, "maternity queue\n");
  size = Demographics::maternity_queue->get_size(day);
  for (int i = 0; i < size; ++i) {
    Person* person = Demographics::maternity_queue->get_event(day, i);
    person->give_birth(day);
  }
  Demographics::maternity_queue->clear_events(day);

  // FRED_VERBOSE(0, "mortality queue\n");
  size = Demographics::mortality_queue->get_size(day);
  for(int i = 0; i < size; ++i) {
    // add dying to the population death_list
    Person * person = Demographics::mortality_queue->get_event(day, i);
    FRED_CONDITIONAL_VERBOSE(0, Global::Enable_Health_Records,
			     "HEALTH RECORD: %s person %d age %d DIES FROM UNKNOWN CAUSE.\n",
			     Date::get_date_string().c_str(),
			     person->get_id(), person->get_age());
    // queue removal from population
    Global::Pop.prepare_to_die(day, person);
  }
  Demographics::mortality_queue->clear_events(day);

  return;
}

void Demographics::add_to_birthday_list(Person* person) {
  int day_of_year = person->get_demographics()->get_day_of_year_for_birthday_in_nonleap_year();
  Demographics::birthday_vecs[day_of_year].push_back(person);
  FRED_VERBOSE(2,
	       "Adding person %d to birthday vector for day = %d.\n ... birthday_vecs[ %d ].size() = %zu\n",
	       person->get_id(), day_of_year, day_of_year, Demographics::birthday_vecs[ day_of_year ].size());
  Demographics::birthday_map[person] = (static_cast<int>(Demographics::birthday_vecs[day_of_year].size()) - 1);
}

void Demographics::delete_from_birthday_list(Person* person) {
  int day_of_year = person->get_demographics()->get_day_of_year_for_birthday_in_nonleap_year();

  FRED_VERBOSE(2,
	       "deleting person %d to birthday vector for day = %d.\n ... birthday_vecs[ %d ].size() = %zu\n",
	       person->get_id(), day_of_year, day_of_year, Demographics::birthday_vecs[ day_of_year ].size());

  map<Person*, int>::iterator itr;
  itr = Demographics::birthday_map.find(person);
  if(itr == Demographics::birthday_map.end()) {
    FRED_VERBOSE(0, "Help! person %d deleted, but not in the birthday_map\n",
		 person->get_id());
  }
  assert(itr != Demographics::birthday_map.end());
  int pos = (*itr).second;

  // copy last person in birthday list into this person's slot
  Person* last = Demographics::birthday_vecs[day_of_year].back();
  Demographics::birthday_vecs[day_of_year][pos] = last;
  Demographics::birthday_map[last] = pos;

  // delete last slot
  Demographics::birthday_vecs[day_of_year].pop_back();

  // delete from birthday map
  Demographics::birthday_map.erase(itr);

  FRED_VERBOSE(2,
	       "deleted person %d to birthday vector for day = %d.\n ... birthday_vecs[ %d ].size() = %zu\n",
	       person->get_id(), day_of_year, day_of_year, Demographics::birthday_vecs[ day_of_year ].size());

}

void Demographics::update_people_on_birthday_list(int day) {

  int birthday_index = Date::get_day_of_year();
  if(Date::is_leap_year()) {
    // skip feb 29
    if(birthday_index == 60) {
      FRED_VERBOSE(1, "LEAP DAY day %d index %d\n", day, birthday_index);
      return;
    }
    // shift all days after feb 28 forward
    if(60 < birthday_index) {
      birthday_index--;
    }
  }
  
  int size = static_cast<int>(Demographics::birthday_vecs[birthday_index].size());

  FRED_VERBOSE(1, "day %d update_people_on_birthday_list started. size = %d\n",
	       day, size);
  
  // make a temporary list of birthday people
  std::vector<Person*> birthday_list;
  birthday_list.reserve(size);
  birthday_list.clear();
  for(int p = 0; p < size; ++p) {
    Person* person = Demographics::birthday_vecs[birthday_index][p];
    birthday_list.push_back(person);
  }

  // update everyone on birthday list
  int birthday_count = 0;
  for(int p = 0; p < size; ++p) {
    Person* person = birthday_list[p];
    person->birthday(day);
    birthday_count++;
  }

  size = static_cast<int>(Demographics::birthday_vecs[birthday_index].size());

  FRED_VERBOSE(1, "day %d update_people_on_birthday_list finished. size = %d\n",
	       day, size);
  
}

void Demographics::report(int day) {
  char filename[FRED_STRING_SIZE];
  FILE* fp = NULL;

  // get the current year
  int year = Date::get_year();

  sprintf(filename, "%s/ages-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");

  int n0, n5, n18, n65;
  int count[20];
  int total = 0;
  n0 = n5 = n18 = n65 = 0;
  // age distribution
  for(int c = 0; c < 20; ++c) {
    count[c] = 0;
  }
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    int a = person->get_age();
    if(a < 5) {
      n0++;
    } else if(a < 18) {
      n5++;
    } else if(a < 65) {
      n18++;
    } else {
      n65++;
    }
    int n = a / 5;
    if(n < 20) {
      count[n]++;
    } else {
      count[19]++;
    }
    total++;
  }
  // fprintf(fp, "\nAge distribution: %d people\n", total);
  for(int c = 0; c < 20; ++c) {
    fprintf(fp, "age %2d to %d: %6d %e %d\n", 5 * c, 5 * (c + 1) - 1, count[c],
	    (1.0 * count[c]) / total, total);
  }
  /*
    fprintf(fp, "AGE 0-4: %d %.2f%%\n", n0, (100.0 * n0) / total);
    fprintf(fp, "AGE 5-17: %d %.2f%%\n", n5, (100.0 * n5) / total);
    fprintf(fp, "AGE 18-64: %d %.2f%%\n", n18, (100.0 * n18) / total);
    fprintf(fp, "AGE 65-100: %d %.2f%%\n", n65, (100.0 * n65) / total);
    fprintf(fp, "\n");
  */
  fclose(fp);

}

void Demographics::add_conception_event(int day, Person* person) {
  Demographics::conception_queue->add_event(day, person);
}

void Demographics::delete_conception_event(int day, Person* person) {
  Demographics::conception_queue->delete_event(day, person);
}

void Demographics::add_maternity_event(int day, Person* person) {
  Demographics::maternity_queue->add_event(day, person);
}

void Demographics::delete_maternity_event(int day, Person* person) {
  Demographics::maternity_queue->delete_event(day, person);
}

void Demographics::add_mortality_event(int day, Person* person) {
  Demographics::mortality_queue->add_event(day, person);
}

void Demographics::delete_mortality_event(int day, Person* person) {
  Demographics::mortality_queue->delete_event(day, person);
}

