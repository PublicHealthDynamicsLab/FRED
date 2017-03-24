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

#include "Age_Map.h"
#include "Condition.h"
#include "Global.h"
#include "HIV_Natural_History.h"
#include "Logit.h"
#include "Natural_History.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "State_Space.h"
#include "Utils.h"



Natural_History::Natural_History() {
  strcpy(this->transition_model, "unset");
  this->transition_matrix = NULL;
  this->transition_period_median = NULL;
  this->transition_period_dispersion = NULL;
  this->transition_period_offset = NULL;
  this->transition_period_upper_bound = NULL;
  this->onset_logit = NULL;
  this->transition_logit = NULL;
  this->state_space = NULL;

  this->R0 = -1.0;
  this->R0_a = -1.0;
  this->R0_b = -1.0;

  strcpy(this->initialization_model, "unset");
  this->initialization_probability = NULL;
  this->initialization_logit = NULL;

  this->immunity_logit = NULL;

  strcpy(this->case_fatality_model, "unset");
  this->case_fatality_logit = NULL;

  this->state_infectivity.clear();
  this->state_susceptibility.clear();
  this->state_symptoms_level.clear();
  this->state_fatality.clear();
  this->probability_of_household_confinement.clear();
  this->decide_household_confinement_daily.clear();
}


Natural_History::~Natural_History() {
}


/**
 * This static factory method is used to get an instance of a specific Natural_History Model.
 * Depending on the model parameter, it will create a specific Natural_History Model and return
 * a pointer to it.
 *
 * @param a string containing the requested Natural_History model type
 * @return a pointer to a specific Natural_History model
 */

Natural_History * Natural_History::get_new_natural_history(char* natural_history_model) {
  
  if (strcmp(natural_history_model, "hiv") == 0) {
    return new HIV_Natural_History;
  }
  
  if (strcmp(natural_history_model, "markov") == 0) {
    return new Natural_History;
  }
  
  FRED_WARNING("Unknown natural_history_model (%s)-- using base Natural_History class.\n", natural_history_model);
  return new Natural_History;
}




void Natural_History::setup(Condition * _condition) {

  FRED_VERBOSE(0, "Natural_History::setup for condition %s\n",
	       _condition->get_condition_name());

  this->condition = _condition;
  this->id = this->condition->get_id();
  this->state_space = new State_Space(this->condition->get_condition_name());

}


void Natural_History::get_parameters() {
  char param_name[256];

  FRED_VERBOSE(0, "Natural_History::get_parameters for condition %d\n", this->condition->get_id());

  // this sets the number and names of the states:
  this->state_space->get_parameters();
  this->number_of_states = this->state_space->get_number_of_states();

  // read optional parameters
  Params::disable_abort_on_failure();

  ///////// TRANSMISSIBILITY

  this->transmissibility = 0.0;
  Params::get_param(get_name(), "transmissibility", &this->transmissibility);

  // convenience R0 setting parameters
  Params::get_param(get_name(), "R0", &this->R0);
  Params::get_param(get_name(), "R0_a", &this->R0_a);
  Params::get_param(get_name(), "R0_b", &this->R0_b);

  if (this->R0 == -1.0 && this->id == 0) {
    // backward compatibility: check for unadorned parameters (deprecated)
    Params::get_param("R0", &this->R0);
    Params::get_param("R0_a", &this->R0_a);
    Params::get_param("R0_b", &this->R0_b);
  }

  if(this->R0 > 0) {
    this->transmissibility = this->R0_a * this->R0 * this->R0 + this->R0_b * this->R0;
    FRED_VERBOSE(0, "R0 = %f so setting transmissibility to %f\n", this->R0, this->transmissibility);
  }

  ///////// INITIALIZATION MODEL

  // the initialization model determines how to select the initial state
  // for each agent.

  // the default is to use a probability distribution over the states
  strcpy(this->initialization_model, "probability_distribution");
  Params::get_param(get_name(), "initialization_model", this->initialization_model);

  if (strcmp(this->initialization_model, "probability_distribution")==0) {
    // set up the initialization probabilities
    this->initialization_probability = new double [ this->number_of_states ];

    // select state 0 by default
    this->initialization_probability[0] = 1.0;
    for (int i = 1; i < this->number_of_states; ++i) {
      this->initialization_probability[i] = 0.0;
    }

    // read explict parameters. Note: cannot set element 0 this way!
    // This ensures a valid prob distribution (see below)
    for (int i = 1; i < this->number_of_states; ++i) {
      double prob = -1.0;
      sprintf(param_name, "%s.initialization_probability[%s]",
	      get_name(), get_state_name(i).c_str());
      Params::get_param(param_name, &prob);
      if (0.0 <= prob) {
	this->initialization_probability[i] = prob;
      }
    }

    // ensure that this is a valid prob distribution
    double sum = 0.0;
    for (int i = 1; i < this->number_of_states; ++i) {
      assert(this->initialization_probability[i] >= 0);
      sum += this->initialization_probability[i];
    }
    this->initialization_probability[0] = 1.0 - sum;
    assert(this->initialization_probability[0] >= 0);

    // print
    for (int i = 0; i < this->number_of_states; ++i) {
      FRED_VERBOSE(0, "%s.initialization_probability[%s] = %f\n",
		   get_name(), get_state_name(i).c_str(),
		   this->initialization_probability[i]);
    }

  }
  else if (strcmp(this->initialization_model, "logit")==0) {
    // create the initialization model
    char logit_name[FRED_STRING_SIZE];
    sprintf(logit_name,"%s.initialization", get_name());
    initialization_logit = new Logit(logit_name, this->number_of_states);
    initialization_logit->get_parameters();
  }
  else {
    Utils::fred_abort("Unrecognized %s.initialization__model = %s\n",
		      get_name(), this->initialization_model);
  }

  ///////// IMMUNITY MODEL

  // the immunity logit determines how to select the initial immunity
  // state for each agent.
  char logit_name[FRED_STRING_SIZE];
  sprintf(logit_name,"%s.immunity", get_name());
  this->immunity_logit = new Logit(logit_name, 2);
  this->immunity_logit->get_parameters();

  ///////// CASE_FATALITY MODEL

  // the case_fatality model determines the probability for
  // the agent being a case fatality from this condition

  strcpy(this->case_fatality_model, "logit");
  sprintf(logit_name,"%s.case_fatality", get_name());
  this->case_fatality_logit = new Logit(logit_name, 2);
  this->case_fatality_logit->get_parameters();

  ///////// TRANSITION MODEL

  // The transition model determines how to decide whether to change
  // state, and which state to change to.
  strcpy(this->transition_model, "transition_matrix");
  Params::get_param(get_name(), "transition_model", this->transition_model);

  // create transition matrix
  this->transition_matrix = new double* [ this->number_of_states ];
  for (int i = 0; i < this->number_of_states; ++i) {
    this->transition_matrix[i] = new double [ this->number_of_states ];
  }

  // initialize to identity matrix
  for (int i = 0; i < this->number_of_states; ++i) {
    for (int j = 0; j < this->number_of_states; ++j) {
      this->transition_matrix[i][j] = 0.0;
    }
    this->transition_matrix[i][i] = 1.0;
  }

  if (strcmp(this->transition_model, "transition_matrix")==0) {

    // read optional parameters
    Params::disable_abort_on_failure();

    // read in the transition matrix of fixed probabilities for changing
    // from state i to state j
    char filename[256];
    strcpy(filename, "");
    Params::get_param(get_name(), "transition_matrix_file", filename);
    if (strcmp(filename, "") != 0) {
      FILE* fp = Utils::fred_open_file(filename);
      for (int i = 0; i < this->number_of_states; ++i) {
	for (int j = 0; j < this->number_of_states; ++j) {
	  int ok = fscanf(fp, "%lf", &(this->transition_matrix[i][j]));
	  assert(ok == 1);
	}      
      }
      fclose(fp);
    }

    // explict parameters can override the transition matrix file.
    // NOTE: cannot change the diagonal element this way.
    for (int i = 0; i < this->number_of_states; ++i) {
      for (int j = 0; j < this->number_of_states; ++j) {
	if (i!=j) {
	  double prob = -1.0;
	  sprintf(param_name, "%s.transition_probability[%s][%s]",
		  get_name(), get_state_name(i).c_str(), get_state_name(j).c_str());
	  Params::get_param(param_name, &prob);
	  if (0.0 <= prob) {
	    this->transition_matrix[i][j] = prob;
	  }
	}      
      }
    }

    // ensure that this is a valid prob transition matrix by adjusting
    // diagonal element as needed
    for (int i = 0; i < this->number_of_states; ++i) {
      double sum = 0.0;
      for (int j = 0; j < this->number_of_states; ++j) {
	assert(this->transition_matrix[i][j] >= 0);
	if (i != j) {
	  sum += this->transition_matrix[i][j];
	}
      }
      this->transition_matrix[i][i] = 1.0 - sum;
    }

    // print matrix to LOG file
    printf("transition_matrix:\n");
    for (int i = 0; i < this->number_of_states; ++i) {
      for (int j = 0; j < this->number_of_states; ++j) {
	printf("%.3f ", this->transition_matrix[i][j]);
      }
      printf("\n");
    }

    // restore requiring parameters
    Params::set_abort_on_failure();

  }    
  else if (strcmp(this->transition_model, "logit")==0) {
    // create the onset model
    char logit_name[FRED_STRING_SIZE];
    sprintf(logit_name,"%s.onset", get_name());
    onset_logit = new Logit(logit_name, 2); 
    onset_logit->get_parameters();

    // create the transition model
    sprintf(logit_name,"%s.transition", get_name());
    transition_logit = new Logit(logit_name, this->number_of_states);
    transition_logit->get_parameters();
  }
  else {
    Utils::fred_abort("Unknown %s.transition_model = %s\n",
		      get_name(), this->transition_model);
  }

  // get the state-specific transition period lognormal distribution
  this->transition_period_median = new double[this->number_of_states];
  this->transition_period_dispersion = new double[this->number_of_states];
  this->transition_period_offset = new double[this->number_of_states];
  this->transition_period_upper_bound = new double[this->number_of_states];

  // read optional parameters
  Params::disable_abort_on_failure();

  // get time period for transition for each state (default = 1 day)
  for(int i = 0; i < this->number_of_states; ++i) {
    double period = 1.0;
    sprintf(param_name, "%s.%s.transition_period", get_name(), get_state_name(i).c_str());
    Params::get_param(param_name, &period);
    this->transition_period_median[i] = period;

    double dispersion = 1.0;
    sprintf(param_name, "%s.%s.transition_period_dispersion", get_name(), get_state_name(i).c_str());
    Params::get_param(param_name, &dispersion);
    this->transition_period_dispersion[i] = dispersion;

    double offset = 0.0;
    sprintf(param_name, "%s.%s.transition_period_offset", get_name(), get_state_name(i).c_str());
    Params::get_param(param_name, &offset);
    this->transition_period_offset[i] = offset;

    double upper = 99999.0;
    sprintf(param_name, "%s.%s.transition_period_upper_bound", get_name(), get_state_name(i).c_str());
    Params::get_param(param_name, &upper);
    this->transition_period_upper_bound[i] = upper;
  }

  ///////// CONTACT TRACING EFFECTIVENESS
  this->contact_tracing_effectiveness = 0.0;
  Params::get_param(get_name(), "contact_tracing_effectiveness", &this->contact_tracing_effectiveness);

  this->contact_tracing_trigger = 0;
  Params::get_param(get_name(), "contact_tracing_trigger", &this->contact_tracing_trigger);

  // restore requiring parameters
  Params::set_abort_on_failure();

  this->state_infectivity.reserve(this->number_of_states);
  this->state_susceptibility.reserve(this->number_of_states);
  this->state_symptoms_level.reserve(this->number_of_states);
  this->state_fatality.reserve(this->number_of_states);
  this->state_is_recovered.reserve(this->number_of_states); 
  this->probability_of_household_confinement.reserve(this->number_of_states);
  this->decide_household_confinement_daily.reserve(this->number_of_states);
  this->state_infectivity.clear();
  this->state_susceptibility.clear();
  this->state_symptoms_level.clear();
  this->state_fatality.clear();
  this->state_is_recovered.clear();
  this->probability_of_household_confinement.clear();
  this->decide_household_confinement_daily.clear();

  // begin reading optional parameters
  Params::disable_abort_on_failure();

  double age = 0.0;
  Params::get_param(get_name(), "min_age", &age);
  this->min_age = age;

  age = 999.0;
  Params::get_param(get_name(), "max_age", &age);
  this->max_age = age;

  int is_recovered, symp_level;
  double inf, susc, symp, fatal;
  char level_str[32];

  for (int i = 0; i < this->number_of_states; i++) {

    sprintf(param_name, "%s.%s.infectivity", get_name(), get_state_name(i).c_str());
    inf = 0.0;
    Params::get_param(param_name, &inf);
    this->state_infectivity.push_back(inf);

    sprintf(param_name, "%s.%s.susceptibility", get_name(), get_state_name(i).c_str());
    if (i == 0) {
      susc = 1.0;
    }
    else {
      susc = 0.0;
    }
    Params::get_param(param_name, &susc);
    this->state_susceptibility.push_back(susc);

    sprintf(param_name, "%s.%s.symptoms_level", get_name(), get_state_name(i).c_str());
    sprintf(level_str,"xxx");
    Params::get_param(param_name, level_str);
    if (strcmp(level_str,"xxx")==0) {
      sprintf(level_str,"none");
    }
    symp_level = get_symptoms_level_from_string(level_str);
    if (symp_level < 0) {
      Utils::fred_abort("Bad symptoms level for %s.%s.symptoms_level\n",
			get_name(), get_state_name(i).c_str());
    }
    this->state_symptoms_level.push_back(symp_level);

    sprintf(param_name, "%s.%s.fatality", get_name(), get_state_name(i).c_str());
    fatal = 0.0;
    Params::get_param(param_name, &fatal);
    this->state_fatality.push_back(fatal);

    sprintf(param_name, "%s.%s.is_recovered", get_name(), get_state_name(i).c_str());
    is_recovered = 0;
    Params::get_param(param_name, &is_recovered);
    this->state_is_recovered.push_back(is_recovered);

    sprintf(param_name, "%s.%s.probability_of_household_confinement", get_name(), get_state_name(i).c_str());
    double prob = 0.0;
    Params::get_param(param_name, &prob);
    this->probability_of_household_confinement.push_back(prob);
    if (prob > 0.0) {
      FRED_VERBOSE(0, "SETTING %s.%s.probability_of_household_confinement = %f\n",
		   get_name(), get_state_name(i).c_str(), this->probability_of_household_confinement[i]);
    }

    sprintf(param_name, "%s.%s.decide_household_confinement_daily", get_name(), get_state_name(i).c_str());
    int daily = 0;
    Params::get_param(param_name, &daily);
    this->decide_household_confinement_daily.push_back(daily);
    if (daily > 0) {
      FRED_VERBOSE(0, "SETTING %s.%s.decide_household_confinement_daily = %f\n",
		   get_name(), get_state_name(i).c_str(), this->decide_household_confinement_daily[i]);
    }

  }

  // get state to enter upon exposure (default to state 1 if it exists)
  this->exposed_state = this->number_of_states > 1 ? 1 : 0;
  char exp_state_name[256];
  strcpy(exp_state_name, "");
  Params::get_param(get_name(), "exposed_state", exp_state_name);
  if (strcmp(exp_state_name,"") != 0) {
    this->exposed_state = this->state_space->get_state_from_name(exp_state_name);
  }
  FRED_VERBOSE(0, "exposed state = %d\n", this->exposed_state);

  // restore requiring parameters
  Params::set_abort_on_failure();

  // enable case_fatality, as specified by each state
  this->enable_case_fatality = 1;

  print();
  FRED_VERBOSE(0, "Natural_History::setup finished for condition %s\n",
	       this->condition->get_condition_name());
}

void Natural_History::prepare() {
  FRED_VERBOSE(0, "Natural_History::prepare entered for condition %s\n",
	       this->condition->get_condition_name());

  if (this->initialization_logit != NULL) {
    FRED_VERBOSE(0, "Natural_History::prepare initialization_logit for %s\n", get_name());
    this->initialization_logit->prepare();
  }
  this->immunity_logit->prepare();
  if (this->onset_logit != NULL) {
    FRED_VERBOSE(0, "Natural_History::prepare onset_logit for %s\n", get_name());
    this->onset_logit->prepare();
  }
  if (this->transition_logit != NULL) {
    FRED_VERBOSE(0, "Natural_History::prepare transition_logit for %s\n", get_name());
    this->transition_logit->prepare();
  }
  if (this->case_fatality_logit != NULL) {
    FRED_VERBOSE(0, "Natural_History::prepare case_fatality_logit for %s\n", get_name());
    this->case_fatality_logit->prepare();
  }
  this->transmission_modifier = new double* [this->number_of_states];
  this->susceptibility_modifier = new double* [this->number_of_states];
  this->symptoms_modifier = new double* [this->number_of_states];
  for (int state = 0; state < this->number_of_states; ++state) {
    this->transmission_modifier[state] = new double [Global::Conditions.get_number_of_conditions()];
    this->susceptibility_modifier[state] = new double [Global::Conditions.get_number_of_conditions()];
    this->symptoms_modifier[state] = new double [Global::Conditions.get_number_of_conditions()];
  }

  this->state_modifier = new int** [this->number_of_states];
  for (int state = 0; state < this->number_of_states; ++state) {
    this->state_modifier[state] = new int* [Global::Conditions.get_number_of_conditions()];
    for (int cond = 0; cond < Global::Conditions.get_number_of_conditions(); ++cond) {
      int target_states = Global::Conditions.get_condition(cond)->get_number_of_states();
      this->state_modifier[state][cond] = new int [target_states];
    }
  }

  // read optional parameters
  Params::disable_abort_on_failure();

  char param_name[FRED_STRING_SIZE];
  for (int state = 0; state < this->number_of_states; ++state) {
    for (int cond = 0; cond < Global::Conditions.get_number_of_conditions(); ++cond) {
      sprintf(param_name, "%s.%s.reduces_transmission_of.%s",
	      get_name(),
	      get_state_name(state).c_str(),
	      Global::Conditions.get_condition(cond)->get_condition_name());
      double reduction = 0.0;
      Params::get_param(param_name, &reduction);
      this->transmission_modifier[state][cond] = 1.0 - reduction;

      sprintf(param_name, "%s.%s.reduces_susceptibility_to.%s",
	      get_name(),
	      get_state_name(state).c_str(),
	      Global::Conditions.get_condition(cond)->get_condition_name());
      reduction = 0.0;
      Params::get_param(param_name, &reduction);
      this->susceptibility_modifier[state][cond] = 1.0 - reduction;

      sprintf(param_name, "%s.%s.reduces_symptoms_of.%s",
	      get_name(),
	      get_state_name(state).c_str(),
	      Global::Conditions.get_condition(cond)->get_condition_name());
      reduction = 0.0;
      Params::get_param(param_name, &reduction);
      this->symptoms_modifier[state][cond] = 1.0 - reduction;
    }
  }

  for (int state = 0; state < this->number_of_states; ++state) {
    for (int cond = 0; cond < Global::Conditions.get_number_of_conditions(); ++cond) {
      int target_states = Global::Conditions.get_condition(cond)->get_number_of_states();
      for (int src = 0; src < target_states; ++src) {
	for (int dest = 0; dest < target_states; ++dest) {
	  sprintf(param_name, "%s.%s.changes_state_from.%s.%s",
		  get_name(),
		  get_state_name(state).c_str(),
		  Global::Conditions.get_condition(cond)->get_condition_name(),
		  Global::Conditions.get_condition(cond)->get_state_name(src).c_str());
	  char dest_string[FRED_STRING_SIZE];
	  strcpy(dest_string, Global::Conditions.get_condition(cond)->get_state_name(src).c_str());
	  Params::get_param(param_name, dest_string);
	  this->state_modifier[state][cond][src] = Global::Conditions.get_condition(cond)->get_state_from_name(dest_string);
	}
      }
    }
  }

  // restore requiring parameters
  Params::set_abort_on_failure();

}

char* Natural_History::get_name() {
  return this->state_space->get_name();
}

string Natural_History::get_state_name(int state) {
  if (state < 0) {
    return "UNSET";
  }
  return this->state_space->get_state_name(state);
}

void Natural_History::print() {
  printf("NATURAL HISTORY %s.initialization_model = %s\n", get_name(), this->initialization_model);
  printf("NATURAL HISTORY %s.transition_model = %s\n", get_name(), this->transition_model);
  printf("NATURAL HISTORY %s.case_fatality_model = %s\n", get_name(), this->case_fatality_model);
  printf("NATURAL HISTORY %s.min_age = %f\n", get_name(), this->min_age);
  printf("NATURAL HISTORY %s.max_age = %f\n", get_name(), this->max_age);
  printf("number of states = %d\n", this->number_of_states);
  for (int i = 0; i < this->number_of_states; i++) {
    printf("NATURAL HISTORY %s.%s.infectivity = %f\n",
	   get_name(), get_state_name(i).c_str(), this->state_infectivity[i]);
    printf("NATURAL HISTORY %s.%s.symptoms_level = %s\n",
	   get_name(), get_state_name(i).c_str(),
	   get_symptoms_string(this->state_symptoms_level[i]).c_str());
    printf("NATURAL HISTORY %s.%s.fatality = %f\n",
	   get_name(), get_state_name(i).c_str(), this->state_fatality[i]);
    printf("NATURAL HISTORY %s.%s.is_recovered = %d\n",
	   get_name(), get_state_name(i).c_str(), this->state_is_recovered[i]);
  }
}

double Natural_History::get_transition_period(int state) {
  double sigma = log(this->transition_period_dispersion[state]);
  if (sigma == 0.0) {
    return this->transition_period_median[state];
  }
  double mu = log(this->transition_period_median[state]);
  double transition_period =  Random::draw_lognormal(mu, sigma) + this->transition_period_offset[state];
  int tries = 1;
  int max_tries = 10;
  while (tries <= max_tries && this->transition_period_upper_bound[state] <= transition_period) {
    transition_period = Random::draw_lognormal(mu, sigma) + this->transition_period_offset[state];
    tries++;
  }
  if (tries > max_tries) {
    transition_period = Random::draw_random(0.0, this->transition_period_upper_bound[state]);
  }

  if (transition_period <= 0.0) {
    transition_period = 1;
  }
  return transition_period;

}

int Natural_History::get_initial_state(Person* person) {
  int is_immune = immunity_logit->get_outcome(person);
  if (is_immune) {
    return -1;
  }
  double age = person->get_real_age();
  if (age < this->min_age || this->max_age < age) {
    return 0;
  }
  if (strcmp(this->initialization_model, "probability_distribution")==0) {
    return get_next_state(0, this->initialization_probability);
  }
  if (strcmp(this->initialization_model, "logit")==0) {
    return initialization_logit->get_outcome(person);
  }
  assert(strcmp("Help! No initialization model","")==0);
  return -1;
}

int Natural_History::get_next_state(Person* person, int state) {
  if (strcmp(this->transition_model, "transition_matrix")==0) {
    return get_next_state(state, transition_matrix[state]);
  }
  if (strcmp(this->transition_model, "logit")==0) {
    if (person->ever_active(this->id)) {
      int result = transition_logit->get_outcome(person);
      FRED_VERBOSE(1, "UPDATE_LOGIT person %d result %d\n", person->get_id(), result);
      return result;
    }
    else {
      if (onset_logit->get_outcome(person)) {
	return this->exposed_state;
      }
      else {
	return 0;
      }
    }
  }
  assert(strcmp("Help! No transition model","")==0);
  return -1;
}

bool Natural_History::is_fatal(Person* person, int state) {
  if (person->is_infected(this->id)==false) {
    return false;
  }
  if (is_fatal(state)) {
    return true;
  }
  if (strcmp(this->case_fatality_model, "probability")==0) {
    if (this->state_fatality[state] == 0.0) {
      return false;
    }
    else {
      double r = Random::draw_random();
      return (r < this->state_fatality[state]);
    }
  }
  else {
    // use logit model
    return this->case_fatality_logit->get_outcome(person);
  }
}


int Natural_History::get_next_state(int state, double* transition_prob) {

  // ensure that transition_prob is a probability distribution
  double total = 0.0;
  for(int j = 0; j < this->number_of_states; ++j) {
    assert(transition_prob[j] >= 0.0);
    total += transition_prob[j];
  }
  if (total == 0.0) {
    // stay in same state
    return state;
  }

  // normalize
  for(int j = 0; j < this->number_of_states; ++j) {
    transition_prob[j] /= total;
  }

  // draw a random uniform variate
  double r = Random::draw_random();

  // find transition corresponding to this draw
  double sum = 0.0;
  for(int j = 0; j < this->number_of_states; ++j) {
    sum += transition_prob[j];
    if(r < sum) {
      return j;
    }
  }

  // should never reach this point
  Utils::fred_abort("Natural_History::get_next_state: Help! Bad result: state = %d total = %f\n",
		  state, total);
  return -1;
}

bool Natural_History::is_household_confinement_enabled(Person *person) {
  int state = person->get_health_state(this->id);
  return (this->probability_of_household_confinement[state] > 0.0);
}

bool Natural_History::is_confined_to_household(Person *person) {
  int state = person->get_health_state(this->id);
  /*
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    "HEALTH RECORD: is_confined person %d COND %s STATE %s ",
	    person->get_id(), get_name(),
	    get_state_name(state).c_str());
  }
  */
  return is_confined_to_household(state);
}

bool Natural_History::is_confined_to_household(int state) {
  /*
  if (Global::Enable_Health_Records) {
    fprintf(Global::HealthRecordfp,
	    " prob_household_confinement %lf ",
	    this->probability_of_household_confinement[state]);
  }
  */
  if (this->probability_of_household_confinement[state] > 0.0) {
    double r = Random::draw_random();
    /*
    if (Global::Enable_Health_Records) {
      fprintf(Global::HealthRecordfp,
	      " r %lf\n",
	      r);
    }
    */
    return r < this->probability_of_household_confinement[state];
  }
  else {
    /*
    if (Global::Enable_Health_Records) {
      fprintf(Global::HealthRecordfp,
	      "\n");
    }
    */
    return false;
  }
}

bool Natural_History::is_confined_to_household_due_to_contact_tracing(Person *person) {
  bool confined = false;
  if (this->contact_tracing_effectiveness > 0.0) {
    char mixing_group_type = person->get_infected_mixing_group_type(this->id);
    if (mixing_group_type != Place::TYPE_COMMUNITY && mixing_group_type != Place::TYPE_NEIGHBORHOOD) {
      double r = Random::draw_random();
      confined = (r < this->contact_tracing_effectiveness);
    }
  }
  return confined;
}


bool Natural_History::get_daily_household_confinement(Person* person) {
  int state = person->get_health_state(this->id);
  return (this->decide_household_confinement_daily[state] > 0);
}
