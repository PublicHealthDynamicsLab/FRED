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

#include "Logit.h"

#include "Condition.h"
#include "County.h"
#include "Date.h"
#include "Global.h"
#include "Logistic_Regression.h"
#include "Params.h"
#include "Person.h"
#include "Population.h"
#include "Place_List.h"
#include "Random.h"
#include "Utils.h"

Logit::Logit(char* _name, int _number_of_states) {
  strcpy(this->name, _name);
  this->number_of_states = _number_of_states;
  this->background_prob = 0.0;
  this->intercept = 0.0;
  this->cutoff = NULL;
  this->beta.clear();

  // could be static:
  this->factor_name.clear();
  this->number_of_factors = 0;
  this->number_of_conditions = 0;

  // get logistic_regression objects
  this->LR = NULL;
  this->OLR = NULL;

}

Logit::~Logit() {
}

void Logit::get_parameters() {

  FRED_VERBOSE(0, "Logit(%s)::get_parameters\n", this->name);

  // read required parameters

  char param_name[FRED_STRING_SIZE];

  // define the possible risk factors
  char fname[FRED_STRING_SIZE];

  this->factor_name.push_back("age");
  this->factor_name.push_back("is_female");
  this->factor_name.push_back("is_male");
  this->factor_name.push_back("race_is_white");
  this->factor_name.push_back("race_is_nonwhite");
  this->factor_name.push_back("race_is_african_american");
  this->factor_name.push_back("race_is_american_indian");
  this->factor_name.push_back("race_is_alaska_native");
  this->factor_name.push_back("race_is_tribal");
  this->factor_name.push_back("race_is_asian");
  this->factor_name.push_back("race_is_hawaiian_native");
  this->factor_name.push_back("race_is_other");
  this->factor_name.push_back("race_is_multiple");
  this->factor_name.push_back("household_income");
  this->factor_name.push_back("household_size");
  this->factor_name.push_back("is_employed");
  this->factor_name.push_back("is_student");
  this->factor_name.push_back("is_pregnant");
  this->factor_name.push_back("year");
  this->factor_name.push_back("age0");
  this->factor_name.push_back("age0-2");
  this->factor_name.push_back("age1-4");
  this->factor_name.push_back("age2-65");
  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "age%d-%d", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("age85+");

  // combine age and sex

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_male,age%d-%d", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_male,age85+");

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_female,age%d-%d", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_female,age85+");

  // combine age, sex and race

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_male,age%d-%d,race_is_white", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_male,age85+,race_is_white");

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_female,age%d-%d,race_is_white", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_female,age85+,race_is_white");

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_male,age%d-%d,race_is_nonwhite", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_male,age85+,race_is_nonwhite");

  for (int n = 0; n <= 80 ; n += 5) {
    sprintf(fname, "is_female,age%d-%d,race_is_nonwhite", n, n+4);
    this->factor_name.push_back(string(fname));
  }
  this->factor_name.push_back("is_female,age85+,race_is_nonwhite");

  // location of residence

  int number_of_counties = Global::Places.get_number_of_counties();
  for (int n = 0; n < number_of_counties; n++) {
    int fips = Global::Places.get_county_with_index(n)->get_fips();
    sprintf(fname, "county_is_%d", fips);
    this->factor_name.push_back(string(fname));
  }

  int number_of_census_tracts = Global::Places.get_number_of_census_tracts();
  for (int n = 0; n < number_of_census_tracts; n++) {
    long int fips = Global::Places.get_census_tract_with_index(n);
    sprintf(fname, "census_tract_is_%ld", fips);
    this->factor_name.push_back(string(fname));
  }

  FRED_VERBOSE(0, "Logit(%s)::get_parameters factor_name.size = %d\n",
	       this->name, factor_name.size());

  // number of health conditions in the model
  this->number_of_conditions = Global::Conditions.get_number_of_conditions();

  FRED_VERBOSE(0, "Logit(%s)::get_parameters conditions = %d\n",
	       this->name, this->number_of_conditions);


  FRED_VERBOSE(0, "Logit(%s)::get_parameters finished\n",
	       this->name);
}


void Logit::setup() {

}


void Logit::prepare() {

  FRED_VERBOSE(0,"prepare entered for %s\n",this->name);
  // now that all the logit's have been defined, we can get factors for
  // each possible health state

  char param_name[FRED_STRING_SIZE];
  char fname[FRED_STRING_SIZE];

  for (int i = 0; i < number_of_conditions; i++) {
    Condition* condition = Global::Conditions.get_condition(i);

    // factor is 1 if the agent ever had onset of the given condition
    sprintf(fname, "%s.ever", condition->get_condition_name());
    // FRED_VERBOSE(0,"fname = %s\n",fname);
    factor_name.push_back(string(fname));

    // factor is 1 if the agent has symptoms of the given condition
    sprintf(fname, "%s.is_symptomatic", condition->get_condition_name());
    // FRED_VERBOSE(0,"fname = %s\n",fname);
    factor_name.push_back(string(fname));

    // factor is 1 if the agent is infectious
    sprintf(fname, "%s.is_infectious", condition->get_condition_name());
    // FRED_VERBOSE(0,"fname = %s\n",fname);
    factor_name.push_back(string(fname));

    // factors for each state on each condition
    for (int j = 0; j < condition->get_number_of_states(); j++) {
      string sname = condition->get_state_name(j);
      sprintf(fname, "%s.%s", condition->get_condition_name(), sname.c_str());
      // FRED_VERBOSE(0,"fname = %s\n",fname);
      factor_name.push_back(string(fname));
    }
  }

  // we are finished adding possible factors 
  this->number_of_factors = factor_name.size();

  // get coefficients for link function

  if (this->number_of_states > 2) {
    // cutoff values
    this->cutoff = new double[this->number_of_states-1];
    for(int i = 0; i < this->number_of_states-1; ++i) {
      this->cutoff[i] = 0.0;
      sprintf(param_name, "%s.cutoff[%d]", this->name, i);
      Params::get_param(param_name, &(this->cutoff[i]));
    }
  }

  // these are optional parameters!
  Params::disable_abort_on_failure();

  if (this->number_of_states <= 2) {
    sprintf(param_name, "%s.background_prob", this->name);
    Params::get_param(param_name, &this->background_prob);
    sprintf(param_name, "%s.intercept", this->name);
    Params::get_param(param_name, &this->intercept);
  }

  for (int i = 0; i < this->number_of_factors; i++) {
    double coeff = 0.0;
    sprintf(param_name, "%s.%s", this->name, this->factor_name[i].c_str());
    Params::get_param(param_name, &coeff );
    beta.push_back(coeff);
    if (coeff > 0) {
      FRED_VERBOSE(0, "%s.%s.beta = %f\n", this->name, this->factor_name[i].c_str(), coeff);
    }
  }

  // restore requiring parameters
  Params::set_abort_on_failure();

  char model_name[256];
  sprintf(model_name,"%s", this->name);
  double z0;
  if (this->background_prob == 0.0) {
    if (this->intercept == 0.0) {
      z0 = -100.0;
    }
    else {
      z0 = exp(this->intercept)/ (1.0 + exp(this->intercept));
    }
  }
  else {
    if (this->background_prob == 1.0) {
      z0 = 100.0;
    }
    else {
      z0 = log(this->background_prob / (1.0 - this->background_prob));
    }
  }
  if (this->number_of_states == 2) {
    this->LR = new Logistic_Regression(model_name, this->number_of_states, z0, this->beta);
  }
  else {
    // ignore background probability for ordinal logit
    z0 = 0.0;
    this->OLR = new Ordinal_Logistic_Regression(model_name, this->number_of_states, z0, this->beta, this->cutoff);
  }

  print();

}


void Logit::print() {
}


int Logit::get_outcome(Person *person) {
  double x[this->number_of_factors];
  fill_x(person, x);
  if (this->number_of_states <= 2) {
    return this->LR->get_outcome(x);
  }
  else {
    return this->OLR->get_outcome(x);
  }
}


void Logit::fill_x(Person* person, double* x) {
  
  double age = person->get_real_age();
  // FRED_VERBOSE(0, "fill_x entered with person %d age %.1f\n", person->get_id(), age);

  // warning: these tests must occur in same as as factor_names are generated.
  int i = 0;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_real_age(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_sex() == 'F'; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_sex() == 'M'; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::WHITE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()!=Global::WHITE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::AFRICAN_AMERICAN; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::AMERICAN_INDIAN; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::ALASKA_NATIVE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::TRIBAL; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::ASIAN; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::HAWAIIAN_NATIVE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::OTHER_RACE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_race()==Global::MULTIPLE_RACE; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_income(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->get_household_size(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->is_employed();; i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->is_student(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : person->is_pregnant(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : Date::get_year(); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : (0 <= age && age < 1); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : (0 <= age && age < 2); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : (1 <= age && age < 5); i++;
  x[i] = (beta[i]==0.0) ? 0.0 : (2 <= age && age < 65); i++;
  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (n <= age && age < n+5); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (85 <= age); i++;

  // sex and age

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && n <= age && age < n+5); i++;
    if (beta[i-1]!=0.0) {printf("LOGIT: sex %c age %.1f beta = %f x = %f\n", person->get_sex(), age, beta[i-1], x[i-1]);}
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && 85 <= age); i++;

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && n <= age && age < n+5); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && 85 <= age); i++;

  // sex, age, and race

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && n <= age && age < n+5 && person->get_race()==Global::WHITE); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && 85 <= age && person->get_race()==Global::WHITE); i++;

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && n <= age && age < n+5 && person->get_race()==Global::WHITE); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && 85 <= age && person->get_race()==Global::WHITE); i++;

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && n <= age && age < n+5 && person->get_race()!=Global::WHITE); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'M' && 85 <= age && person->get_race()!=Global::WHITE); i++;

  for (int n = 0; n <= 80 ; n += 5) {
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && n <= age && age < n+5 && person->get_race()!=Global::WHITE); i++;
  }
  x[i] = (beta[i]==0.0) ? 0.0 : (person->get_sex() == 'F' && 85 <= age && person->get_race()!=Global::WHITE); i++;

  int number_of_counties = Global::Places.get_number_of_counties();
  for (int n = 0; n < number_of_counties; n++) {
    int fips = Global::Places.get_county_with_index(n)->get_fips();
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_household_county_fips()==fips); i++;
  }

  int number_of_census_tracts = Global::Places.get_number_of_census_tracts();
  for (int n = 0; n < number_of_census_tracts; n++) {
    long int fips = Global::Places.get_census_tract_with_index(n);
    x[i] = (beta[i]==0.0) ? 0.0 : (person->get_household_census_tract_fips()==fips); i++;
  }

  for (int id = 0; id < this->number_of_conditions; id++) {
    x[i] = (beta[i]==0.0) ? 0.0 : person->ever_active(id); i++;
    x[i] = (beta[i]==0.0) ? 0.0 : person->is_symptomatic(id); i++;
    x[i] = (beta[i]==0.0) ? 0.0 : person->is_infectious(id); i++;

    int nstates = Global::Conditions.get_condition(id)->get_number_of_states();
    for (int state = 0; state < nstates; ++state) {
      x[i] = (beta[i]==0.0) ? 0.0 : (state == person->get_health_state(id)); i++;
    }
  }
  // FRED_VERBOSE(0, "fill_x finished for %d factors",i);
}



void Logit::test() {
  Logit* test_logit;
  int states = 2;
  test_logit = new Logit((char*)"test.logit", states);
  test_logit->get_parameters();
  test_logit->prepare();
  test_logit->test_pop();
  Logit* test_ologit;
  states = 3;
  test_ologit = new Logit((char*)"test.logit", states);
  test_ologit->get_parameters();
  test_ologit->prepare();
  test_ologit->test_pop();
  exit(0);
}

void Logit::test_pop() {
  int popsize = Global::Pop.get_population_size();
  for(int p = 0; p < popsize; ++p) {
    Person* person = Global::Pop.get_person(p);
    double x[this->number_of_factors];
    fill_x(person, x);
    double prob;
    int out;
    if (this->number_of_states <= 2) {
      prob = this->LR->get_prob(x);
      out = this->LR->get_outcome(x);
    }
    else {
      prob = this->OLR->get_prob(x);
      out = this->OLR->get_outcome(x);
    }
    printf("TEST_POP: states %d person %d age %.1f sex %c race %d prob %f outcome %d\n",
	   this->number_of_states,
	   person->get_id(),
	   person->get_real_age(),
	   person->get_sex(),
	   person->get_race(),
	   prob, out);
  }
}
