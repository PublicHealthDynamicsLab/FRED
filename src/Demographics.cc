/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
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
double Demographics::age_yearly_mortality_rate_male[Demographics::MAX_AGE + 1];
double Demographics::age_yearly_mortality_rate_female[Demographics::MAX_AGE + 1];
double Demographics::age_yearly_birth_rate[Demographics::MAX_PREGNANCY_AGE + 1];
double Demographics::orig_age_yearly_mortality_rate_male[Demographics::MAX_AGE + 1];
double Demographics::orig_age_yearly_mortality_rate_female[Demographics::MAX_AGE + 1];
double Demographics::orig_age_yearly_birth_rate[Demographics::MAX_PREGNANCY_AGE + 1];
int Demographics::target_popsize = 0;
double Demographics::population_growth_rate = 0.0;
double Demographics::max_adjustment_rate = 0.0;
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
int max_beds = -1;
int max_occupants = -1;
std::vector < pair<Person *, int> > ready_to_move;

Demographics::Demographics() {
  this->init_age = -1;
  this->age = -1;
  this->sex = 'n';
  this->birth_day_of_year = -1;
  this->birth_year = -1;
  this->deceased_sim_day = -1;
  this->conception_sim_day = -1;
  this->due_sim_day = -1;
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
    while (RANDOM() < 0.6) _age++;
  }

  // set demographic variables
  init_age            = _age;
  age                 = init_age;
  sex                 = _sex;
  race                = _race;
  relationship        = rel;
  birth_day_of_year   = -1;
  deceased_sim_day    = -1;
  conception_sim_day  = -1;
  due_sim_day         = -1;
  pregnant            = false;
  deceased            = false;
  number_of_children = 0;

  if (is_newborn == false) {
    double eps = std::numeric_limits<double>::epsilon();
    double rand_birthday = URAND(0.0 + eps, 365.0);
    int current_day_of_year = Global::Sim_Current_Date->get_day_of_year();
    int birthyear;

    //If the random birthday selected is less than or equal to the system day then the birthyear is
    // found using system_year - init_age, else it is system_year - (init_age + 1)
    if (((int) ceil(rand_birthday)) <= current_day_of_year) {
      birthyear = Global::Sim_Current_Date->get_year() - init_age;
    } else {
      birthyear = Global::Sim_Current_Date->get_year() - (init_age + 1);
    }

    //If the birthyear would have been a leap year, adjust the random number
    // so that it is based on 366 days
    if (Date::is_leap_year(birthyear)) {
      rand_birthday = rand_birthday * 366.0 / 365.0;
    }

    FRED_CONDITIONAL_VERBOSE(0, birthyear < 1800, "===========================================> \n","");

    Date tmpDate = Date(birthyear, (int) ceil(rand_birthday));
    this->birth_day_of_year = tmpDate.get_day_of_year();
    this->birth_year = birthyear;

  } else {
    this->birth_day_of_year = Global::Sim_Current_Date->get_day_of_year();
    birth_year = Global::Sim_Current_Date->get_year();
  }

  if (Global::Enable_Population_Dynamics) {

    // will this person die in the next year?
    double pct_chance_to_die = 0.0;
    if (Demographics::MAX_AGE <= age) {
      pct_chance_to_die = 1.0;
    }
    else if (self->is_nursing_home_resident()) {
      pct_chance_to_die = 0.25;
    }
    else {
      // look up mortality in the mortality rate tables
      if (this->sex == 'F') {
	pct_chance_to_die = Demographics::age_yearly_mortality_rate_female[age];
      }
      else {
	pct_chance_to_die = Demographics::age_yearly_mortality_rate_male[age];
      }
    }

    if ( RANDOM() <= pct_chance_to_die ) {
      //Yes, so set the death day (in simulation days)
      this->deceased_sim_day = (day + IRAND(1,364));
      Global::Pop.set_mask_by_index( fred::Update_Deaths, self_index );
    }

    //Is this person pregnant?
    if (this->sex == 'F' && age <= Demographics::MAX_PREGNANCY_AGE) {
      if ( ( ( 280.0/365.25 ) * Demographics::age_yearly_birth_rate[ age ] ) > RANDOM() ) {
	//Yes, so set the due_date (in simulation days)
	this->due_sim_day = ( day + IRAND( 1, 280 ) );
	this->pregnant = true;
	Global::Pop.set_mask_by_index( fred::Update_Births, self_index );
	FRED_STATUS( 2, "Birth scheduled during initialization! conception day: %d, delivery day: %d\n",
		     conception_sim_day, due_sim_day );
      }
    }
  }
}


Demographics::~Demographics() {
}

void Demographics::update(int day) {
  // TODO: this does nothing, leave as stub for future extensions
}

void Demographics::clear_pregnancy( Person * self ) {
  conception_sim_day = -1;
  due_sim_day = -1;
  pregnant = false;
  // don't update this person's demographics anymore
  Global::Pop.clear_mask_by_index( fred::Update_Births, self->get_pop_index() );
}


void Demographics::update_births( Person * self, int day ) {
  // Is this your day to give birth? ( previously set in Demographics::birthday )
  if ( pregnant && due_sim_day == day ) {
    clear_pregnancy(self);
    // give birth
    Global::Pop.prepare_to_give_birth( day, self->get_pop_index() );
    number_of_children++;
    Demographics::births_today++;
    Demographics::births_ytd++;
    Demographics::total_births++;
  }
}

void Demographics::update_deaths( Person * self, int day ) {
  //Is this your day to die?
  if ( deceased_sim_day == day ) {
    // don't update this person's demographics anymore
    Global::Pop.clear_mask_by_index( fred::Update_Deaths, self->get_pop_index() );
    deceased = true;
    Global::Pop.prepare_to_die( day, self->get_pop_index() );
    Demographics::deaths_today++;
    Demographics::deaths_ytd++;
    Demographics::total_deaths++;
  }
}

void Demographics::birthday( Person * self, int day ) {

  if (Global::Enable_Population_Dynamics == false)
    return;

  FRED_STATUS(2, "birthday entered for person %d with (previous) age %d\n", self->get_id(), self->get_age());
  
  // change age
  age++;

  // will this person die in the next year?
  double pct_chance_to_die = 0.0;
  if (Demographics::MAX_AGE <= age) {
    pct_chance_to_die = 1.0;
  }
  else {
    // look up mortality in the mortality rate tables
    if (this->sex == 'F') {
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_female[age];
    }
    else {
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_male[age];
    }
  }
  // if (90 <= age) printf("AGE %d PCT %f\n", age, pct_chance_to_die);

  if ( this->deceased == false && this->deceased_sim_day == -1 &&
       RANDOM() <= pct_chance_to_die) {
    
    // Yes, so set the death day (in simulation days)
    this->deceased_sim_day = (day + IRAND(0,364));
    FRED_STATUS(2, "DOOMED: AGE %d set deceased_sim_day = %d\n", age, this->deceased_sim_day);
    
    // and flag for daily updates until death
    Global::Pop.set_mask_by_index( fred::Update_Deaths, self->get_pop_index() );
  }
  else {
    FRED_STATUS(2, "SURVIVER: AGE %d deceased_sim_day = %d\n", age, this->deceased_sim_day);
  }
  
  // Will this person conceive this year ( delivery may occur in the next year )?
  if (sex == 'F' && age <= Demographics::MAX_PREGNANCY_AGE && self->lives_in_group_quarters() == false){
    
    FRED_STATUS(2, "birthday for female id %d age %d preg %d conc %d due %d\n",
		self->get_id(), age, pregnant?1:0, conception_sim_day, due_sim_day);
    
    if ( pregnant == false && conception_sim_day == -1 && due_sim_day == -1 ) {
      
      // ignore small distortion due to leap years
      if (RANDOM() < Demographics::age_yearly_birth_rate[ age ]) {
	
	// ignore small distortion due to leap years
	conception_sim_day = day + IRAND( 1, 365 );
	due_sim_day = conception_sim_day + (int) ( draw_normal(
		       Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5 );
	pregnant = true;
	
	// flag for daily updates
	Global::Pop.set_mask_by_index( fred::Update_Births, self->get_pop_index() );
	
	FRED_STATUS( 2, "Birth scheduled! conception day: %d, delivery day: %d\n",
		     conception_sim_day, due_sim_day );
      }
    }
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
  return double( get_age_in_days() ) / 365.0;
}

int Demographics::get_age_in_days() const {
  if ( birth_year > Date::get_epoch_start_year() ) {
    const Date tmp_date = Date(birth_year, birth_day_of_year);
    return Date::days_between(Global::Sim_Current_Date, & tmp_date);
  }
  FRED_WARNING("WARNING: Birth year (%d) before start of epoch (Date::EPOCH_START_YEAR == %d)!\n",
      birth_year, Date::get_epoch_start_year());
  return -1;
}

//////// Static Methods

void Demographics::initialize_static_variables() {

  if ( Demographics::is_initialized ) { return; }
  Demographics::is_initialized = true;

  if (Global::Enable_Population_Dynamics == false)
    return;

  char yearly_mortality_rate_file[FRED_STRING_SIZE];
  char yearly_birth_rate_file[FRED_STRING_SIZE];
  double birth_rate_multiplier;
  FILE *fp = NULL;

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "Demographics::initialize_static_variables() entered\n");
    fflush(Global::Statusfp);
  }

  Params::get_param_from_string("population_growth_rate", &(Demographics::population_growth_rate));
  Params::get_param_from_string("yearly_birth_rate_file", yearly_birth_rate_file);
  Params::get_param_from_string("birth_rate_multiplier", &birth_rate_multiplier);
  Params::get_param_from_string("yearly_mortality_rate_file", yearly_mortality_rate_file);
  Params::get_param_from_string("max_adjustment_rate", &(Demographics::max_adjustment_rate));
  Params::get_param_from_string("college_departure_rate", &(Demographics::college_departure_rate));
  Params::get_param_from_string("military_departure_rate", &(Demographics::military_departure_rate));
  Params::get_param_from_string("prison_departure_rate", &(Demographics::prison_departure_rate));
  Params::get_param_from_string("youth_home_departure_rate", &(Demographics::youth_home_departure_rate));
  Params::get_param_from_string("adult_home_departure_rate", &(Demographics::adult_home_departure_rate));
  
  // read and load the birth rates
  fp = Utils::fred_open_file(yearly_birth_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Demographics yearly_birth_rate_file %s not found\n", yearly_birth_rate_file);
    exit(1);
  }
  for (int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; i++) {
    int age;
    float rate;
    if (fscanf(fp, "%d %f", &age, &rate) != 2) {
      Utils::fred_abort("Help! Read failure for age %d\n", i); 
    }
    Demographics::age_yearly_birth_rate[i] = birth_rate_multiplier * (double)rate;
    Demographics::orig_age_yearly_birth_rate[i] = Demographics::age_yearly_birth_rate[i]; 
  }
  fclose(fp);
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "finished reading yearly_birth_rate_file = %s\n", yearly_birth_rate_file);
    fflush(Global::Statusfp);
  }

  // read death rate file and load the values unt the death_rate_array
  fp = Utils::fred_open_file(yearly_mortality_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Demographic mortality_rate %s not found\n", yearly_mortality_rate_file);
    exit(1);
  }
  for (int i = 0; i <= Demographics::MAX_AGE; i++) {
    int age;
    float female_rate;
    float male_rate;
    if (fscanf(fp, "%d %f %f", &age, &female_rate, &male_rate) != 3) {
      Utils::fred_abort("Help! Read failure for age %d\n", i); 
    }
    Demographics::age_yearly_mortality_rate_female[i] = (double)female_rate;
    Demographics::orig_age_yearly_mortality_rate_female[i] = Demographics::age_yearly_mortality_rate_female[i];
    Demographics::age_yearly_mortality_rate_male[i] = (double)male_rate;
    Demographics::orig_age_yearly_mortality_rate_male[i] = Demographics::age_yearly_mortality_rate_male[i];
  }
  fclose(fp);
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "finished reading mortality_rate_file = %s\n", yearly_mortality_rate_file);
    fflush(Global::Statusfp);
  }

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "Demographics::initialize_static_vars finished\n"); fflush(Global::Statusfp);
  }
}

int count_by_age[Demographics::MAX_AGE+1];

void Demographics::update_population_dynamics(int day) {

  if (day == 0) {
    // initialize house data structures
    Demographics::houses = Global::Places.get_number_of_households();
    Demographics::beds = new int[houses];
    Demographics::occupants = new int[houses];
  }

  Demographics::births_today = 0;
  Demographics::deaths_today = 0;

  if(Date::simulation_date_matches_pattern("01-01-*")) {
    Demographics::births_ytd = 0;
    Demographics::deaths_ytd = 0;
  }

  if(day == 0 || Date::simulation_date_matches_pattern("06-30-*")) {
    Demographics::update_housing(day);
  }

  if(Date::simulation_date_matches_pattern("12-31-*") == false) {
    return;
  }

  int births = Demographics::births_ytd;
  int deaths = Demographics::deaths_ytd;
  // adjust for partial first year
  if(day < 364) {
    births *= 365.0 / (day + 1);
    deaths *= 365.0 / (day + 1);
  }
  FRED_VERBOSE(0, "POP_DYNAMICS: births_last_year = %d deaths_last_year = %d\n", births, deaths);

  // get the target population size for the end of the coming year
  Demographics::target_popsize = (1.0 + 0.01 * Demographics::population_growth_rate)
      * Demographics::target_popsize;

  // get the current popsize
  int current_popsize = Global::Pop.get_pop_size();
  int year = day / 365;
  printf("YEAR %d POPSIZE %d\n", year, current_popsize); fflush(stdout);
  assert(current_popsize > 0);

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
  for(int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; i++) {
    projected_births += Demographics::orig_age_yearly_birth_rate[i] * count_females_by_age[i];
  }
  double max_projected_births = (1.0 + max_adjustment_rate) * projected_births;
  double min_projected_births = (1.0 - max_adjustment_rate) * projected_births;

  // compute projected number of deaths for coming year
  double projected_deaths = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_deaths += orig_age_yearly_mortality_rate_male[i] * count_males_by_age[i];
    projected_deaths += orig_age_yearly_mortality_rate_female[i] * count_females_by_age[i];
  }
  double max_projected_deaths = (1.0 + max_adjustment_rate) * projected_deaths;
  double min_projected_deaths = (1.0 - max_adjustment_rate) * projected_deaths;

  // projected population size
  int projected_popsize = current_popsize + projected_births - projected_deaths;
  FRED_VERBOSE(1,
	       "POP_DYNAMICS: year %d current = %d  exp_births = %0.1f  exp_deaths = %0.1f  new = %d target = %d\n",
	       year, current_popsize, projected_births, projected_deaths, projected_popsize,
	       Demographics::target_popsize);

  double max_projected_popsize = current_popsize + max_projected_births - min_projected_deaths;
  double min_projected_popsize = current_popsize + min_projected_births - max_projected_deaths;
  double projected_range = max_projected_popsize - min_projected_popsize;
  double adjustment_factor = 1.0;
  if (projected_range > 0)
    adjustment_factor = (Demographics::target_popsize - min_projected_popsize) / projected_range;
  if (adjustment_factor > 1.0) adjustment_factor = 1.0;
  if (adjustment_factor < 0.0) adjustment_factor = 0.0;
  double birth_adjustment = 1.0 + (2.0 * max_adjustment_rate * adjustment_factor)- max_adjustment_rate;
  double death_adjustment = 1.0 - ((2.0 * max_adjustment_rate * adjustment_factor)- max_adjustment_rate);

  // make adjustments as needed:
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    Demographics::age_yearly_mortality_rate_female[i] = death_adjustment
      * Demographics::orig_age_yearly_mortality_rate_female[i];
    Demographics::age_yearly_mortality_rate_male[i] = death_adjustment
      * Demographics::orig_age_yearly_mortality_rate_male[i];
  }
  for(int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; ++i) {
    Demographics::age_yearly_birth_rate[i] = birth_adjustment
      * Demographics::orig_age_yearly_birth_rate[i];
  }
  // projected number of births for coming year
  projected_births = 0;
  for(int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; i++) {
    projected_births += Demographics::age_yearly_birth_rate[i] * count_females_by_age[i];
  }

  // projected number of deaths for coming year
  projected_deaths = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    projected_deaths += age_yearly_mortality_rate_male[i] * count_males_by_age[i];
    projected_deaths += age_yearly_mortality_rate_female[i] * count_females_by_age[i];
  }

  // projected population size
  projected_popsize = current_popsize + projected_births - projected_deaths;
  FRED_VERBOSE(0,
	       "POP DYNAMICS YEAR %d current = %d  exp_births = %0.1f  exp_deaths = %0.1f  projected = %d target = %d ",
	       year, current_popsize, projected_births, projected_deaths, projected_popsize,
	       Demographics::target_popsize);
  FRED_VERBOSE(0, " adjustment factor for births %0.3f and deaths %0.3f\n",
	       birth_adjustment, death_adjustment); 

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

