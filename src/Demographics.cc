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

bool Demographics::is_initialized = false;
double Demographics::male_mortality_rate[Demographics::MAX_AGE + 1];
double Demographics::female_mortality_rate[Demographics::MAX_AGE + 1];
double Demographics::adjusted_male_mortality_rate[Demographics::MAX_AGE + 1];
double Demographics::adjusted_female_mortality_rate[Demographics::MAX_AGE + 1];
double Demographics::birth_rate[Demographics::MAX_AGE + 1];
double Demographics::adjusted_birth_rate[Demographics::MAX_AGE + 1];
double Demographics::pregnancy_rate[Demographics::MAX_AGE + 1];
int Demographics::target_popsize = 0;
int Demographics::control_population_growth_rate = 1;
double Demographics::population_growth_rate = 0.0;
double Demographics::college_departure_rate = 0.0;
double Demographics::military_departure_rate = 0.0;
double Demographics::prison_departure_rate = 0.0;
double Demographics::youth_home_departure_rate = 0.0;
double Demographics::adult_home_departure_rate = 0.0;
int Demographics::births_today = 0;
int Demographics::births_ytd = 0;
int Demographics::total_births = 0;
int Demographics::deaths_today = 0;
int Demographics::deaths_ytd = 0;
int Demographics::total_deaths = 0;
int Demographics::houses = 0;
int * Demographics::beds = NULL;
int * Demographics::occupants = NULL;
std::vector <Person*> Demographics::birthday_vecs[367]; //0 won't be used | day 1 - 366
std::map<Person*, int> Demographics::birthday_map;

Events * Demographics::conception_queue = new Events;
Events * Demographics::maternity_queue = new Events;
Events * Demographics::mortality_queue = new Events;

int max_beds = -1;
int max_occupants = -1;
std::vector < pair<Person *, int> > ready_to_move;

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
}

void Demographics::setup( Person * self, short int _age, char _sex, 
			   short int _race, short int rel, int day, bool is_newborn ) {

  int self_index = self->get_pop_index();

  // adjust age for those over 89 (due to binning in the synthetic pop)
  if (_age > 89) {
    _age = 90;
    while (age < MAX_AGE && RANDOM() < 0.6) _age++;
  }

  // set demographic variables
  init_age            = _age;
  age                 = init_age;
  sex                 = _sex;
  race                = _race;
  relationship        = rel;
  deceased_sim_day    = -1;
  conception_sim_day  = -1;
  maternity_sim_day         = -1;
  pregnant            = false;
  deceased            = false;
  number_of_children = 0;

  if (is_newborn) {
    // today is birthday
    birthday_sim_day = day;
  }
  else {
    // set the agent's birthday relative to simulation day
    birthday_sim_day = day - 365*age;
    // adjust for leap years:
    birthday_sim_day -= (age/4);
    // pick a random birthday in the previous year
    birthday_sim_day -= IRAND(1,365);
  }

  if (Global::Enable_Population_Dynamics) {

    // will this person die in the next year?
    double age_specific_probability_of_death = 0.0;
    if (Demographics::MAX_AGE <= age) {
      age_specific_probability_of_death = 1.0;
      // printf("DAY %d DEATH BY MAX_AGE RULE\n", day);
    }
    /*
    else if (self->is_nursing_home_resident()) {
      age_specific_probability_of_death = 0.25;
    }
    */
    else {
      // look up mortality in the mortality rate tables
      if (this->sex == 'F') {
	age_specific_probability_of_death = Demographics::adjusted_female_mortality_rate[age];
      }
      else {
	age_specific_probability_of_death = Demographics::adjusted_male_mortality_rate[age];
      }
    }

    if ( RANDOM() <= age_specific_probability_of_death ) {
      //Yes, so set the death day (in simulation days)
      this->deceased_sim_day = (day + IRAND(1,364));
      Demographics::add_mortality_event(deceased_sim_day, self);
      FRED_STATUS(0, "MORTALITY EVENT ADDDED today %d id %d age %d decease %d\n",
		  day, self->get_id(),age,deceased_sim_day);
    }

    // set pregnancy status
    if (sex == 'F' && 
	Demographics::MIN_PREGNANCY_AGE <= age &&
	age <= Demographics::MAX_PREGNANCY_AGE && 
	self->lives_in_group_quarters() == false) {

      int current_day_of_year = Date::get_day_of_year(day);
      int days_since_birthday = current_day_of_year - Date::get_day_of_year(birthday_sim_day);
      if (days_since_birthday < 0) {
	days_since_birthday += 365;
      }
      double frac_of_year = (double) days_since_birthday / 366.0;

      if (RANDOM() < frac_of_year*Demographics::pregnancy_rate[age]) {
	// already pregnant
	int length_of_pregnancy = (int) (draw_normal(Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5);
	conception_sim_day = day - IRAND(1,length_of_pregnancy-1);
	maternity_sim_day = conception_sim_day + length_of_pregnancy;
	pregnant = true;
	Demographics::add_maternity_event(maternity_sim_day, self);
	FRED_STATUS(0, "MATERNITY EVENT ADDDED today %d id %d age %d due %d\n",
		    day, self->get_id(),age,maternity_sim_day);
      }
    } // end test for pregnancy
  } // end population_dynamics
  if(Global::Enable_Population_Dynamics) {
    Demographics::add_to_birthday_list(self);
  }
}

Demographics::~Demographics() {
}

int Demographics::get_day_of_year_for_birthday_in_nonleap_year() {
  int day_of_year = Date::get_day_of_year(birthday_sim_day);
  int year = Date::get_year(birthday_sim_day);
  if (Date::is_leap_year(year) && 59 < day_of_year) {
    day_of_year--;
  }
  return day_of_year;
}

void Demographics::cancel_conception( Person * self ) {
  assert(conception_sim_day > -1);
  Demographics::delete_conception_event(conception_sim_day, self);
  FRED_STATUS(0, "CONCEPTION EVENT DELETED\n");
  conception_sim_day = -1;
}

void Demographics::conception_handler( int day, Person * self ) {
  self->get_demographics()->become_pregnant(day, self);
}

void Demographics::become_pregnant( int day, Person * self ) {
  // No pregnancies in group quarters
  if(self->lives_in_group_quarters()) {
    FRED_STATUS(0, "GQ PREVENTS PREGNANCY today %d id %d age %d\n",
		day, self->get_id(), self->get_age());
    conception_sim_day = -1;
    return;
  }
  int length_of_pregnancy = (int) (draw_normal(Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5);
  maternity_sim_day = conception_sim_day + length_of_pregnancy;
  Demographics::add_maternity_event(maternity_sim_day, self);
  FRED_STATUS(0, "MATERNITY EVENT ADDDED today %d id %d age %d due %d\n",
	      day, self->get_id(),age,maternity_sim_day);
  pregnant = true;
  conception_sim_day = -1;
}


void Demographics::cancel_pregnancy( Person * self ) {
  assert(pregnant == true);
  Demographics::delete_maternity_event(maternity_sim_day, self);
  FRED_STATUS(0, "MATERNITY EVENT DELETED\n");
  maternity_sim_day = -1;
  pregnant = false;
}

void Demographics::maternity_handler( int day, Person * self ) {
  // NOTE: This calls Person::give_birth() to create the baby
  self->give_birth(day);
}

void Demographics::update_birth_stats( int day, Person * self ) {
  // NOTE: This is called by Person::give_birth() to update stats.
  // The baby is actually created in Person::give_birth()
  pregnant = false;
  maternity_sim_day = -1;
  number_of_children++;
  Demographics::births_today++;
  Demographics::births_ytd++;
  Demographics::total_births++;
  if(Global::Birthfp != NULL) {
  // report births
    fprintf(Global::Birthfp, "day %d mother %d age %d\n",
	    day, self->get_id(), self->get_age());
    fflush(Global::Birthfp);
  }
}


void Demographics::mortality_handler( int day, Person * self ) {
  self->get_demographics()->die(day, self);
}

void Demographics::die(int day, Person * self) {

  // cancel any planned pregnancy
  if (day <= conception_sim_day) {
    printf("DEATH CANCELS PLANNED CONCEPTION: today %d person %d age %d conception %d\n",
	   day, self->get_id(), age, conception_sim_day);
    cancel_conception(self);
  }

  // cancel any current pregnancy
  if (pregnant) {
    printf("DEATH CANCELS PREGNANCY: today %d person %d age %d due %d\n",
	   day, self->get_id(), age, maternity_sim_day);
    cancel_pregnancy(self);
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
  self->die();
  deceased = true;

  // remove from the birthday lists
  if(Global::Enable_Population_Dynamics) {
    Demographics::delete_from_birthday_list(self);
  }

  // remove from population
  Global::Pop.remove_dead_person_from_population( day, self);
}

void Demographics::birthday( Person * self, int day ) {

  if (Global::Enable_Population_Dynamics == false)
    return;

  FRED_STATUS(2, "birthday entered for person %d with (previous) age %d\n", self->get_id(), self->get_age());
  
  // change age
  age++;

  // will this person die in the next year?
  double age_specific_probability_of_death = 0.0;
  if (Demographics::MAX_AGE <= age) {
    age_specific_probability_of_death = 1.0;
    // printf("DAY %d DEATH BY MAX_AGE RULE\n", day);
  }
  /*
  else if (self->is_nursing_home_resident()) {
    age_specific_probability_of_death = 0.25;
  }
  */
  else {
    // look up mortality in the mortality rate tables
    if (this->sex == 'F') {
      age_specific_probability_of_death = Demographics::adjusted_female_mortality_rate[age];
    }
    else {
      age_specific_probability_of_death = Demographics::adjusted_male_mortality_rate[age];
    }
  }
  if ( this->deceased == false && this->deceased_sim_day == -1 &&
       RANDOM() <= age_specific_probability_of_death) {
    
    // Yes, so set the death day (in simulation days)
    this->deceased_sim_day = (day + IRAND(0,364));
    Demographics::add_mortality_event(deceased_sim_day, self);
    FRED_STATUS(0, "MORTALITY EVENT ADDDED today %d id %d age %d decease %d\n",
		day, self->get_id(),age,deceased_sim_day);
  }
  else {
    FRED_STATUS(2, "SURVIVER: AGE %d deceased_sim_day = %d\n", age, this->deceased_sim_day);
  }
  
  // Will this person conceive in the coming year year?
  if (sex == 'F' && 
      Demographics::MIN_PREGNANCY_AGE <= age &&
      age <= Demographics::MAX_PREGNANCY_AGE && 
      conception_sim_day == -1 && maternity_sim_day == -1 &&
      self->lives_in_group_quarters() == false &&
      RANDOM() < Demographics::pregnancy_rate[age]) {
    
    assert(pregnant == false);
    
    // ignore small distortion due to leap years
    conception_sim_day = day + IRAND( 1, 365 );
    Demographics::add_conception_event(conception_sim_day, self);
    FRED_STATUS(0, "CONCEPTION EVENT ADDDED today %d id %d age %d conceive %d house %s\n",
		day, self->get_id(),age,conception_sim_day,self->get_household()->get_label());
  }

  // become responsible for health decisions when reaching adulthood
  if ( age == Global::ADULT_AGE && self->is_health_decision_maker() == false) {
    FRED_STATUS( 2, "become_health_decision_maker\n");
    self->become_health_decision_maker(self);
  }
  
  FRED_STATUS( 2, "birthday finished for person %d with new age %d\n", self->get_id(), self->get_age());
}

void Demographics::print() {
}

double Demographics::get_real_age() const {
  return double( Global::Simulation_Day - birthday_sim_day ) / 365.25;
}

//////// Static Methods

vector <int> fips_codes;

int find_fips_code(int n) {
  int size = fips_codes.size();
  for (int i = 0; i < size; i++) {
    if (fips_codes[i] == n) {
      return i;
    }
  }
  return -1;
}

void Demographics::initialize_static_variables() {

  if ( Demographics::is_initialized ) { return; }
  Demographics::is_initialized = true;

  if (Global::Enable_Population_Dynamics == false)
    return;

  char mortality_rate_file[FRED_STRING_SIZE];
  char birth_rate_file[FRED_STRING_SIZE];

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "Demographics::initialize_static_variables() entered\n");
    fflush(Global::Statusfp);
  }

  Params::get_param_from_string("control_population_growth_rate", &(Demographics::control_population_growth_rate));
  Params::get_param_from_string("population_growth_rate", &(Demographics::population_growth_rate));
  Params::get_param_from_string("birth_rate_file", birth_rate_file);
  Params::get_param_from_string("mortality_rate_file", mortality_rate_file);
  Params::get_param_from_string("college_departure_rate", &(Demographics::college_departure_rate));
  Params::get_param_from_string("military_departure_rate", &(Demographics::military_departure_rate));
  Params::get_param_from_string("prison_departure_rate", &(Demographics::prison_departure_rate));
  Params::get_param_from_string("youth_home_departure_rate", &(Demographics::youth_home_departure_rate));
  Params::get_param_from_string("adult_home_departure_rate", &(Demographics::adult_home_departure_rate));
  
  // initialize the birth rate array
  for (int j = 0; j <= Demographics::MAX_AGE; j++) {
    Demographics::birth_rate[j] = 0.0;
  }

  // read the birth rates
  FILE *fp = NULL;
  fp = Utils::fred_open_file(birth_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Demographics birth_rate_file %s not found\n", birth_rate_file);
    exit(1);
  }
  int age;
  double rate;
  while (fscanf(fp, "%d %lf", &age, &rate) == 2) {
    if (age >= 0 && age <= Demographics::MAX_AGE) {
      birth_rate[age] = rate;
    }
  }
  fclose(fp);
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "finished reading birth_rate_file = %s\n", birth_rate_file);
    fflush(Global::Statusfp);
  }

  // set up pregnancy rates
  for (int i = 0; i < Demographics::MAX_AGE; i++) {
    // approx 3/4 of pregnancies deliver at the next age of the mother
    Demographics::pregnancy_rate[i] =
      0.25 * Demographics::birth_rate[i] + 0.75 * Demographics::birth_rate[i+1];
  }
  Demographics::pregnancy_rate[Demographics::MAX_AGE] = 0.0;

  if (Global::Verbose) {
    for (int i = 0; i <= Demographics::MAX_AGE; i++) {
      fprintf(Global::Statusfp, "BIRTH RATE     for age %d %e\n", i, birth_rate[i]);
      fprintf(Global::Statusfp, "PREGNANCY RATE for age %d %e\n", i, pregnancy_rate[i]);
    }
  }

  // read mortality rate file
  fp = Utils::fred_open_file(mortality_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Demographic mortality_rate %s not found\n", mortality_rate_file);
    exit(1);
  }
  for (int i = 0; i <= Demographics::MAX_AGE; i++) {
    int age;
    double female_rate;
    double male_rate;
    if (fscanf(fp, "%d %lf %lf", &age, &female_rate, &male_rate) != 3) {
      Utils::fred_abort("Help! Read failure for age %d\n", i); 
    }
    if (Global::Verbose) {
      fprintf(Global::Statusfp, "MORTALITY RATE for age %d: female: %e male: %e\n", age, female_rate, male_rate);
    }
    Demographics::female_mortality_rate[i] = female_rate;
    Demographics::male_mortality_rate[i] = male_rate;
  }
  fclose(fp);
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "finished reading mortality_rate_file = %s\n", mortality_rate_file);
    fflush(Global::Statusfp);
  }

  for (int i = 0; i <= Demographics::MAX_AGE; i++) {
    Demographics::adjusted_female_mortality_rate[i] = Demographics::female_mortality_rate[i];
    Demographics::adjusted_male_mortality_rate[i] = Demographics::male_mortality_rate[i];
  }

  // clear birthday lists
  Demographics::birthday_map.clear();
  for(int i = 0; i < 367; ++i) {
    Demographics::birthday_vecs[i].reserve(100);
    Demographics::birthday_vecs[i].clear();
  }

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "Demographics::initialize_static_vars finished\n"); fflush(Global::Statusfp);
  }
}

int count_by_age[Demographics::MAX_AGE+1];

void Demographics::update(int day) {

  // reset counts of births and deaths
  Demographics::births_today = 0;
  Demographics::deaths_today = 0;
  if(Date::get_month() == 1 && Date::get_day_of_month() == 1) {
    Demographics::births_ytd = 0;
    Demographics::deaths_ytd = 0;
  }

  Demographics::update_people_on_birthday_list(day);

  // initiate pregnancies
  Demographics::conception_queue->event_handler(day, Demographics::conception_handler);

  // add newborns to the population
  Demographics::maternity_queue->event_handler(day, Demographics::maternity_handler);

  // remove dead from population
  Demographics::mortality_queue->event_handler(day, Demographics::mortality_handler);

  // update housing periodically
  if(day == 0 || (Date::get_month() == 6 && Date::get_day_of_month() == 30)) {
    Demographics::update_housing(day);
  }

  // update birth and death rates at end of each year
  if(Date::get_month() == 12 && Date::get_day_of_month() == 31) {
    Demographics::update_population_dynamics(day);
  }

}

void Demographics::update_population_dynamics(int day) {

  int births = Demographics::births_ytd;
  int deaths = Demographics::deaths_ytd;
  // adjust for partial first year
  if(day < 364) {
    births *= 365.0 / (day + 1);
    deaths *= 365.0 / (day + 1);
  }

  // get the current popsize
  int current_popsize = Global::Pop.get_pop_size();
  assert(current_popsize > 0);

  // get the current year
  int year = Date::get_year();

  // get the target population size for the end of the coming year
  Demographics::target_popsize = (1.0 + 0.01 * Demographics::population_growth_rate)
      * Demographics::target_popsize;

  // growth rate needed to hit the target for this year
  double target_growth_rate = (100.0 * (Demographics::target_popsize - current_popsize)) / current_popsize;

  // get current age distribution
  int count_males_by_age[Demographics::MAX_AGE + 1];
  int count_females_by_age[Demographics::MAX_AGE + 1];
  Global::Pop.get_age_distribution(count_males_by_age, count_females_by_age);

  // get total age distribution
  for (int a = 0; a <= Demographics::MAX_AGE; a++) {
    count_by_age[a] = count_males_by_age[a] + count_females_by_age[a];
  }

  // compute projected number of births for coming year
  double projected_births = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_births += Demographics::birth_rate[i] * count_females_by_age[i];
  }
  // compute projected number of deaths for coming year
  double projected_deaths = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_deaths += Demographics::male_mortality_rate[i] * count_males_by_age[i];
    projected_deaths += Demographics::female_mortality_rate[i] * count_females_by_age[i];
  }

  // projected population size
  int projected_popsize = current_popsize + projected_births - projected_deaths;

  // consider results of varying birth and death rates by up to 100%:
  double max_projected_births = 2.0 * projected_births;
  double max_projected_deaths = 2.0 * projected_deaths;
  double max_projected_popsize = current_popsize + max_projected_births;
  double min_projected_popsize = current_popsize - max_projected_deaths;
  double projected_range = max_projected_popsize - min_projected_popsize;

  // find the adjustment level that will achieve the target population, if possible.
  // Note: a value of 0.5 will leave birth and death rates as they are.
  double adjustment_factor = 0.5;
  if (Demographics::control_population_growth_rate == 1 && projected_range > 0) {
    adjustment_factor = (Demographics::target_popsize - min_projected_popsize) / projected_range;
  }

  // adjustment_factor = 1 means that births are doubled and deaths are eliminated.
  // the resulting population size will be max_projected_popsize.
  if (adjustment_factor > 1.0) adjustment_factor = 1.0;

  // adjustment_factor = 0 means that deaths are doubled and births are eliminated.
  // the resulting population size will be min_projected_popsize.
  if (adjustment_factor < 0.0) adjustment_factor = 0.0;

  // set birth_rate_adjustment to between 0 and 2, increasing linearly with adjustment_factor
  double birth_rate_adjustment = 2.0 * adjustment_factor;
 
  // set death_rate_adjustment to between 2 and 0, decreasing linearly with adjustment_factor
  double death_rate_adjustment = 2.0 * (1.0 - adjustment_factor);

  for (int i = 0; i <= Demographics::MAX_AGE; i++) {
    Demographics::adjusted_female_mortality_rate[i] = death_rate_adjustment * Demographics::female_mortality_rate[i];
    Demographics::adjusted_male_mortality_rate[i] = death_rate_adjustment * Demographics::male_mortality_rate[i];
    Demographics::adjusted_birth_rate[i] = birth_rate_adjustment * Demographics::birth_rate[i];
  }

  // adjust pregnancy rates
  for (int i = 0; i < Demographics::MAX_AGE; i++) {
    // approx 3/4 of pregnancies deliver at the next age of the mother
    Demographics::pregnancy_rate[i] =
      0.25 * Demographics::adjusted_birth_rate[i] + 0.75 * Demographics::adjusted_birth_rate[i+1];
  }
  Demographics::pregnancy_rate[Demographics::MAX_AGE] = 0.0;

  // projected number of births for coming year
  projected_births = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_births += Demographics::adjusted_birth_rate[i] * count_females_by_age[i];
  }

  // projected number of deaths for coming year
  projected_deaths = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_deaths += Demographics::adjusted_male_mortality_rate[i] * count_males_by_age[i];
    projected_deaths += Demographics::adjusted_female_mortality_rate[i] * count_females_by_age[i];
  }

  // projected population size
  projected_popsize = current_popsize + projected_births - projected_deaths;

  FRED_VERBOSE(0,
	       "POPULATION_DYNAMICS: year %d popsize %d births = %d deaths = %d target = %d exp_popsize = %d exp_births = %0.0f  exp_deaths = %0.0f birth_adj %0.3f death_adj %0.3f\n",
	       year, current_popsize, births, deaths, target_popsize,
	       projected_popsize, projected_births, projected_deaths,
	       birth_rate_adjustment, death_rate_adjustment); 

  // reset counters for coming year:
  births = 0;
  deaths = 0;
}

void Demographics::get_housing_imbalance(int day) {
  Global::Places.get_housing_data(beds, occupants);
  int imbalance = 0;
  for (int i = 0; i < houses; i++) {
    // skip group quarters
    Place * houseptr = Global::Places.get_household(i);
    if (houseptr->is_group_quarters()) continue;
    imbalance += abs(beds[i] - occupants[i]);
  }
  printf("DAY %d HOUSING: houses = %d, imbalance = %d\n", day, houses, imbalance);
}

int Demographics::fill_vacancies(int day) {
  // move ready_to_moves into underfilled units
  int moved = 0;
  if (ready_to_move.size() > 0) {
    // first focus on the empty units
    for (int newhouse = 0; newhouse < houses; newhouse++) {
      if (occupants[newhouse] > 0) continue;
      int vacancies = beds[newhouse] - occupants[newhouse];
      if (vacancies > 0) {
	Place * houseptr = Global::Places.get_household(newhouse);
	// skip group quarters
	if (houseptr->is_group_quarters()) continue;
	for (int j = 0; (j < vacancies) && (ready_to_move.size() > 0); j++) {
	  Person * person = ready_to_move.back().first;
	  int oldhouse = ready_to_move.back().second;
	  Place * ohouseptr = Global::Places.get_household(oldhouse);
	  ready_to_move.pop_back();
	  if (ohouseptr->is_group_quarters() || (occupants[oldhouse]-beds[oldhouse] > 0)) {
	    // move person to new home
	    person->move_to_new_house(houseptr);
	    occupants[oldhouse]--;
	    occupants[newhouse]++;
	    moved++;
	  }
	}
      }
    }

    // now consider any vacancy
    for (int newhouse = 0; newhouse < houses; newhouse++) {
      int vacancies = beds[newhouse] - occupants[newhouse];
      if (vacancies > 0) {
	Place * houseptr = Global::Places.get_household(newhouse);
	// skip group quarters
	if (houseptr->is_group_quarters()) continue;
	for (int j = 0; (j < vacancies) && (ready_to_move.size() > 0); j++) {
	  Person * person = ready_to_move.back().first;
	  int oldhouse = ready_to_move.back().second;
	  Place * ohouseptr = Global::Places.get_household(oldhouse);
	  ready_to_move.pop_back();
	  if (ohouseptr->is_group_quarters() || (occupants[oldhouse]-beds[oldhouse] > 0)) {
	    // move person to new home
	    person->move_to_new_house(houseptr);
	    occupants[oldhouse]--;
	    occupants[newhouse]++;
	    moved++;
	  }
	}
      }
    }
  }
  return moved;
}

void Demographics::update_housing(int day) {

  if (day == 0) {
    // initialize house data structures
    Demographics::houses = Global::Places.get_number_of_households();
    Demographics::beds = new int[houses];
    Demographics::occupants = new int[houses];
    Demographics::target_popsize = Global::Pop.get_pop_size();
  }

  // reserve ready_to_move vector:
  ready_to_move.reserve(Global::Pop.get_pop_size());

  Global::Places.get_housing_data(beds, occupants);
  max_beds = -1;
  max_occupants = -1;
  for (int i = 0; i < houses; i++) {
    if (beds[i] > max_beds) max_beds = beds[i];
    if (occupants[i] > max_occupants) max_occupants = occupants[i];
  }
  // printf("DAY %d HOUSING: houses = %d, max_beds = %d max_occupants = %d\n",
  // day, houses, max_beds, max_occupants);
  // printf("BEFORE "); get_housing_imbalance(day);

  move_college_students_out_of_dorms(day);
  // get_housing_imbalance(day);

  move_college_students_into_dorms(day);
  // get_housing_imbalance(day);

  move_military_personnel_out_of_barracks(day);
  // get_housing_imbalance(day);

  move_military_personnel_into_barracks(day);
  // get_housing_imbalance(day);

  move_inmates_out_of_prisons(day);
  // get_housing_imbalance(day);

  move_inmates_into_prisons(day);
  // get_housing_imbalance(day);

  move_patients_into_nursing_homes(day);
  // get_housing_imbalance(day);

  move_young_adults(day);
  // get_housing_imbalance(day);

  move_older_adults(day);
  // get_housing_imbalance(day);

  swap_houses(day);
  get_housing_imbalance(day);

  Global::Places.report_household_distributions();
  Global::Places.report_school_distributions(day);
  return;
}

void Demographics::move_college_students_out_of_dorms(int day) {
  // printf("MOVE FORMER COLLEGE RESIDENTS =======================\n");
  ready_to_move.clear();
  int college = 0;
  // find students ready to move off campus
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_college()) {
      int hsize = house->get_size();
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN COLLEGE DORM %s\n", person->get_id(), house->get_label());
	assert(person->is_college_dorm_resident());
	college++;
	// some college students leave each year
	if (RANDOM() < Demographics::college_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d COLLEGE\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  // printf("DAY %d MOVED %d COLLEGE STUDENTS\n",day,moved);
  // printf("DAY %d COLLEGE COUNT AFTER DEPARTURES %d\n", day, college-moved); fflush(stdout);
}

void Demographics::move_college_students_into_dorms(int day) {
  // printf("GENERATE NEW COLLEGE RESIDENTS =======================\n");
  ready_to_move.clear();
  int moved = 0;
  int college = 0;

  // find vacant doom rooms
  std::vector<int>dorm_rooms;
  dorm_rooms.clear();
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_college()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; j++) { dorm_rooms.push_back(i); }
      college += house->get_size();
    }
  }
  int dorm_vacancies = (int)dorm_rooms.size();
  // printf("COLLEGE COUNT %d VACANCIES %d\n", college, dorm_vacancies);
  if (dorm_vacancies == 0) {
    printf("NO COLLEGE VACANCIES FOUND\n");
    return;
  }

  // find students to fill the dorms
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if (hsize <= house->get_orig_size()) continue;
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	int age = person->get_age();
	if (18 < age && age < 40 && person->get_number_of_children() == 0) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("COLLEGE APPLICANTS %d\n", (int)ready_to_move.size());

  if (ready_to_move.size() == 0) {
    printf("NO COLLEGE APPLICANTS FOUND\n");
    return;
  }

  // shuffle the applicants
  FYShuffle< pair<Person *, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for (int i = 0; i < dorm_vacancies && ready_to_move.size() > 0; i++) {
    int newhouse = dorm_rooms[i];
    Place * houseptr = Global::Places.get_household(newhouse);
    /*
      printf("VACANT DORM %s ORIG %d SIZE %d\n", houseptr->get_label(),
      houseptr->get_orig_size(),houseptr->get_size());
    */
    Person * person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place * ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    /*
      printf("APPLICANT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
      person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
      ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    */
    person->move_to_new_house(houseptr);
    occupants[oldhouse]--;
    occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ACCEPTED %d COLLEGE STUDENTS, CURRENT = %d  MAX = %d\n",day,moved, college+moved, college+dorm_vacancies);
}

void Demographics::move_military_personnel_out_of_barracks(int day) {
  // printf("MOVE FORMER MILITARY =======================\n");
  ready_to_move.clear();
  int military = 0;
  // find military personnel to discharge
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_military_base()) {
      int hsize = house->get_size();
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN MILITARY BARRACKS %s\n", person->get_id(), house->get_label());
	assert(person->is_military_base_resident());
	military++;
	// some military leave each each year
	if (RANDOM() < Demographics::military_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d FORMER MILITARY\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  // printf("DAY %d RELEASED %d MILITARY, TOTAL NOW %d\n",day,moved,military-moved);
}

void Demographics::move_military_personnel_into_barracks(int day) {
  // printf("GENERATE NEW MILITARY BASE RESIDENTS =======================\n");
  ready_to_move.clear();
  int moved = 0;
  int military = 0;

  // find unfilled barracks units
  std::vector<int>barracks_units;
  barracks_units.clear();
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_military_base()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; j++) { barracks_units.push_back(i); }
      military += house->get_size();
    }
  }
  int barracks_vacancies = (int)barracks_units.size();
  // printf("MILITARY VACANCIES %d\n", barracks_vacancies);
  if (barracks_vacancies == 0) {
    printf("NO MILITARY VACANCIES FOUND\n");
    return;
  }

  // find recruits to fill the dorms
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if (hsize <= house->get_orig_size()) continue;
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	int age = person->get_age();
	if (18 < age && age < 40 && person->get_number_of_children() == 0) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("MILITARY RECRUITS %d\n", (int)ready_to_move.size());

  if (ready_to_move.size() == 0) {
    printf("NO MILITARY RECRUITS FOUND\n");
    return;
  }

  // shuffle the recruits
  FYShuffle< pair<Person *, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for (int i = 0; i < barracks_vacancies && ready_to_move.size() > 0; i++) {
    int newhouse = barracks_units[i];
    Place * houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED BARRACKS %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person * person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place * ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    // printf("RECRUIT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    occupants[oldhouse]--;
    occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d MILITARY, CURRENT = %d  MAX = %d\n",day,moved,military+moved,military+barracks_vacancies);
}

void Demographics::move_inmates_out_of_prisons(int day) {
  // printf("RELEASE PRISONERS =======================\n");
  ready_to_move.clear();
  int prisoners = 0;
  // find former prisoners still in jail
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_prison()) {
      int hsize = house->get_size();
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	// printf("PERSON %d LIVES IN PRISON %s\n", person->get_id(), house->get_label());
	assert(person->is_prisoner());
	prisoners++;
	// some prisoners get out each year
	if (RANDOM() < Demographics::prison_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE %d FORMER PRISONERS\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("DAY %d RELEASED %d PRISONERS, TOTAL NOW %d\n",day,moved,prisoners-moved);
}

void Demographics::move_inmates_into_prisons(int day) {
  // printf("GENERATE NEW PRISON RESIDENTS =======================\n");
  ready_to_move.clear();
  int moved = 0;
  int prisoners = 0;

  // find unfilled jail_cell units
  std::vector<int>jail_cell_units;
  jail_cell_units.clear();
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_prison()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; j++) { jail_cell_units.push_back(i); }
      prisoners += house->get_size();
    }
  }
  int jail_cell_vacancies = (int)jail_cell_units.size();
  // printf("PRISON VACANCIES %d\n", jail_cell_vacancies);
  if (jail_cell_vacancies == 0) {
    printf("NO PRISON VACANCIES FOUND\n");
    return;
  }

  // find inmates to fill the jail_cells
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if (hsize <= house->get_orig_size()) continue;
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	int age = person->get_age();
	if ((18 < age && person->get_number_of_children() == 0) || (age < 50)) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("PRISON POSSIBLE INMATES %d\n", (int)ready_to_move.size());

  if (ready_to_move.size() == 0) {
    printf("NO INMATES FOUND\n");
    return;
  }

  // shuffle the inmates
  FYShuffle< pair<Person *, int> >(ready_to_move);

  // pick the top of the list to move into dorms
  for (int i = 0; i < jail_cell_vacancies && ready_to_move.size() > 0; i++) {
    int newhouse = jail_cell_units[i];
    Place * houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED JAIL_CELL %s ORIG %d SIZE %d\n", houseptr->get_label(),
    // houseptr->get_orig_size(),houseptr->get_size());
    Person * person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place * ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    // printf("INMATE %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
    // person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
    // ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    person->move_to_new_house(houseptr);
    occupants[oldhouse]--;
    occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d PRISONERS, CURRENT = %d  MAX = %d\n",day,moved,prisoners+moved,prisoners+jail_cell_vacancies);
}

void Demographics::move_patients_into_nursing_homes(int day) {
  // printf("NEW NURSING HOME RESIDENTS =======================\n");
  ready_to_move.clear();
  int moved = 0;
  int nursing_home_residents = 0;
  int beds = 0;

  // find unfilled nursing_home units
  std::vector<int>nursing_home_units;
  nursing_home_units.clear();
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_nursing_home()) {
      int vacancies = house->get_orig_size() - house->get_size();
      for (int j = 0; j < vacancies; j++) { nursing_home_units.push_back(i); }
      nursing_home_residents += house->get_size();;
      beds += house->get_orig_size();
    }
  }
  int nursing_home_vacancies = (int)nursing_home_units.size();
  // printf("NURSING HOME VACANCIES %d\n", nursing_home_vacancies);
  if (nursing_home_vacancies == 0) {
    printf("DAY %d ADDED %d NURSING HOME PATIENTS, TOTAL NOW %d BEDS = %d\n",day,0,nursing_home_residents,beds);
    return;
  }

  // find patients to fill the nursing_homes
  for (int i = 0; i < houses; i++) {
    Household * house = Global::Places.get_household_ptr(i);
    if (house->is_group_quarters()==false) {
      int hsize = house->get_size();
      if (hsize <= house->get_orig_size()) continue;
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	int age = person->get_age();
	if (60 <= age) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("NURSING HOME POSSIBLE PATIENTS %d\n", (int)ready_to_move.size());

  // shuffle the patients
  FYShuffle< pair<Person *, int> >(ready_to_move);

  // pick the top of the list to move into nursing_home
  for (int i = 0; i < nursing_home_vacancies && ready_to_move.size() > 0; i++) {
    int newhouse = nursing_home_units[i];
    Place * houseptr = Global::Places.get_household(newhouse);
    // printf("UNFILLED NURSING_HOME UNIT %s ORIG %d SIZE %d\n", houseptr->get_label(),houseptr->get_orig_size(),houseptr->get_size());
    Person * person = ready_to_move.back().first;
    int oldhouse = ready_to_move.back().second;
    Place * ohouseptr = Global::Places.get_household(oldhouse);
    ready_to_move.pop_back();
    // move person to new home
    /*
    printf("PATIENT %d SEX %c AGE %d HOUSE %s SIZE %d ORIG %d PROFILE %c\n",
	   person->get_id(),person->get_sex(),person->get_age(),ohouseptr->get_label(),
	   ohouseptr->get_size(),ohouseptr->get_orig_size(),person->get_profile());
    */
    person->move_to_new_house(houseptr);
    occupants[oldhouse]--;
    occupants[newhouse]++;
    moved++;
  }
  printf("DAY %d ADDED %d NURSING HOME PATIENTS, CURRENT = %d  MAX = %d\n",day,moved,nursing_home_residents+moved,beds);
}

void Demographics::move_young_adults(int day) {
  // printf("MOVE YOUNG ADULTS =======================\n");
  ready_to_move.clear();

  // find young adults in overfilled units who are ready to move out
  for (int i = 0; i < houses; i++) {
    if (occupants[i] > beds[i]) {
      Household * house = Global::Places.get_household_ptr(i);
      int hsize = house->get_size();
      for (int j = 0; j < hsize; j++) {
	Person * person = house->get_housemate(j);
	int age = person->get_age();
	if (18 <= age && age < 30) {
	  if (RANDOM() < Demographics::youth_home_departure_rate) {
	    ready_to_move.push_back(make_pair(person,i));
	    // printf("READY: PERSON %d AGE %d OLDHOUSE %d\n", person->get_id(), person->get_age(),i);
	  }
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE young adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("MOVED %d YOUNG ADULTS =======================\n",moved);
}


void Demographics::move_older_adults(int day) {
  // printf("MOVE OLDER ADULTS =======================\n");
  ready_to_move.clear();

  // find older adults in overfilled units
  for (int i = 0; i < houses; i++) {
    int excess = occupants[i] - beds[i];
    if (excess > 0) {
      Household * house = Global::Places.get_household_ptr(i);
      // find the oldest person in the house
      int hsize = house->get_size();
      int max_age = -1;
      int pos = -1;
      int adults = 0;
      for (int j = 0; j < hsize; j++) {
	int age = house->get_housemate(j)->get_age();
	if (age > max_age) {
	  max_age = age; pos = j;
	}
	if (age > 20) adults++;
      }
      if (adults > 1) {
	Person * person = house->get_housemate(pos);
	if (RANDOM() < Demographics::adult_home_departure_rate) {
	  ready_to_move.push_back(make_pair(person,i));
	}
      }
    }
  }
  // printf("DAY %d READY TO MOVE older adults = %d\n", day, (int) ready_to_move.size());
  int moved = fill_vacancies(day);
  printf("MOVED %d OLDER ADULTS =======================\n",moved);
}

void Demographics::report_ages(int day, int house_id) {
  Household * house = Global::Places.get_household_ptr(house_id);
  printf("HOUSE %d BEDS %d OCC %d AGES ",
	 house->get_id(), beds[house_id], occupants[house_id]);
  int hsize = house->get_size();
  for (int j = 0; j < hsize; j++) {
    int age = house->get_housemate(j)->get_age();
    printf("%d ", age);
  }
}


// 2-d array of lists
typedef std::vector<int> HouselistT;

void Demographics::swap_houses(int day) {
  printf("SWAP HOUSES =======================\n");

  // two-dim array of vectors of imbalanced houses
  HouselistT ** houselist;
  houselist = new HouselistT * [13];
  for (int i = 0; i < 13; i++) {
    houselist[i] = new HouselistT [13];
    for (int j = 0; j < 13; j++) {
      houselist[i][j].clear();
    }
  }
  for (int i = 0; i < houses; i++) {
    // skip group quarters
    if (Global::Places.get_household(i)->is_group_quarters()) continue;
    int b = beds[i];
    if (b > 12) b = 12;
    int occ = occupants[i];
    if (occ > 12) occ = 12;
    if (b != occ)
      houselist[b][occ].push_back(i);
  }

  // find complementary pairs (beds=i,occ=j) and (beds=j,occ=i)
  for (int i = 1; i < 10; i++) {
    for (int j = i+1; j < 10; j++) {
      while (houselist[i][j].size() > 0 && houselist[j][i].size() > 0) {
	int hi = houselist[i][j].back();
	houselist[i][j].pop_back();
	int hj = houselist[j][i].back();
	houselist[j][i].pop_back();
	// swap houses hi and hj
	// printf("SWAPPING: "); report_ages(day,hi); report_ages(day,hj); printf("\n");
	Global::Places.swap_houses(hi, hj);
	occupants[hi] = i;
	occupants[hj] = j;
	// printf("AFTER:\n"); report_ages(day,hi); report_ages(day,hj);
      }
    }
  }

  return; 

  // refill-vectors
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      houselist[i][j].clear();
    }
  }
  for (int i = 0; i < houses; i++) {
    int b = beds[i];
    if (b > 9) b = 9;
    int occ = occupants[i];
    if (occ > 9) occ = 9;
    if (b > 0 && b != occ)
      houselist[b][occ].push_back(i);
  }

  /*
  // find complementary pairs (beds=B,occ=o) and (beds=b,occ=O) where o<=b and O <= B
  for (int bd = 0; bd < 10; bd++) {
    for (int oc = bd+1; oc < 10; oc++) {
      // this household needs more beds
      while (houselist[bd][oc].size() > 0) {
	int h1 = houselist[bd][oc].back();
	houselist[bd][oc].pop_back();
	int swapper = 0;
	for (int Bd = oc; (swapper == 0) && Bd < 10; Bd++) {
	  for (int Oc = bd; (swapper == 0) && 0 <= Oc; Oc--) {
	    // this house has enough beds for the first house
	    // and needs at most as many beds as are in the first house
	    if (houselist[Bd][Oc].size() > 0) {
	      int h2 = houselist[Bd][Oc].back();
	      houselist[Bd][Oc].pop_back();
	      // swap houses h1 and h2
	      printf("SWAPPING II: "); report_ages(day,h1); report_ages(day,h2); printf("\n");
	      Global::Places.swap_houses(h1, h2);
	      occupants[h1] = Oc;
	      occupants[h2] = oc;
	      // printf("AFTER:\n"); report_ages(day,h1); report_ages(day,h1);
	      swapper = 1;
	    }
	  }
	}
      }
    }
  }
  */
  // get_housing_imbalance(day);

  // NOTE: This can be simplified using swap houses matrix

  for (int i = 0; i < houses; i++) {
    if (beds[i] == 0) continue;
    int diff = occupants[i] - beds[i];
    if(diff < -1 || diff > 1) {
      // take a look at this house
      Household * house = Global::Places.get_household_ptr(i);
      printf("DAY %d PROBLEM HOUSE %d BEDS %d OCC %d AGES ",
	     day, house->get_id(), beds[i], occupants[i]);
      int hsize = house->get_size();
      for (int j = 0; j < hsize; j++) {
	int age = house->get_housemate(j)->get_age();
	printf("%d ", age);
      }
      printf("\n");
    }
  }

  // make lists of overfilled houses
  vector<int> * overfilled;
  overfilled = new vector<int> [max_beds+1];
  for(int i = 0; i <= max_beds; i++) {
    overfilled[i].clear();
  }
  
  // make lists of underfilled houses
  vector<int> * underfilled;
  underfilled = new vector<int> [max_beds + 1];
  for(int i = 0; i <= max_beds; i++) {
    underfilled[i].clear();
  }

  for (int i = 0; i < houses; i++) {
    if (beds[i] == 0) continue;
    int diff = occupants[i] - beds[i];
    if(diff > 0) {
      overfilled[beds[i]].push_back(i);
    }
    if (diff < 0) {
      underfilled[beds[i]].push_back(i);
    }
  }

  int count[100];
  for(int i = 0; i <= max_beds; i++) {
    for (int j = 0; j <= max_beds+1; j++) {
      count[j] = 0;
    }
    for (int k = 0; k < (int) overfilled[i].size(); k++) {
      int kk = overfilled[i][k];
      int occ = occupants[kk];
      if (occ <= max_beds+1)
	count[occ]++;
      else
	count[max_beds+1]++;
    }
    for (int k = 0; k < (int) underfilled[i].size(); k++) {
      int kk = underfilled[i][k];
      int occ = occupants[kk];
      if (occ <= max_beds+1)
	count[occ]++;
      else
	count[max_beds+1]++;
    }
    printf("DAY %4d BEDS %2d ", day, i);
    for (int j = 0; j <= max_beds+1; j++) {
      printf("%3d ", count[j]);
    }
    printf("\n");
    fflush(stdout);
  }

}

void Demographics::add_to_birthday_list(Person * person) {
  int day_of_year = person->get_demographics()->get_day_of_year_for_birthday_in_nonleap_year();
  Demographics::birthday_vecs[day_of_year].push_back(person);
  FRED_VERBOSE(2,
	       "Adding person %d to birthday vector for day = %d.\n ... birthday_vecs[ %d ].size() = %zu\n",
	       person->get_id(), day_of_year, day_of_year, Demographics::birthday_vecs[ day_of_year ].size());
  Demographics::birthday_map[person] = ((int)Demographics::birthday_vecs[day_of_year].size() - 1);
}

void Demographics::delete_from_birthday_list(Person * person) {
  int day_of_year = person->get_demographics()->get_day_of_year_for_birthday_in_nonleap_year();
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
  birthday_map[last] = pos;

  // delete last slot
  Demographics::birthday_vecs[day_of_year].pop_back();

  // delete from birthday map
  Demographics::birthday_map.erase(itr);
}

void Demographics::update_people_on_birthday_list(int day) {
  int birthday_index = Date::get_day_of_year();
  if (Date::is_leap_year()) {
    // skip feb 29
    if (birthday_index == 60) {
      return;
    }
    // shift all days after feb 28 forward
    if (60 < birthday_index) {
      birthday_index--;
    }
  }

  // update everyone on today birthday list
  int birthday_count = 0;
  int size = (int) Demographics::birthday_vecs[birthday_index].size();
  for(int p = 0; p < size; ++p) {
    Demographics::birthday_vecs[birthday_index][p]->birthday(day);
    birthday_count++;
  }
  
  FRED_VERBOSE(0, "update_people_on_birthday_list: index = %d count = %d\n",
	       birthday_index, birthday_count);
}

