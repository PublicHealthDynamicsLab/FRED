/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
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

class Global;

const double Demographics::MEAN_PREG_DAYS = 280.0; //40 weeks
const double Demographics::STDDEV_PREG_DAYS = 7.0; //1 week

bool Demographics::is_initialized = false;
double Demographics::age_yearly_mortality_rate_male[Demographics::MAX_AGE + 1];
double Demographics::age_yearly_mortality_rate_female[Demographics::MAX_AGE + 1];
double Demographics::age_yearly_birth_rate[Demographics::MAX_PREGNANCY_AGE + 1];
double Demographics::age_daily_birth_rate[Demographics::MAX_PREGNANCY_AGE + 1];

Demographics::Demographics() {
  //Create the static arrays one time
  //if (!Demographics::is_initialized) {
    //read_init_files();
    //Demographics::is_initialized = true;
  //}
  self = NULL;
  init_age = -1;
  age = -1;
  sex = 'n';
  birth_day_of_year = -1;
  deceased_sim_day = -1;
  conception_sim_day = -1;
  due_sim_day = -1;
  pregnant = false;
  deceased = false;
}

Demographics::Demographics(Person * _self, short int _age, char _sex, 
			   short int _race, short int rel, int day, bool is_newborn) {

  //Create the static arrays one time
  //if (!Demographics::is_initialized) {
    //read_init_files();
    //Demographics::is_initialized = true;
  //}

  // adjust age for those over 89 (due to binning in the synthetic pop)
  if (_age > 89) {
    _age = 90;
    while (RANDOM() < 0.6) _age++;
  }

  // set demographic variables
  self                = _self;
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

    Date * tmpDate = new Date(birthyear, (int) ceil(rand_birthday));
    this->birth_day_of_year = tmpDate->get_day_of_year();
    this->birth_year = birthyear;
    delete tmpDate;

  } else {

    this->birth_day_of_year = Global::Sim_Current_Date->get_day_of_year();
    birth_year = Global::Sim_Current_Date->get_year();
  }

  //Will this person die in the next year?
  if ( Global::Enable_Deaths ) {
    double pct_chance_to_die = 0.0;
    int age_lookup = (age <= Demographics::MAX_AGE ? age : Demographics::MAX_AGE);
    if (this->sex == 'F')
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_female[age_lookup];
    else
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_male[age_lookup];

    if ( RANDOM() <= pct_chance_to_die ) {
      //Yes, so set the death day (in simulation days)
      this->deceased_sim_day = (day + IRAND(1,364));
      Global::Pop.set_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
    }
  }

  //Is this person pregnant?
  if (Global::Enable_Births && this->sex == 'F' && age <= Demographics::MAX_PREGNANCY_AGE) {
    if ( ( ( 280 ) * Demographics::age_daily_birth_rate[ age ] ) > RANDOM() ) {
      //Yes, so set the due_date (in simulation days)
      this->due_sim_day = ( day + IRAND( 1, 280 ) );
      this->pregnant = true;
      Global::Pop.set_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
      FRED_STATUS( 2, "Birth scheduled during initialization! conception day: %d, delivery day: %d\n",
            conception_sim_day, due_sim_day );
    }
  }
}

Demographics::~Demographics() {
}

void Demographics::update(int day) {
  // TODO: this does nothing, leave as stub for future extensions
}

void Demographics::update_births( int day ) {
  if (Global::Enable_Births) {
    // Is this your day to give birth? ( previously set in Demographics::birthday )
    if ( pregnant && due_sim_day == day ) {
      conception_sim_day = -1;
      due_sim_day = -1;
      pregnant = false;
      // don't update this person's demographics anymore
      Global::Pop.clear_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
      // give birth
      Global::Pop.prepare_to_give_birth( day, self );
    }
  }
}

void Demographics::update_deaths( int day ) {
  if ( Global::Enable_Deaths ) {
    //Is this your day to die?
    if ( deceased_sim_day == day ) {
      // don't update this person's demographics anymore
      Global::Pop.clear_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
      deceased = true;
      Global::Pop.prepare_to_die( day, self );
    }
  }
}

void Demographics::birthday(int day) {
  //The age to look up should be an integer between 0 and the MAX_AGE
  int age_lookup = (this->age <= Demographics::MAX_AGE ? this->age : Demographics::MAX_AGE);

  if (Global::Enable_Aging) {
    this->age++;
    if (this->age == Global::ADULT_AGE && this->self != this->self->get_adult_decision_maker()) {
      // become responsible for adult decisions
      this->self->become_an_adult_decision_maker();
    }
  }

  //Will this person die in the next year?
  if (Global::Enable_Deaths) {
    double pct_chance_to_die = 0.0;

    if (this->sex == 'F')
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_female[age_lookup];
    else
      pct_chance_to_die = Demographics::age_yearly_mortality_rate_male[age_lookup];

    if ( !this->deceased &&
          this->deceased_sim_day == -1 &&
          RANDOM() <= pct_chance_to_die) {

      //Yes, so set the death day (in simulation days)
      this->deceased_sim_day = (day + IRAND(0,364));
      // and flag for daily updates until death
      Global::Pop.set_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
    }
  }


  if ( Global::Enable_Births ) {
    // Will this person conceive this year ( delivery may occur in the next year )?
    if ( !pregnant && sex == 'F' && age <= Demographics::MAX_PREGNANCY_AGE && conception_sim_day == -1
          && due_sim_day == -1 ) {

      // ignore small distortion due to leap years
      double yearly_birth_prob = 365 * Demographics::age_daily_birth_rate[ age ]; 
  
      if ( yearly_birth_prob > RANDOM() ) {
        // ignore small distortion due to leap years
        conception_sim_day = day + IRAND( 1, 365 );
        due_sim_day = conception_sim_day + (int) ( draw_normal(
              Demographics::MEAN_PREG_DAYS, Demographics::STDDEV_PREG_DAYS) + 0.5 );
        pregnant = true;
        // flag for daily updates
        Global::Pop.set_mask_by_index( fred::Update_Demographics, self->get_pop_index() );
        FRED_STATUS( 2, "Birth scheduled! conception day: %d, delivery day: %d\n",
            conception_sim_day, due_sim_day );
      }
    }
  }
}

void Demographics::read_init_files() {

  if ( Demographics::is_initialized ) { return; }

  char yearly_mortality_rate_file[256];
  char yearly_birth_rate_file[256];
  double birth_rate_multiplier;
  FILE *fp = NULL;

  if (!Global::Enable_Births && !Global::Enable_Deaths)
    return;

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "read demographic init files entered\n"); fflush(Global::Statusfp);
  }

  if (Global::Enable_Births) {
    Params::get_param_from_string("yearly_birth_rate_file", yearly_birth_rate_file);
    Params::get_param_from_string("birth_rate_multiplier", &birth_rate_multiplier);
    // read and load the birth rates
    fp = Utils::fred_open_file(yearly_birth_rate_file);
    if (fp == NULL) {
      fprintf(Global::Statusfp, "Demographic init_file %s not found\n", yearly_birth_rate_file);
      exit(1);
    }
    for (int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; i++) {
      int age;
      float rate;
      if (fscanf(fp, "%d %f", &age, &rate) != 2) {
        Utils::fred_abort("Help! Read failure for age %d\n", i); 
      }
      Demographics::age_yearly_birth_rate[i] = birth_rate_multiplier * (double)rate;
    }
    for (int i = 0; i <= Demographics::MAX_PREGNANCY_AGE; i++) {
      Demographics::age_daily_birth_rate[i] = Demographics::age_yearly_birth_rate[i] / 365.25;
    }
    fclose(fp);
    if (Global::Verbose) {
      fprintf(Global::Statusfp, "finished reading Demographic init_file = %s\n", yearly_birth_rate_file);
      fflush(Global::Statusfp);
    }
  }

  if (Global::Enable_Deaths) {
    Params::get_param_from_string("yearly_mortality_rate_file", yearly_mortality_rate_file);
    
    // read death rate file and load the values unt the death_rate_array
    FILE *fp = Utils::fred_open_file(yearly_mortality_rate_file);
    if (fp == NULL) {
      fprintf(Global::Statusfp, "Demographic init_file %s not found\n", yearly_mortality_rate_file);
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
      Demographics::age_yearly_mortality_rate_male[i] = (double)male_rate;
    }
    fclose(fp);
    if (Global::Verbose) {
      fprintf(Global::Statusfp, "finished reading Demographic init_file = %s\n", yearly_mortality_rate_file);
      fflush(Global::Statusfp);
    }
  }
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "read demographic init files finished\n"); fflush(Global::Statusfp);
  }

  Demographics::is_initialized = true;
}

void Demographics::print() {
}

double Demographics::get_real_age() const {
  if ( birth_year > Date::get_epoch_start_year() ) {
    Date * tmp_date = new Date(this->birth_year, this->birth_day_of_year);
    int days_of_life = Date::days_between(Global::Sim_Current_Date, tmp_date);
    return ((double) days_of_life / 365.0);
  }
  Utils::fred_warning("WARNING: Birth year (%d) before start of epoch (Date::EPOCH_START_YEAR == %d)!\n", birth_year, Date::get_epoch_start_year());
  return -1;
}


