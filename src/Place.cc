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
// File: Place.cc
//
#include <algorithm>

#include "Date.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Global.h"
#include "Household.h"
#include "Infection.h"
#include "Neighborhood.h"
#include "Neighborhood_Patch.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Seasonality.h"
#include "Transmission.h"
#include "Utils.h"
#include "Vector_Layer.h"
#define PI 3.14159265359

// static place type codes
char Place::HOUSEHOLD = 'H';
char Place::NEIGHBORHOOD = 'N';
char Place::SCHOOL = 'S';
char Place::CLASSROOM = 'C';
char Place::WORKPLACE = 'W';
char Place::OFFICE = 'O';
char Place::HOSPITAL = 'M';
char Place::COMMUNITY = 'X';

bool Place::Enable_Neighborhood_Density_Transmission = false;
bool Place::Enable_Density_Transmission_Maximum_Infectees = false;
int Place::Density_Transmission_Maximum_Infectees = 10.0;

double Place::Seasonal_Reduction = 0.0;
double* Place::Seasonality_multiplier = NULL;

place_vec Place::infectious_places;


void Place::initialize_static_variables() {
  int temp_int = 0;
  Params::get_param_from_string("enable_neighborhood_density_transmission", &temp_int);
  Place::Enable_Neighborhood_Density_Transmission = (temp_int == 1);
  Params::get_param_from_string("enable_density_transmission_maximum_infectees", &temp_int);
  Place::Enable_Density_Transmission_Maximum_Infectees = (temp_int == 1);
  Params::get_param_from_string("density_transmission_maximum_infectees",
				&Place::Density_Transmission_Maximum_Infectees);

  // all-disease seasonality reduction
  Params::get_param_from_string("seasonal_reduction", &Place::Seasonal_Reduction);
  // setup seasonal multipliers

  if (Place::Seasonal_Reduction > 0.0) {
    int seasonal_peak_day_of_year; // e.g. Jan 1
    Params::get_param_from_string("seasonal_peak_day_of_year", &seasonal_peak_day_of_year);

    // setup seasonal multipliers
    Place::Seasonality_multiplier = new double [367];
    for(int day = 1; day <= 366; ++day) {
      int days_from_peak_transmissibility = abs(seasonal_peak_day_of_year - day);
      Place::Seasonality_multiplier[day] = (1.0 - Place::Seasonal_Reduction) +
	    Place::Seasonal_Reduction * 0.5 * (1.0 + cos(days_from_peak_transmissibility * (2 * PI / 365.0)));
      if(Place::Seasonality_multiplier[day] < 0.0) {
	      Place::Seasonality_multiplier[day] = 0.0;
      }
      // printf("Seasonality_multiplier[%d] = %e %d\n", day, Place::Seasonality_multiplier[day], days_from_peak_transmissibility);
    }
  }

  infectious_places.clear();
}

static bool compare_age (Person* p1, Person* p2) {
  return ((p1->get_age() == p2->get_age()) ? (p1->get_id() < p2->get_id()) : (p1->get_age() < p2->get_age()));
}

double** Place::prob_contact = NULL;

void Place::setup(const char* lab, fred::geo lon, fred::geo lat, Place* cont) {
  this->id = -1;		  // actual id assigned in Place_List::add_place
  this->container = cont;
  strcpy(this->label, lab);
  this->longitude = lon;
  this->latitude = lat;
  this->enrollees.reserve(8); // initial slots for 8 people -- this is expanded in enroll()
  this->enrollees.clear();
  this->N = 0;
  this->first_day_infectious = -1;
  this->last_day_infectious = -2;

  this->intimacy = 0.0;

  if(this->is_household()) {
    this->intimacy = 1.0;
  }

  if(this->is_neighborhood()) {
    this->intimacy = 0.0025;
  }

  if(this->is_school()) {
    this->intimacy = 0.025;
  }

  if(this->is_workplace()) {
    this->intimacy = 0.01;
  }

  this->staff_size = 0;

  // zero out all disease-specific counts
  for(int d = 0; d < Global::MAX_NUM_DISEASES; ++d) {

    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;

    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;

    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;

    // NOT IMPLEMENTED YET:
    // new_deaths[ d ] = 0;
    // total_deaths[ d ] = 0;
  }
  this->is_registered_as_an_infectous_place = false;

  if(Place::prob_contact == NULL) {
    Place::prob_contact = new double * [101];
    
    // create a PolyMod type matrix;
    for(int i = 0; i <= 100; ++i) {
      Place::prob_contact[i] = new double [101];
      
      for(int j = 0; j <= 100; ++j) {
        Place::prob_contact[i][j] = 0.05;
      }
    }
    for(int i = 0; i <= 100; ++i) {
      for(int j = i - 4; j <= i+4; ++j) {
        if(j < 0 || j > 100) {
          continue;
        }
        Place::prob_contact[i][j] = 1.0 - 0.2 * abs(i - j);
      }
    }
  }
  this->county_index = -1;
  this->census_tract_index = -1;
}

void Place::reset_place_state(int disease_id) {
  if(this->infectious_bitset.test(disease_id)) {
    this->place_state[disease_id].clear();
  } else {
    this->place_state[disease_id].reset();
  }
  this->infectious_bitset.reset(disease_id);
}

void Place::prepare() {
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    // Following arithmetic estimates the optimal number of thread-safe states
    // to be allocated for this place, for each disease.  The number of states
    // should always be 1 <= dim <= max_num_threads.  Each state is thread-safe,
    // but increasing the number of states reduces lock-contention.  Minimizing
    // the number of states saves memory.  The values used here must be determined
    // through experimentation.  Optimal values will vary based on the particular
    // cpu, number of threads, calibrated attack rate, population density, etc...
    unsigned int dim = (this->N / 1000) * (fred::omp_get_max_threads() / 2);
    dim = dim == 0 ? 1 : dim;
    dim = dim <= fred::omp_get_max_threads() ? dim : fred::omp_get_max_threads();
    // Initialize specified number of states for this disease
    this->place_state[d] = State<Place_State>(dim);
    // Clear the states for use
    this->place_state[d].clear();
  }
  this->N_orig = this->N;
  if(Global::Enable_Vector_Transmission && !this->is_neighborhood()) {
    setup_vector_model();
  }
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    if(this->infectious_bitset.test(d)) {
      this->place_state[d].clear();
    } else {
      this->place_state[d].reset();
    }
  }
  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->exposed_bitset.reset();
  this->unique_visitors.clear();

  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;
    // new_deaths[ d ] = 0;
  }

  this->open_date = 0;
  this->close_date = INT_MAX;
  FRED_VERBOSE(2, "Prepare place %d label %s type %c\n", this->id, this->label, this->type);
}


void Place::update(int day) {
  // sub for future use.
}

void Place::reset_visualization_data(int day) {
  this->exposed_bitset.reset();
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;
    // new_deaths[ d ] = 0;
  }
}

void Place::reset_vector_data(int day) {
  if(!this->is_neighborhood()){
    this->update_vector_population(day);
    this->vectors_not_infected_yet = true;
    this->unique_visitors.clear();
  }
}

void Place::print(int disease_id) {
  printf("Place %d label %s type %c\n", this->id, this->label, this->type);
  fflush (stdout);
}

void Place::enroll(Person* per) {
  if(get_enrollee_index(per) == -1) {
    if (N == enrollees.capacity()) {
      // double capacity if needed (to reduce future reallocations)
      enrollees.reserve(2*N);
    }
    this->enrollees.push_back(per);
    this->N++;
    FRED_VERBOSE(1,"Enroll person %d age %d in place %d %s\n", per->get_id(), per->get_age(), this->id, this->label);
  }
  else {
    FRED_VERBOSE(1,"Enroll E_WARNING person %d already in place %d %s\n", per->get_id(), this->id, this->label);
  }
}

void Place::unenroll(Person* per) {
  FRED_VERBOSE(1,"Unenroll person %d age %d from place %d %s Size = %d\n",
	       per->get_id(), per->get_age(), this->id, this->label, this->N);
  int i = get_enrollee_index(per);
  if(i == -1) {
    FRED_VERBOSE(1,"U_WARNING NOT FOUND\n");
  } else {
    enrollees.erase(enrollees.begin()+i);
    this->N--;
    FRED_VERBOSE(1,"Unenrolled. Size = %d\n", this->N);
  }
}

int Place::get_children() {
  int children = 0;
  for(int i = 0; i < this->N; ++i) {
    children += (this->enrollees[i]->get_age() < Global::ADULT_AGE);
  }
  return children;
}

int Place::get_adults() {
  return (this->N - this->get_children());
}


void Place::register_as_an_infectious_place(int disease_id) {
  if(!(this->infectious_bitset.test(disease_id))) {
    Disease * dis = Global::Diseases.get_disease(disease_id);
    dis->add_infectious_place(this);
    this->infectious_bitset.set(disease_id);
    // printf("REGISTER place %s for disease %d\n", label, disease_id);
  }
  if (this->is_registered_as_an_infectous_place == false) { 
    infectious_places.push_back(this);
    this->is_registered_as_an_infectous_place = true;
  }
}

void Place::add_susceptible(int disease_id, Person* per) {
  if(Global::Enable_Vector_Transmission && this->is_neighborhood()) {
    return;
  }
  
  if(Global::Enable_Vector_Transmission) {
    if(this->I_vectors[disease_id] || this->E_vectors[disease_id]) {
      FRED_VERBOSE(1, "add_susceptible::Place %s has %d enrollees I_vectors[%d] = %d N_vectors %d \n", this->get_label(), this->enrollees.size(), disease_id, this->I_vectors[disease_id], this->N_vectors);
      this->register_as_an_infectious_place(disease_id);
    }
  }
  this->add_host(per);
  this->place_state[disease_id]().add_susceptible(per);
}

void Place::add_nonsusceptible(int disease_id, Person* per) {
  if(Global::Enable_Vector_Transmission && this->is_neighborhood()) {
    return;
  }
  this->add_host(per);
}

void Place::add_infectious(int disease_id, Person* per) {
  if(Global::Enable_Vector_Transmission && this->is_neighborhood()) {
    return;
  }

  this->add_host(per);
  this->place_state[disease_id]().add_infectious(per);
  add_infectious_visitor(disease_id);
  if(per->get_health()->is_symptomatic()) {
    add_symptomatic_visitor(disease_id);
  }

  if(Global::Enable_Vector_Transmission) {
    FRED_VERBOSE(1,"add_infectious:: Place %s has %d enrollees S_vectors = %d N_vectors %d \n", this->get_label(), this->enrollees.size(), S_vectors,N_vectors);
    if(this->S_vectors) {
      this->register_as_an_infectious_place(disease_id);
    }
  } else {
    this->register_as_an_infectious_place(disease_id);
  }
}

void Place::print_susceptibles(int disease_id) {

  // get reference to susceptibles list for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person*> susceptibles = this->place_state_merge.get_susceptible_vector();

  printf("Disease %d SUS: ", disease_id);
  vector<Person*>::iterator itr;
  for(itr = susceptibles.begin(); itr != susceptibles.end(); ++itr) {
    printf(" %d", (*itr)->get_id());
  }
  printf("\n");
}

void Place::print_infectious(int disease_id) {
  // get references to infectious list for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person *> & infectious = this->place_state_merge.get_infectious_vector();
  
  printf("Disease %d INF: ", disease_id);
  vector<Person *>::iterator itr;
  for(itr = infectious.begin(); itr != infectious.end(); ++itr) {
    printf(" %d", (*itr)->get_id());
  }
  printf("\n");
}

bool Place::is_open(int day) {
  if(this->container) {
    return this->container->is_open(day);
  } else {
    return (day < this->close_date || this->open_date <= day);
  }
}

void Place::turn_workers_into_teachers(Place* school) {
  FRED_VERBOSE(1,"Place %s has %d enrollees\n", this->get_label(), this->enrollees.size());
  std::vector <Person *> workers;
  workers.reserve((int)this->enrollees.size());
  workers.clear();
  for(int i = 0; i < (int)this->enrollees.size(); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  int new_teachers = 0;
  for(int i = 0; i < (int)workers.size(); ++i) {
    Person* person = workers[i];
    if(person != NULL) {
      FRED_VERBOSE(1,"Potential teacher %d age %d\n", person->get_id(), person->get_age());
    } else {
      printf("Enrollee %i not found\n", i);
      abort();
    }
    if(person->become_a_teacher(school)) {
      new_teachers++;
      FRED_VERBOSE(1,"new teacher %d age %d moving from workplace %s to school %s\n",
	     person->get_id(), person->get_age(), label, school->get_label());
    }
  }
  FRED_VERBOSE(1, "%d new teachers reassigned from workplace %s to school %s\n", new_teachers,
	       label, school->get_label());
  this->N = 0;
}

void Place::reassign_workers(Place* new_place) {
  std::vector<Person*> workers;
  workers.reserve((int)this->enrollees.size());
  workers.clear();
  for(int i = 0; i < (int)this->enrollees.size(); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  int reassigned_workers = 0;
  for(int i = 0; i < (int)workers.size(); ++i) {
    workers[i]->change_workplace(new_place,0);
    // printf("worker %d age %d moving from workplace %s to place %s\n",
    //   workers[i]->get_id(), workers[i]->get_age(), label, new_place->get_label());
    reassigned_workers++;
  }
  FRED_VERBOSE(1, "%d workers reassigned from workplace %s to place %s\n", reassigned_workers,
      label, new_place->get_label());
  this->N = 0;
}

int Place::get_output_count(int disease_id, int output_code) {
  switch(output_code) {
    case Global::OUTPUT_I:
      return get_current_infectious_visitors(disease_id);
      break;
    case Global::OUTPUT_Is:
      return get_current_symptomatic_visitors(disease_id);
      break;
    case Global::OUTPUT_C:
      return get_new_infections(disease_id);
      break;
    case Global::OUTPUT_Cs:
      return get_new_symptomatic_infections(disease_id);
      break;
    case Global::OUTPUT_P:
      return get_current_infections(disease_id);
      break;
    case Global::OUTPUT_R:
      return get_recovereds(disease_id);
      break;
    case Global::OUTPUT_N:
      return get_size();
      break;
    case Global::OUTPUT_HC_DEFICIT:
      if(this->type == Place::HOUSEHOLD) {
        Household* hh = static_cast<Household*>(this);
        return hh->get_count_seeking_hc() - hh->get_count_receiving_hc();
      } else {
        return 0;
      }
      break;
  }
  return 0;
}

int Place::get_recovereds(int disease_id) {
  int count = 0;
  int size = enrollees.size();
  for(int i = 0; i < size; ++i) {
    Person * per = get_enrollee(i);
    count += per->is_recovered(disease_id);
  }
  return count;
}



/////////////////////////////////////////
//
// PLACE-SPECIFIC TRANSMISSION MODELS
//
/////////////////////////////////////////

double Place::get_contact_rate(int day, int disease_id) {

  Disease* disease = Global::Diseases.get_disease(disease_id);
  // expected number of susceptible contacts for each infectious person
  // OLD: double contacts = get_contacts_per_day(disease_id) * ((double) susceptibles[disease_id].size()) / ((double) (N-1));
  double contacts = get_contacts_per_day(disease_id) * disease->get_transmissibility();
  if(Global::Enable_Seasonality) {

    //contacts = contacts * Global::Clim->get_seasonality_multiplier_by_lat_lon(
    //    latitude,longitude,disease_id);

    double m = Global::Clim->get_seasonality_multiplier_by_lat_lon(this->latitude, this->longitude, disease_id);
    //cout << "SEASONALITY: " << day << " " << m << endl;
    contacts *= m;
  }

  // increase neighborhood contacts on weekends
  if(this->is_neighborhood()) {
    int day_of_week = Date::get_day_of_week();
    if(day_of_week == 0 || day_of_week == 6) {
      contacts = Neighborhood::get_weekend_contact_rate(disease_id) * contacts;
    }
  }
  // FRED_VERBOSE(1,"Disease %d, expected contacts = %f\n", disease_id, contacts);
  return contacts;
}

int Place::get_contact_count(Person* infector, int disease_id, int day, double contact_rate) {
  // reduce number of infective contacts by infector's infectivity
  double infectivity = infector->get_infectivity(disease_id, day);
  double infector_contacts = contact_rate * infectivity;

  FRED_VERBOSE(1, "infectivity = %f, so ", infectivity);
  FRED_VERBOSE(1, "infector's effective contacts = %f\n", infector_contacts);

  // randomly round off the expected value of the contact counts
  int contact_count = (int) infector_contacts;
  double r = Random::draw_random();
  if(r < infector_contacts - contact_count) {
    contact_count++;
  }

  FRED_VERBOSE(1, "infector contact_count = %d  r = %f\n", contact_count, r);

  return contact_count;
}

bool Place::attempt_transmission(double transmission_prob, Person* infector, Person* infectee,
    int disease_id, int day) {

  assert(infectee->is_susceptible(disease_id));
  FRED_STATUS(1, "infectee is susceptible\n", "");

  double susceptibility = infectee->get_susceptibility(disease_id);

  // reduce transmission probability due to infector's hygiene (face masks or hand washing)
  transmission_prob *= infector->get_transmission_modifier_due_to_hygiene(disease_id);

  // reduce susceptibility due to infectee's hygiene (face masks or hand washing)
  susceptibility *= infectee->get_susceptibility_modifier_due_to_hygiene(disease_id);
    
  if(Global::Enable_hh_income_based_susc_mod) {
    int hh_income = Household::get_max_hh_income(); //Default to max (no modifier)
    Household* hh = static_cast<Household*>(infectee->get_household());
    if(hh != NULL) {
      hh_income = hh->get_household_income();
    }
    susceptibility *= infectee->get_health()->get_susceptibility_modifier_due_to_household_income(hh_income);
    //Utils::fred_log("SUSC Modifier [%.4f] for HH Income [%i] modified suscepitibility to [%.4f]\n", hh_income, infectee->get_health()->get_susceptibility_modifier_due_to_household_income(hh_income), susceptibility);
  }

  FRED_VERBOSE(2, "susceptibility = %f\n", susceptibility);

  // reduce transmissibility due to seasonality
  if (Place::Seasonal_Reduction > 0.0) {
    int day_of_year = Date::get_day_of_year(day);
    transmission_prob *= Place::Seasonality_multiplier[day_of_year];
  }

  double r = Random::draw_random();
  double infection_prob = transmission_prob * susceptibility;

  if(r < infection_prob) {
    // successful transmission; create a new infection in infectee
    // Transmission transmission = Transmission(infector, this, day);
    infector->infect(infectee, disease_id, this);

    FRED_VERBOSE(1, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob);
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) == 0,
        "SEED infection day %i from %d to %d\n", day, infector->get_id(), infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) != 0,
        "infection day %i of disease %i from %d to %d\n", day, disease_id, infector->get_id(),
        infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(3, infection_prob > 1, "infection_prob exceeded unity!\n");
    return true;
  } else {
    FRED_VERBOSE(1, "transmission failed: r = %f  prob = %f\n", r, infection_prob);
    return false;
  }
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC TRANSMISSION MODELS
//
//////////////////////////////////////////////////////////


void Place::spread_infection(int day, int disease_id) {

  Disease* disease = Global::Diseases.get_disease(disease_id);
  double beta = disease->get_transmissibility();
  if(beta == 0.0) {
    reset_place_state(disease_id);
    return;
  }

  if(is_open(day) == false) {
    reset_place_state(disease_id);
    return;
  }
  if(should_be_open(day, disease_id) == false) {
    reset_place_state(disease_id);
    return;
  }

  if(this->first_day_infectious == -1) {
    this->first_day_infectious = day;
  }
  this->last_day_infectious = day;

  // get reference to susceptibles and infectious lists for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person*> & susceptibles = this->place_state_merge.get_susceptible_vector();
  std::vector<Person*> & infectious = this->place_state_merge.get_infectious_vector();

  if(Global::Enable_Vector_Transmission) {
    vector_transmission(day, disease_id);
    reset_place_state(disease_id);
    return;
  }

  // need at least one susceptible
  if(susceptibles.size() == 0) {
    reset_place_state(disease_id);
    return;
  }

  if(this->is_household()) {
    pairwise_transmission_model(day, disease_id);
    reset_place_state(disease_id);
    return;
  }

  if(this->is_neighborhood() && Place::Enable_Neighborhood_Density_Transmission==true) {
    density_transmission_model(day, disease_id);
    reset_place_state(disease_id);
    return;
  }

  if(Global::Enable_New_Transmission_Model) {
    age_based_transmission_model(day, disease_id);
  } else {
    default_transmission_model(day, disease_id);
  }
  reset_place_state(disease_id);
  return;
}

void Place::default_transmission_model(int day, int disease_id) {

  Disease* disease = Global::Diseases.get_disease(disease_id);

  // get reference to susceptibles and infectious lists for this disease_id
  place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(place_state_merge);
  std::vector<Person*> & susceptibles = place_state_merge.get_susceptible_vector();
  std::vector<Person*> & infectious = place_state_merge.get_infectious_vector();

  // printf("DAY %d PLACE %s N %d susc %d inf %d\n",
  // day, this->get_label(), N, (int) susceptibles.size(), (int) infectious.size());

  // the number of possible infectees per infector is max of (N-1) and S[s]
  // where N is the capacity of this place and S[s] is the number of current susceptibles
  // visiting this place. S[s] might exceed N if we have some ad hoc visitors,
  // since N is estimated only at startup.
  int number_targets = (N - 1 > susceptibles.size() ? N - 1 : susceptibles.size());

  // contact_rate is contacts_per_day with weeked and seasonality modulation (if applicable)
  double contact_rate = get_contact_rate(day, disease_id);

  // randomize the order of the infectious list
  FYShuffle<Person*>(infectious);

  for(int infector_pos = 0; infector_pos < infectious.size(); ++infector_pos) {
    // infectious visitor
    Person* infector = infectious[infector_pos];
    assert(infector->get_health()->is_infectious(disease_id));

    // get the actual number of contacts to attempt to infect
    int contact_count = get_contact_count(infector, disease_id, day, contact_rate);

    std::map<int, int> sampling_map;
    // get a susceptible target for each contact resulting in infection
    for(int c = 0; c < contact_count; ++c) {
      // select a target infectee from among susceptibles with replacement
      int pos = Random::draw_random_int(0, number_targets - 1);
      if(pos < susceptibles.size()) {
        if(infector == susceptibles[pos]) {
          if(susceptibles.size() > 1) {
            --(c); // redo
            continue;
          } else {
            break; // give up
          }
        }
        sampling_map[pos]++;
      }
    }

    std::map<int, int>::iterator i;
    for(i = sampling_map.begin(); i != sampling_map.end(); ++i) {
      int pos = (*i).first;
      int times_drawn = (*i).second;
      Person* infectee = susceptibles[pos];
      // get the transmission probs for this infector/infectee pair
      double transmission_prob = get_transmission_prob(disease_id, infector, infectee);
      for(int draw = 0; draw < times_drawn; ++draw) {
        // only proceed if person is susceptible
        if(infectee->is_susceptible(disease_id)) {
          attempt_transmission(transmission_prob, infector, infectee, disease_id, day);
        }
      }
    } // end contact loop
  } // end infectious list loop
}

void Place::age_based_transmission_model(int day, int disease_id) {
  Disease* disease = Global::Diseases.get_disease(disease_id);

  // get references to susceptibles and infectious lists for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person*> & susceptibles = this->place_state_merge.get_susceptible_vector();
  std::vector<Person*> & infectious = this->place_state_merge.get_infectious_vector();

  std::vector<double> infectivity_of_agent((int) infectious.size());
  int n[101];
  int start[101];
  int s[101];
  int size[101];
  double p[101][101];
  double infectivity_of_group[101];
  int infectee_count[101];
  double beta = disease->get_transmissibility();
  double force = beta * this->intimacy;

  // sort the infectious list by age
  std::sort(infectious.begin(), infectious.end(), compare_age);

  // get number of infectious and susceptible persons and infectivity of each age group
  for (int i = 0; i <=100; ++i) {
    n[i] = 0;
    s[i] = 0;
    size[i] = 0;
    start[i] = -1;
    infectivity_of_group[i] = 0.0;
  }
  for(int inf_pos = 0; inf_pos < infectious.size(); ++inf_pos) {
    Person* person = infectious[inf_pos];
    int age = person->get_age();
    if (age > 100) age = 100;
    n[age]++;
    if (start[age] == -1) { start[age] = inf_pos; }
    double infectivity = person->get_infectivity(disease_id, day); 
    infectivity_of_group[age] += infectivity;
    infectivity_of_agent[inf_pos] = infectivity;
    size[age]++;
  }
  for(int sus_pos = 0; sus_pos < susceptibles.size(); ++sus_pos) {
    int age = susceptibles[sus_pos]->get_age();
    if (age > 100) age = 100;
    s[age]++;
    size[age]++;
  }

  // compute p[i][j] = prob of an individual in group i 
  // getting infected by someone in group j
  for(int i = 0; i <=100; ++i) {
    for (int j = 0; j <=100; ++j) {
      if (s[i] == 0 || size[j] == 0) {
        p[i][j] = 0.0;
      } else {
	    p[i][j] = force * Place::prob_contact[i][j] * infectivity_of_group[j] / size[j];
      }
    }
  }    

  // get number of infections for each age group i
  for(int i = 0; i <= 100; ++i) {
    infectee_count[i] = 0;
    if(s[i] > 0) {
      double product = 1.0;
      for(int j = 0; j <= 100; ++j) {
        if(n[j] == 0) {
          continue;
        }
        if(p[i][j] == 0.0) {
          continue;
        }
        product *= pow((1 - p[i][j]), n[j]);
	    if(0 && this->is_household()) {
	      printf("SUS_AGE %d INF_AGE %d INF_COUNT %d p[i][j] %e PRODUCT %e\n",
		        i, j, n[j], p[i][j], product);
        }
      }
      double prob_infection = 1.0 - product;
      double expected_infections = s[i] * prob_infection;
	
      // convert to integer
      infectee_count[i] = (int) expected_infections;
      double r = Random::draw_random();
      if(r < expected_infections - infectee_count[i]) {
        infectee_count[i]++;
      }
      if(1 || this->is_household()) {
        printf("PLACE TYPE %c AGE GROUP %d SIZE %d PROB_INF %0.6f EXPECTED %0.2f INFECTEE COUNT %d\n",
	       this->type, i, s[i], prob_infection, expected_infections, infectee_count[i]);
      }
    }
  }
    
  // turn rows in p[i][j] into a cdf
  for(int i = 0; i <= 100; ++i) {
    double total = 0.0;
    for(int j = 0; j <= 100; ++j) {
      p[i][j] += total;
      total = p[i][j];
    }
  }  
  printf("cdf done\n"); fflush(stdout);

  // randomize the order of the susceptibles list
  FYShuffle<Person*>(susceptibles);

  // infect susceptibles according to the infectee_counts
  for(int sus_pos = 0; sus_pos < susceptibles.size(); ++sus_pos) {
    // susceptible visitor
    Person* infectee = susceptibles[sus_pos];
    int age = infectee->get_age();
    if(age > 100) {
      age = 100;
    }
    if(infectee_count[age] > 0) {

      // is this person susceptible?
      double r = Random::draw_random();
      if(r < infectee->get_susceptibility(disease_id)) {

	      printf("INFECTING pos %d age %d \n", sus_pos, age); fflush(stdout);

	      // pick an age group of infector based on cdf in row p[i]
	      r = Random::draw_random(0,p[age][100]);
	      int j = 0;
	      for(j = 0; j <= 100; ++j) {
	        printf("r %e p[%d][%d] %e\n",r,age,j,p[age][j]);
	        if(r < p[age][j]) {
	          break;
	        }
	      }
	      if(j > 100) {
	        printf("age_based_transmission: ERROR IN FINDING INFECTING AGE GROUP\n");
	        abort();
	      }
	      printf("PICK INFECTOR age %d size %d start pos %d infectivity %e\n", j, size[j], start[j], infectivity_of_group[j]); fflush(stdout);

	      // pick a infectious person from group j based on infectivity
	      r = Random::draw_random(0, infectivity_of_group[j]);
	      int pos = start[j];
	      while(infectivity_of_agent[pos] < r) {
	        r -= infectivity_of_agent[pos];
	        pos++;
	      }
	      Person* infector = infectious[pos];
	      printf("PICKED INFECTOR pos %d with infectivity %e\n", pos, infectivity_of_agent[pos]); fflush(stdout);

	      // successful transmission; create a new infection in infectee
          Transmission transmission = Transmission(infector, this, day);
	      infector->infect(infectee, disease_id, transmission);

	      FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) == 0,
				  "SEED infection day %i from %d to %d\n",
				  day, infector->get_id(), infectee->get_id());
	      FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) != 0,
				  "infection day %i of disease %i from %d to %d\n",
				  day, disease_id, infector->get_id(),
				  infectee->get_id());
      }
      // decrement counter (even if transmission fails)
      infectee_count[age]++;
    }
  }
  return;
}

void Place::pairwise_transmission_model(int day, int disease_id) {
  Disease* disease = Global::Diseases.get_disease(disease_id);
  double contact_prob = get_contact_rate(day, disease_id);

  // randomize the order of the infectious list
  FYShuffle<Person*>(this->enrollees);

  for(int infector_pos = 0; infector_pos < this->enrollees.size(); ++infector_pos) {
    Person* infector = this->enrollees[infector_pos];      // infectious individual
    if(!infector->get_health()->is_infectious(disease_id)) {
      continue;
    }

    for(int pos = 0; pos < this->enrollees.size(); ++pos) {
      if(pos == infector_pos) {
        continue;
      }
      Person* infectee = this->enrollees[ pos ];
      // if a non-infectious person is selected, pick from non_infectious vector
      // only proceed if person is susceptible
      if(infectee->is_susceptible(disease_id)) {
        // get the transmission probs for this infector/infectee pair
        double transmission_prob = get_transmission_prob(disease_id, infector, infectee);
        double infectivity = infector->get_infectivity(disease_id, day);
        // scale transmission prob by infectivity and contact prob
        transmission_prob *= infectivity * contact_prob;     
        attempt_transmission(transmission_prob, infector, infectee, disease_id, day);
      }
    } // end contact loop
  } // end infectious list loop
}

void Place::density_transmission_model(int day, int disease_id) {
  Disease* disease = Global::Diseases.get_disease(disease_id);

  // get references to susceptibles and infectious lists for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person*> & susceptibles = this->place_state_merge.get_susceptible_vector();
  std::vector<Person*> & infectious = this->place_state_merge.get_infectious_vector();

  // printf("DAY %d PLACE %s N %d susc %d inf %d\n",
  // day, this->get_label(), N, (int) susceptibles.size(), (int) infectious.size());

  double contact_prob = get_contact_rate(day, disease_id);
  int sus_hosts = susceptibles.size();
  int inf_hosts = infectious.size();
  int exposed = 0;
      
  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - contact_prob), inf_hosts);
  
  // select a number of hosts to be infected
  double expected_infections = sus_hosts * prob_infection;
  exposed = floor(expected_infections);
  double remainder = expected_infections - exposed;
  if(Random::draw_random() < remainder) {
    exposed++;
  }

  int infectee_count [inf_hosts];
  for (int i = 0; i < inf_hosts; ++i) {
    infectee_count[i] = 0;
  }
    
  int reached_max_infectees_count = 0;
  int number_infectious_hosts = inf_hosts;

  FRED_VERBOSE(1, "density_transmission: exposed = %d\n", exposed);
  // randomize the order of the susceptibles list
  FYShuffle<Person *>(susceptibles);

  for(int j = 0; j < exposed && j < sus_hosts && 0 < inf_hosts; ++j) {
    Person * infectee = susceptibles[j];
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());

    // only proceed if person is susceptible
    if(infectee->is_susceptible(disease_id)) {
      // select a random infector
      int infector_pos = Random::draw_random_int(0,inf_hosts-1);
      Person * infector = infectious[infector_pos];
      assert(infector->get_health()->is_infectious(disease_id));

      // get the transmission probs for this infector  pair
      double transmission_prob = infector->get_infectivity(disease_id, day);
      if(attempt_transmission(transmission_prob, infector, infectee, disease_id, day)) {
	    // successful transmission
	    infectee_count[infector_pos]++;
	    // if the infector has reached the max allowed, remove from infector list
	    if(Enable_Density_Transmission_Maximum_Infectees &&
	      Place::Density_Transmission_Maximum_Infectees <= infectee_count[infector_pos]) {
	      // effectively remove the infector from infector list
	      infectious[infector_pos] = infectious[inf_hosts-1];
	      int tmp = infectee_count[infector_pos];
	      infectee_count[infector_pos] = infectee_count[inf_hosts-1];
	      infectee_count[inf_hosts-1] = tmp;
	      inf_hosts--;
	      reached_max_infectees_count++;
        }
      }
    } else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
  if(reached_max_infectees_count) {
    FRED_VERBOSE(1, "day %d DENSITY TRANSMISSION place %s: %d with %d infectees out of %d infectious hosts\n",
		 day, this->label, reached_max_infectees_count,
		 Place::Density_Transmission_Maximum_Infectees, number_infectious_hosts);
  }
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC VECTOR TRANSMISSION MODEL
//
//////////////////////////////////////////////////////////


void Place::setup_vector_model() {
  // local state variables
  this->N_vectors = 0;
  this->N_hosts = 0;
  this->S_vectors = 0;
  for(int i = 0; i < DISEASE_TYPES; ++i) {
    this->E_vectors[i] = 0;
    this->I_vectors[i] = 0;
    this->place_seeds[i] = Global::Vectors->get_seeds(this,i);
    this->day_start_seed[i] = Global::Vectors->get_day_start_seed(this,i);
    this->day_end_seed[i] = Global::Vectors->get_day_end_seed(this,i);
  }	
  this->death_rate = 1.0/18.0;
  this->birth_rate = 1.0/18.0;
  this->bite_rate = 0.76;
  this->incubation_rate = 1.0/11.0;
  this->infection_efficiency = Global::Vectors->get_infection_efficiency();
  this->transmission_efficiency = Global::Vectors->get_transmission_efficiency();
  this->suitability = 1.0;
  this->temperature = 0;
  this->vectors_per_host = 0; // depends on the number of pupa and temperature
  this->pupae_per_host = 1.02; //Armenia average
  this->life_span = 18.0; // From Chao and longini
  this->sucess_rate = 0.83; // Focks
  this->female_ratio = 0.5;
  this->development_time = 1.0;
  set_temperature();
  this->N_vectors = this->N_orig * this->vectors_per_host;
  this->S_vectors = this->N_vectors;
  FRED_VERBOSE(1, "VECTOR_MODEL_SETUP: House %s temp %f vectors_per_host %f N_vectors %d N_orig %d\n", this->label, this->temperature, this->vectors_per_host, this->N_vectors, this->N_orig);
}

double Place::get_seeds(int dis, int day) {
  if((day < this->day_start_seed[dis]) ||( day > this->day_end_seed[dis])){
    return 0.0;
  } else {
    return this->place_seeds[dis];
  }
}

void Place::set_temperature(){
  //temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
  double temps[8] = {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92}; //temperatures
  double dev_times[8] = {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0}; //development times
  this->temperature = Global::Vectors->get_temperature(this);
  if(this->temperature > 32) {
    this->temperature = 32;
  }
  if(this->temperature <= 18) {
    this->vectors_per_host = 0;
  } else {
    for(int i = 0; i < 8; ++i) {
      if(this->temperature <= temps[i]) {
        //obtain the development time using linear interpolation
        this->development_time = dev_times[i - 1] + (dev_times[i] - dev_times[i - 1]) / (temps[i] - temps[i-1]) * (this->temperature - temps[i - 1]);
      }
    }
    this->vectors_per_host = this->pupae_per_host * this->female_ratio * this->sucess_rate * this->life_span / this->development_time;
  }
  FRED_VERBOSE(1, "SET TEMP: House %s temp %f vectors_per_host %f N_vectors %d N_orig %d\n", this->label, this->temperature, this->vectors_per_host, this->N_vectors, this->N_orig);
}

void Place::vector_transmission(int day, int disease_id) {

  // N_vectors is updated in update_vector_population()

  // N_hosts includes all visitors: infectious, susceptible, or neither
  this->N_hosts = this->unique_visitors.size();

  // infections of vectors by hosts
  if(this->vectors_not_infected_yet) {
    this->infect_vectors(day);
  }

  // transmission from vectors to hosts
  this->vectors_transmit_to_hosts(day, disease_id);
}


void Place::infect_vectors(int day) {

  // skip if there are no susceptible vectors
  if(this->S_vectors == 0) {
    return;
  }

  // find the percent distribution of infectious hosts
  int* infectious_hosts = new int [Global::Diseases.get_number_of_diseases()];
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    infectious_hosts[disease_id] = 0;
  }
  int total_infectious_hosts = 0;
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    // get references to infectious list for this disease_id
    this->place_state_merge = Place_State_Merge();
    this->place_state[disease_id].apply(this->place_state_merge);
    std::vector<Person*> & infectious = this->place_state_merge.get_infectious_vector();
    infectious_hosts[disease_id] = infectious.size();
    total_infectious_hosts += infectious_hosts[disease_id];
  }

  // return if there are no infectious hosts
  if(total_infectious_hosts == 0) {
    return;
  }

  FRED_VERBOSE(1, "infect_vectors entered S_vectors %d N_inf_hosts %d\n", this->S_vectors, total_infectious_hosts);

  // the vector infection model of Chao and Longini

  // decide on total number of vectors infected by any infectious host

  // each vectors's probability of infection
  double prob_infection = 1.0 - pow((1.0 - this->infection_efficiency), (this->bite_rate * total_infectious_hosts) / this->N_hosts);

  // select a number of vectors to be infected
  int total_infections = prob_infection * this->S_vectors;
  FRED_VERBOSE(1, "total infections %d\n", total_infections);

  // assign strain based on distribution of infectious hosts
  int newly_infected = 0;
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    int strain_infections = total_infections *((double)infectious_hosts[disease_id] / (double)total_infectious_hosts);
    this->E_vectors[disease_id] += strain_infections;
    this->S_vectors -= strain_infections;
    newly_infected += strain_infections;
  }
  this->vectors_not_infected_yet = false;
  FRED_VERBOSE(1, "newly_infected_vectors %d S %d \n", newly_infected,this-> S_vectors);
}

void Place::vectors_transmit_to_hosts(int day, int disease_id) {

  int e_hosts = 0;

  // get reference to susceptibles list for this disease_id
  this->place_state_merge = Place_State_Merge();
  this->place_state[disease_id].apply(this->place_state_merge);
  std::vector<Person*> & susceptibles = this->place_state_merge.get_susceptible_vector();

  int s_hosts = susceptibles.size();

  if(Global::Verbose > 1) {
    int R_hosts = get_recovereds(disease_id);
    FRED_VERBOSE(1, "day %d vector_update: %s S_vectors %d E_vectors %d I_vectors %d  N_vectors = %d N_host = %d S_hosts %d R_hosts %d Temp = %f dis %d\n",
		 day, this->label, S_vectors, E_vectors[disease_id], I_vectors[disease_id], N_vectors, 
		 N_hosts,susceptibles.size(),R_hosts,temperature,disease_id);
  }

  if(s_hosts == 0 || this->I_vectors[disease_id] == 0 || this->transmission_efficiency == 0.0) {
    return;
  }

  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - this->transmission_efficiency), (this->bite_rate * this->I_vectors[disease_id] / this->N_hosts));
  
  // select a number of hosts to be infected
  double expected_infections = s_hosts * prob_infection;
  e_hosts = floor(expected_infections);
  double remainder = expected_infections - e_hosts;
  if(Random::draw_random() < remainder) {
    e_hosts++;
  }
  FRED_VERBOSE(1, "transmit_to_hosts: E_hosts[%d] = %d\n", disease_id, e_hosts);
  // randomize the order of the susceptibles list
  FYShuffle<Person *>(susceptibles);
  FRED_VERBOSE(1,"Shuffle finished S_hosts size = %d\n", susceptibles.size());
  // get the disease object   
  Disease* disease = Global::Diseases.get_disease(disease_id);

  for(int j = 0; j < e_hosts && j < susceptibles.size(); ++j) {
    Person* infectee = susceptibles[j];
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());
    // NOTE: need to check if infectee already infected
    if(infectee->is_susceptible(disease_id)) {
      // create a new infection in infectee
      FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
      Transmission transmission = Transmission(NULL, NULL, day);
      transmission.set_initial_loads(disease->get_primary_loads(day));
      infectee->become_exposed(disease, transmission);

      // become unsusceptible to other diseases(?)
      for(int d = 0; d < DISEASE_TYPES; d++) {
        if(d != disease_id) {
          Disease* other_disease = Global::Diseases.get_disease(d);
          infectee->become_unsusceptible(other_disease);
        }
      }
    } else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
}

void Place::update_vector_population(int day) {
  if(this->N_vectors <= 0) {
    return;
  }
  FRED_VERBOSE(1,"update vector pop: day %d place %s initially: S %d, N: %d\n",day,this->label,S_vectors,N_vectors);

  int born_infectious[DISEASE_TYPES];
  int total_born_infectious=0;

  // new vectors are born susceptible
  this->S_vectors += floor(this->birth_rate * this->N_vectors - this->death_rate * S_vectors);
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n", this->S_vectors, this->N_vectors);
  // but some are infected
  for(int d=0;d<DISEASE_TYPES;d++){
    double seeds = this->get_seeds(d, day);
    born_infectious[d] = ceil(this->S_vectors * seeds);
    total_born_infectious += born_infectious[d];
    if(born_infectious[d] > 0) {
      FRED_VERBOSE(1,"vector_update_population:: Vector born infectious disease[%d] = %d \n",d, born_infectious[d]);
      FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
    }
  }
  // printf("PLACE %s SEEDS %d S_vectors %d DAY %d\n",this->label,total_born_infectious,S_vectors,day);
  this->S_vectors -= total_born_infectious;
  FRED_VERBOSE(1,"vector_update_population - seeds:: S_vector: %d, N_vectors: %d\n", this->S_vectors, this->N_vectors);
  // print this 
  if(total_born_infectious > 0){
    FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
  }
  if(this->S_vectors < 0) {
    this->S_vectors = 0;
  }
    
  // accumulate total number of vectors
  this->N_vectors = this->S_vectors;
  // we assume vectors can have at most one infection, if not susceptible
  for(int i = 0; i < DISEASE_TYPES; ++i) {
    // some die
    FRED_VERBOSE(1,"vector_update_population:: E_vectors[%d] = %d \n",i, this->E_vectors[i]);
    if(this->E_vectors[i] < 10){
      for(int k = 0; k < this->E_vectors[i]; ++k) {
	      double r = Random::draw_random(0,1);
	      if(r < this->death_rate){
	        this->E_vectors[i]--;
	      }
      }
    } else {
      this->E_vectors[i] -= floor(this->death_rate * this->E_vectors[i]);
    } 
    // some become infectious
    int become_infectious = 0;
    if(this->E_vectors[i] < 10) {
      for(int k = 0; k < this->E_vectors[i]; ++k) {
	      double r = Random::draw_random(0,1);
	      if(r < this->incubation_rate) {
	        become_infectious++;
	      }
      }
    } else {
      become_infectious = floor(this->incubation_rate * this->E_vectors[i]);
    }
      
    // int become_infectious = floor(incubation_rate * E_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: become infectious [%d] = %d, incubation rate: %f,E_vectors[%d] %d \n", i,
        become_infectious, this->incubation_rate, i, this->E_vectors[i]);
    this->E_vectors[i] -= become_infectious;
    if(this->E_vectors[i] < 0) this->E_vectors[i] = 0;
    // some die
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    this->I_vectors[i] -= floor(this->death_rate * this->I_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    // some become infectious
    this->I_vectors[i] += become_infectious;
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    // some were born infectious
    this->I_vectors[i] += born_infectious[i];
    FRED_VERBOSE(1,"vector_update_population::+= born infectious I_Vectors[%d] = %d,born infectious[%d] = %d \n", i,
        this->I_vectors[i], i, born_infectious[i]);
    if(this->I_vectors[i] < 0) {
      this->I_vectors[i] = 0;
    }

    // add to the total
    this->N_vectors += (this->E_vectors[i] + this->I_vectors[i]);
    FRED_VERBOSE(1, "update_vector_population entered S_vectors %d E_Vectors[%d] %d  I_Vectors[%d] %d N_Vectors %d\n",
        this->S_vectors, i, this->E_vectors[i], i, this->I_vectors[i], this->N_vectors);

    // register if any infectious vectors
    if(this->I_vectors[i]) {
      this->register_as_an_infectious_place(i);
    }
  }
}

char* Place::get_place_label(Place* p) {
   return ((p == NULL) ? (char*) "-1" : p->get_label());
 }

void Place::add_visitors_if_infectious(int day) {
  /*
  if (is_household()) {
    return;
  }
  if (is_neighborhood()) {
    return;
  }
  */
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); disease_id++) {
    if(this->infectious_bitset.test(disease_id)) {
      this->add_visitors_to_infectious_place(day, disease_id);
    }
  }
}

void Place::add_visitors_to_infectious_place(int day, int disease_id) {
  // ask each enrollee if s/he plans to visit today
  int size = this->enrollees.size();
  int not_here = 0;
  int sus = 0;
  int inf = 0;
  int not_inf = 0;
  for(int i = 0; i < size; ++i) {
    Person* person = get_enrollee(i);
    int status = person->get_visiting_health_status(this, day, disease_id);
    // status = 0 if not visiting, 1 if susceptible, 2 if infectious, 3 if non-susc/non-inf
    // printf("PERSON %d STATUS = %d\n", person->get_id(), status);
    switch(status) {
      case 0: // ignore if not visiting
        not_here++;
        break;
      case 1: // susceptible
        sus++;
        this->add_susceptible(disease_id, person);
        break;
      case 2: // infectious
        // do nothing -- already added.
        inf++;
        break;
      case 3: // not infectious and not susceptible
        not_inf++;
        this->add_host(person);
        break;
    }
  }
  /*
  if (is_neighborhood()) {
    printf("VISITORS day %d PLACE %s SIZE %d ",day, label, size);
    printf(" == not_here %d + sus %d + inf %d + not_inf %d\n",not_here,sus,inf,not_inf);
  }
  */
  assert(size == not_here + sus + inf + not_inf);
  // print_susceptibles(disease_id);
}

