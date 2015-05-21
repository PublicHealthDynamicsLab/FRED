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
// File: Place.h
//

#ifndef _FRED_PLACE_H
#define _FRED_PLACE_H

#include <new>
#include <vector>
#include <deque>
#include <map>
#include <set>
using namespace std;

#include "Population.h"
#include "Random.h"
#include "Global.h"
#include "State.h"
#include "Geo_Utils.h"
#define DISEASE_TYPES 4

typedef std::vector <Place *> place_vec;

class Neighborhood_Patch;
class Person;

struct Place_State {

  fred::Spin_Mutex mutex;
  // working copies of susceptibles and infectious lists
  std::vector<Person *> susceptibles;
  std::vector<Person *> infectious;

  void add_susceptible(Person* p) {
    fred::Spin_Lock lock(mutex);
    this->susceptibles.push_back(p);
  }

  void clear_susceptibles() {
    fred::Spin_Lock lock(mutex);
    this->susceptibles.clear();
  }

  size_t susceptibles_size() {
    fred::Spin_Lock lock(mutex);
    return this->susceptibles.size();
  }

  std::vector<Person*> & get_susceptible_vector() {
    return this->susceptibles;
  }

  void add_infectious(Person* p) {
    fred::Spin_Lock lock(mutex);
    this->infectious.push_back(p);
  }

  void clear_infectious() {
    fred::Spin_Lock lock(mutex);
    this->infectious.clear();
  }

  size_t infectious_size() {
    fred::Spin_Lock lock(mutex);
    return this->infectious.size();
  }

  std::vector<Person*> & get_infectious_vector() {
    return this->infectious;
  }

  void clear() {
    susceptibles.clear();
    infectious.clear();
  }

  void reset() {
    if(susceptibles.size() > 0) {
      this->susceptibles = std::vector<Person*>();
    }
    if(this->infectious.size() > 0) {
      this->infectious = std::vector<Person*>();
    }
  }

};

struct Place_State_Merge : Place_State {

  void operator()(const Place_State & state) {
    fred::Spin_Lock lock(mutex);
    this->susceptibles.insert(this->susceptibles.end(),
                              state.susceptibles.begin(),
                              state.susceptibles.end());
    this->infectious.insert(this->infectious.end(),
                            state.infectious.begin(),
                            state.infectious.end());
  }
};


class Place {

public:
  
  // place-specific transmission mode parameters
  static bool Enable_Neighborhood_Density_Transmission;
  static bool Enable_Density_Transmission_Maximum_Infectees;
  static int Density_Transmission_Maximum_Infectees;

  // static seasonal transmission parameters
  static double Seasonal_Reduction;
  static double * Seasonality_multiplier;

  // place type codes
  static char HOUSEHOLD;
  static char NEIGHBORHOOD;
  static char SCHOOL;
  static char CLASSROOM;
  static char WORKPLACE;
  static char OFFICE;
  static char HOSPITAL;
  static char COMMUNITY;

  virtual ~Place() {}

  /**
   *  Sets the id, label, logitude, latitude , container and population of this Place
   *  Allocates disease-related memory for this place
   *
   *  @param lab this Place's label
   *  @param lon this Place's longitude
   *  @param lat this Place's latitude
   *  @param cont this Place's container
   *  @param pop this Place's population
   */
  void setup(const char* lab, fred::geo lon, fred::geo lat, Place* cont, Population* pop);

  static void initialize_static_variables();

  /**
   * Get this place ready
   */
  virtual void prepare();

  /**
   * Perform a daily update of this place.  The vectors
   * containing infectious and symptomatics will be cleared.
   */
  virtual void update(int day);

  void reset_place_state(int disease_id);

  /**
   * The daily count arrays will all be reset.
   */
  void reset_visualization_data(int day);

  void reset_vector_data(int day);

  /**
   * Display the information for a given disease.
   *
   * @param disease an integer representation of the disease
   */
  virtual void print(int disease_id);

  /**
   * Add a person to the place. This method increments the number of people in
   * the place.
   *
   * @param per a pointer to a Person object that may be added to the place
   */
  virtual void enroll(Person* per);

  /**
   * Remove a person from the place. This method decrements the number of people in
   * the place.
   *
   * @param per a pointer to a Person object that may be removed to the place
   */
  virtual void unenroll(Person* per);

  /**
   * Get the number of adults in the household.
   * @return the number of adults
   */
  int get_adults();

  /**
   * Get the number of children in the household.
   * @return the number of children
   */
  int get_children();

  void register_as_an_infectious_place(int disease_id);

  void add_visitors_if_infectious(int day);
  void add_visitors_to_infectious_place(int day, int disease_id);

  /**
   * Add a susceptible person to the place. This method adds the person to the susceptibles vector.
   *
   * @param disease_id an integer representation of the disease
   * @param per a pointer to a Person object that will be added to the place for a given diease
   */
  void add_susceptible(int disease_id, Person* per);
  void add_nonsusceptible(int disease_id, Person* per);

  /**
   * Add a infectious person to the place. This method adds the person to the infectious vector.
   *
   * @param disease_id an integer representation of the disease
   * @param per a pointer to a Person object that will be added to the place for a given diease
   */
  void add_infectious(int disease_id, Person* per);

  /**
   * Prints the id of every person in the susceptible vector for a given diease.
   *
   * @param disease_id an integer representation of the disease
   */
  void print_susceptibles(int disease_id);

  /**
   * Prints the id of every person in the infectious vector for a given diease.
   *
   * @param disease_id an integer representation of the disease
   */
  void print_infectious(int disease_id);

  /**
   * Attempt to spread the infection for a given diease on a given day.
   *
   * @param day the simulation day
   * @param disease_id an integer representation of the disease
   */
  void spread_infection(int day, int disease_id);

  void default_transmission_model(int day, int disease_id);
  void age_based_transmission_model(int day, int disease_id);
  void pairwise_transmission_model(int day, int disease_id);
  void density_transmission_model(int day, int disease_id);

  /**
   * Is the place open on a given day?
   *
   * @param day the simulation day
   * @return <code>true</code> if the place is open; <code>false</code> if not
   */
  bool is_open(int day);

  /**
   * Test whether or not any infectious people are in this place.
   *
   * @return <code>true</code> if any infectious people are here; <code>false</code> if not
   */
  bool is_infectious(int disease_id) {
    return this->infectious_bitset.test(disease_id);
  }
  
  bool is_infectious() {
    return this->infectious_bitset.any();
  }
  
  bool is_human_infectious(int disease_id) {
    return this->human_infectious_bitset.test(disease_id);
  }  

  void set_human_infectious (int disease_id) {
    if(!(this->human_infectious_bitset.test(disease_id))) {
      this->human_infectious_bitset.set(disease_id);
    }
  }
    
  void reset_human_infectious() {
    this->human_infectious_bitset.reset();
  }
    
  void set_exposed(int disease_id) {
    this->exposed_bitset.set(disease_id);
  }
  
  void reset_exposed() {
    this->exposed_bitset.reset();
  }

  bool is_exposed(int disease_id) {
    return this->exposed_bitset.test(disease_id);
  }
  
  void set_recovered(int disease_id) {
    this->recovered_bitset.set(disease_id);
  }
  
  void reset_recovered() {
    this->recovered_bitset.reset();
  }
  
  bool is_recovered(int disease_id) {
    return this->recovered_bitset.test(disease_id);
  }
  
  /**
   * Sets the static variables for the class from the parameter file for a given number of disease_ids.
   *
   * @param disease_id an integer representation of the disease
   */
  virtual void get_parameters(int disease_id) = 0;

  /**
   * Get the age group for a person given a particular disease_id.
   *
   * @param disease_id an integer representation of the disease
   * @param per a pointer to a Person object
   * @return the age group for the given person for the given diease
   */
  virtual int get_group(int disease_id, Person * per) = 0;

  /**
   * Get the transmission probability for a given diease between two Person objects.
   *
   * @param disease_id an integer representation of the disease
   * @param i a pointer to a Person object
   * @param s a pointer to a Person object
   * @return the probability that there will be a transmission of disease_id from i to s
   */
  virtual double get_transmission_prob(int disease_id, Person * i, Person * s) = 0;

  /**
   * Get the contacts for a given diease.
   *
   * @param disease_id an integer representation of the disease
   * @return the contacts per day for the given diease
   */
  virtual double get_contacts_per_day(int disease_id) = 0; // access functions

  /**
   * Determine if the place should be open. It is dependent on the disease_id and simulation day.
   *
   * @param day the simulation day
   * @param disease_id an integer representation of the disease
   * @return <code>true</code> if the place should be open; <code>false</code> if not
   */
  virtual bool should_be_open(int day, int disease_id) = 0;

  /**
   * Get the id.
   * @return the id
   */
  int get_id() {
    return this->id;
  }

  /**
   * Get the label.
   *
   * @return the label
   */
  char* get_label() {
    return this->label;
  }

  /**
   * Get the type (H)OME, (W)ORK, (S)CHOOL, (C)OMMUNITY).
   *
   * @return the type
   */
  char get_type() {
    return this->type;
  }

  // test place types
  bool is_household() {
    return this->type == Place::HOUSEHOLD;
  }
  
  bool is_neighborhood() {
    return this->type == Place::NEIGHBORHOOD;
  }
  
  bool is_school() {
    return this->type == Place::SCHOOL;
  }
  
  bool is_classroom() {
    return this->type == Place::CLASSROOM;
  }
  
  bool is_workplace(){
    return this->type == Place::WORKPLACE;
  }
  
  bool is_office() {
    return this->type == Place::OFFICE;
  }
  
  bool is_hospital() {
    return this->type == Place::HOSPITAL;
  }
  
  bool is_community() {
    return this->type == Place::COMMUNITY;
  }

  // test place subtypes
  bool is_college(){
    return this->subtype == fred::PLACE_SUBTYPE_COLLEGE;
  }
  
  bool is_prison(){
    return this->subtype == fred::PLACE_SUBTYPE_PRISON;
  }
  
  bool is_nursing_home(){
    return this->subtype == fred::PLACE_SUBTYPE_NURSING_HOME;
  }
  
  bool is_military_base(){
    return this->subtype == fred::PLACE_SUBTYPE_MILITARY_BASE;
  }
    
  bool is_healthcare_clinic(){
    return this->subtype == fred::PLACE_SUBTYPE_HEALTHCARE_CLINIC;
  }
    
  bool is_group_quarters() {
    return (is_college() || is_prison() || is_military_base() || is_nursing_home());
  }

  // test for household types
  bool is_college_dorm(){
    return is_household() && is_college();
  }
  
  bool is_prison_cell(){
    return  is_household() && is_prison();
  }
  
  bool is_military_barracks() {
    return is_household() && is_military_base();
  }

  /**
   * Get the latitude.
   *
   * @return the latitude
   */
  fred::geo get_latitude() {
    return this->latitude;
  }

  /**
   * Get the longitude.
   *
   * @return the longitude
   */
  fred::geo get_longitude() {
    return this->longitude;
  }

  double get_distance(Place *place) {
    double x1 = this->get_x();
    double y1 = this->get_y();
    double x2 = place->get_x();
    double y2 = place->get_y();
    double distance = sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
    return distance;
  }

  /**
   * Get the count of agents in this place.
   *
   * @return the count of agents
   */
  int get_size() {
    return this->N;
  }

  int get_orig_size() {
    return this->N_orig;
  }

  /**
   * Get the simulation day (an integer value of days from the start of the simulation) when the place will close.
   *
   * @return the close_date
   */
  int get_close_date() {
    return this->close_date;
  }

  /**
   * Get the simulation day (an integer value of days from the start of the simulation) when the place will open.
   *
   * @return the open_date
   */
  int get_open_date() {
    return this->open_date;
  }

  /**
   * Get the population.
   *
   * @return the population
   */
  Population* get_population() {
    return this->population;
  }

  /**
   * Set the type.
   *
   * @param t the new type
   */
  void set_type(char t) {
    this->type = t;
  }

  /**
   * Set the latitude.
   *
   * @param x the new latitude
   */
  void set_latitude(fred::geo x) {
    this->latitude = x;
  }

  /**
   * Set the longitude.
   *
   * @param x the new longitude
   */
  void set_longitude(fred::geo x) {
    this->longitude = x;
  }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will close.
   *
   * @param day the simulation day when the place will close
   */
  void set_close_date(int day) {
    this->close_date = day;
  }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will open.
   *
   * @param day the simulation day when the place will open
   */
  void set_open_date(int day) {
    this->open_date = day;
  }

  /**
   * Set the population.
   *
   * @param p the new population
   */
  void set_population(Population* p) {
    this->population = p;
  }

  /**
   * Set the container.
   *
   * @param cont the new container
   */
  void set_container(Place* cont) {
    this->container = cont;
  }

  /**
   * Get the container.
   *
   * @return a pointer to this Place's container
   */
  Place* get_container() {
    return this->container;
  }

  /**
   * Get the patch where this place is.
   *
   * @return a pointer to the patch where this place is
   */
  Neighborhood_Patch* get_patch() {
    return this->patch;
  }

  /**
   * Set the patch where this place will be.
   *
   * @param p the new patch
   */
  void set_patch(Neighborhood_Patch* p) {
    this->patch = p;
  }
  
  int get_output_count(int disease_id, int output_code);

  void turn_workers_into_teachers(Place* school);
  void reassign_workers(Place* place);

  double get_x() {
    return Geo_Utils::get_x(this->longitude);
  }

  double get_y() {
    return Geo_Utils::get_y(this->latitude);
  }

  void add_new_infection(int disease_id) {
    #pragma omp atomic
    this->new_infections[disease_id]++;
    #pragma omp atomic
    this->total_infections[disease_id]++;
  }
  
  void add_current_infection(int disease_id) {
    #pragma omp atomic
    this->current_infections[disease_id]++;
  }
  
  void add_new_symptomatic_infection(int disease_id) {
    #pragma omp atomic
    this->new_symptomatic_infections[disease_id]++;
    #pragma omp atomic
    this->total_symptomatic_infections[disease_id]++;
  }
  
  void add_current_symptomatic_infection(int disease_id) {
    #pragma omp atomic
    this->current_symptomatic_infections[disease_id]++;
  }
  
  int get_new_infections(int disease_id) {
    return this->new_infections[disease_id];
  }

  int get_current_infections(int disease_id) {
    return this->current_infections[disease_id];
  }

  int get_total_infections(int disease_id) {
    return this->total_infections[disease_id];
  }

  int get_new_symptomatic_infections(int disease_id) {
    return this->new_symptomatic_infections[disease_id];
  }

  int get_current_symptomatic_infections(int disease_id) {
    return this->current_symptomatic_infections[disease_id];
  }

  int get_total_symptomatic_infections(int disease_id) {
    return this->total_symptomatic_infections[disease_id];
  }

  int get_current_infectious_visitors(int disease_id) {
    return this->current_infectious_visitors[disease_id];
  }

  int get_current_symptomatic_visitors(int disease_id) {
    return this->current_symptomatic_visitors[disease_id];
  }

  /**
   * Get the number of cases of a given disease for day.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of cases for a given diease
   */
  int get_current_cases(int disease_id) {
    return get_current_symptomatic_visitors(disease_id);
  }

  /**
   * Get the number of deaths from a given disease for a day.
   * The member variable deaths gets reset when <code>update()</code> is called, which for now is on a daily basis.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of deaths for a given disease
   */
  int get_new_deaths(int disease_id) {
    return 0 /* new_deaths[disease_id] */;
  }

  int get_recovereds(int disease_id);

  /**
   * Get the number of cases of a given disease for the simulation thus far.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of cases for a given disease
   */
  int get_total_cases(int disease_id) {
    return this->total_symptomatic_infections[disease_id];
  }

  /**
   * Get the number of deaths from a given disease for the simulation thus far.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of deaths for a given disease
   */
  int get_total_deaths(int disease_id) {
    return 0 /* total_deaths[disease_id] */;
  }

  /**
   * Get the number of cases of a given disease for the simulation thus far divided by the
   * number of agents in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of rate of cases per people for a given disease
   */
  double get_incidence_rate(int disease_id) {
    return (double)this->total_symptomatic_infections[disease_id] / (double)this->N;
  }
  
  /**
   * Get the clincal attack rate = 100 * number of cases thus far divided by the
   * number of agents in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of rate of cases per people for a given disease
   */
  double get_symptomatic_attack_rate(int disease_id) {
    return (100.0 * this->total_symptomatic_infections[disease_id]) / (double) this->N;
  }
  
  /**
   * Get the attack rate = 100 * number of infections thus far divided by the
   * number of agents in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of rate of cases per people for a given disease
   */
  double get_attack_rate(int disease_id) {
    return(this->N ? (100.0 * this->total_infections[disease_id]) / (double)this->N : 0.0);
  }

  int get_first_day_infectious() {
    return this->first_day_infectious;
  }

  int get_last_day_infectious() {
    return this->last_day_infectious;
  }

  Person* get_enrollee(int i) {
    return this->enrollees[i];
  }

  int get_enrollee_index(Person* person) {
    for (int i = 0; i < enrollees.size(); i++) {
      if (enrollees[i] == person) {
        return i;
      }
    }
    return -1;
  }

  void set_index(int _index) {
    index = _index;
  }
  
  int get_index() {
    return index;
  }

  void set_subtype(fred::place_subtype _subtype) {
    subtype = _subtype;
  }
  
  fred::place_subtype get_subtype() {
    return subtype;
  }
  
  int get_staff_size() {
    return staff_size;
  }
  
  void set_staff_size(int _staff_size) {
    staff_size = _staff_size;
  }

  // vector transmission model
  void setup_vector_model();
  void set_temperature();
  void vector_transmission(int day, int disease_id);
  void infect_vectors(int day);
  void vectors_transmit_to_hosts(int day, int disease_id);
  void update_vector_population(int day);
  int get_vector_population_size() {
    return this->N_vectors;
  }
    
  bool has_infectious_vectors(int disease_id) {
    return this->I_vectors[disease_id] > 0;
  }
    
  // int get_number_of_infected_hosts(int disease_id) { return E_hosts[disease_id]; }
  double get_temperature() {
    return this->temperature;
  }
    
  double get_seeds(int dis, int day);
  int get_infected_vectors() {
    int n = 0;
    for(int i = 0; i < DISEASE_TYPES; ++i) {
      n += this->E_vectors[i] + this->I_vectors[i];
    }
    return n;
  }

  int get_infectious_vectors() {
    int n = 0;
    for (int i = 0; i < DISEASE_TYPES; ++i) {
      n += this->I_vectors[i];
    }
    return n;
  }

  void add_host(Person* person) {
    this->unique_visitors.insert(person);
  }

  static void clear_infectious_places(int capacity) {
    int size = infectious_places.size();
    for (int i = 0; i < size; i++) {
      Place *place = infectious_places[i];
      place->is_registered_as_an_infectous_place = false;
    }
    infectious_places.clear();
    infectious_places.reserve(capacity);
  }

  static int count_infectious_places() {
    return infectious_places.size();
  }

  static Place* get_infectious_place(int n) {
    return infectious_places[n];
  }
  
  int get_household_fips() {
  	return this->household_fips;
  }

  void set_household_fips(int input_fips) {
  	this->household_fips = input_fips;
  }
  
  void set_county_index(int _county_index) {
    this->county_index = _county_index;
  }

  int get_county_index() {
    return this->county_index;
  }

  void set_census_tract_index(int _census_tract_index) {
    this->census_tract_index = _census_tract_index;
  }

  int get_census_tract_index() {
    return this->census_tract_index;
  }
  
  static char* get_place_label(Place* p);

protected:
  // list of places that are infectious today
  static place_vec infectious_places;

  // true if this place is on the above list
  bool is_registered_as_an_infectous_place;

  //  collects list of susceptible visitors (per disease)
  //  collects list of infectious visitors (per disease)
  State< Place_State > place_state[Global::MAX_NUM_DISEASES];

  // temporary working copy of:
  //  list of susceptible visitors (per disease); size of which gives the susceptibles count
  //  list of infectious visitors (per disease); size of which gives the infectious count
  Place_State_Merge place_state_merge;

  // track whether or not place is infectious with each disease
  fred::disease_bitset infectious_bitset; 
  fred::disease_bitset human_infectious_bitset;
  fred::disease_bitset recovered_bitset; 
  fred::disease_bitset exposed_bitset; 

  char label[32];         // external id
  char type;              // HOME, WORK, SCHOOL, COMMUNITY, etc;
  fred::place_subtype subtype;
  char worker_profile;
  int id;                 // place id
  Place* container;       // container place
  fred::geo latitude;     // geo location
  fred::geo longitude;    // geo location
  vector <Person*> enrollees;
  int close_date;         // this place will be closed during:
  int open_date;          //   [close_date, open_date)
  int N;                  // total number of potential visitors
  int N_orig;		  // orig number of potential visitors
  double intimacy;	  // prob of intimate contact
  static double** prob_contact;
  int index;		  // index for households
  int staff_size;			// outside workers in this place

  int household_fips;
  int county_index;
  int census_tract_index;
  
  Population* population;
  Neighborhood_Patch* patch;       // geo patch for this place
  
  // infection stats
  int new_infections[Global::MAX_NUM_DISEASES]; // new infections today
  int current_infections[Global::MAX_NUM_DISEASES]; // current active infections today
  int total_infections[Global::MAX_NUM_DISEASES]; // total infections over all time

  int new_symptomatic_infections[Global::MAX_NUM_DISEASES]; // new sympt infections today
  int current_symptomatic_infections[Global::MAX_NUM_DISEASES]; // current active sympt infections
  int total_symptomatic_infections[Global::MAX_NUM_DISEASES]; // total sympt infections over all time

  // these counts refer to today's visitors:
  int current_infectious_visitors[Global::MAX_NUM_DISEASES]; // total infectious visitors today
  int current_symptomatic_visitors[Global::MAX_NUM_DISEASES]; // total sympt infections today

  // NOT IMPLEMENTED YET:
  // int new_deaths[ Global::MAX_NUM_DISEASES ];	    // deaths today
  // int total_deaths[ Global::MAX_NUM_DISEASES ];     // total deaths

  int first_day_infectious;
  int last_day_infectious;

  double get_contact_rate(int day, int disease_id);
  int get_contact_count(Person* infector, int disease_id, int day, double contact_rate);
  bool attempt_transmission(double transmission_prob, Person * infector, Person * infectee, int disease_id, int day);

  // Place_List, Neighborhood_Layer and Neighborhood_Patch are friends so that they can access
  // the Place Allocator.  
  friend class Place_List;
  friend class Neighborhood_Layer;
  friend class Neighborhood_Patch;

  // friend Place_List can assign id
  void set_id(int _id) {
    this->id = _id;
  }

  void add_infectious_visitor(int disease_id) { 
    #pragma omp atomic
    this->current_infectious_visitors[disease_id]++;
  }
  
  void add_symptomatic_visitor(int disease_id) {
    #pragma omp atomic
    this->current_symptomatic_visitors[disease_id]++;
  }
  
  // data for vector transmission model
  double death_rate;
  double birth_rate;
  double bite_rate;
  double incubation_rate;
  double suitability;
  double transmission_efficiency;
  double infection_efficiency;

  // vectors per host
  double temperature;
  double vectors_per_host;
  double pupae_per_host;
  double life_span;
  double sucess_rate;
  double female_ratio;
  double development_time;

  // counts for vectors
  int N_vectors;
  int S_vectors;
  int E_vectors[DISEASE_TYPES];
  int I_vectors[DISEASE_TYPES];
  // proportion of imported or born infectious
  double place_seeds[DISEASE_TYPES];
  // day on and day off of seeding mosquitoes in the patch
  int day_start_seed[DISEASE_TYPES];
  int day_end_seed[DISEASE_TYPES];

  std::set<Person *> unique_visitors;

  // counts for hosts
  int N_hosts;
  bool vectors_not_infected_yet;

  // Place Allocator reserves chunks of memory and hands out pointers for use
  // with placement new
  template<typename Place_Type>
  struct Allocator {
    Place_Type* allocation_array;
    int current_allocation_size;
    int current_allocation_index;
    int number_of_contiguous_blocks_allocated;
    int remaining_allocations;
    int allocations_made;

    Allocator() {
      remaining_allocations = 0;
      number_of_contiguous_blocks_allocated = 0;
      allocations_made = 0;
      current_allocation_index = 0;
      current_allocation_size = 0;
      allocation_array = NULL;
    }

    bool reserve( int n = 1 ) {
      if(remaining_allocations == 0) {
        current_allocation_size = n;
        allocation_array = new Place_Type[n];
        remaining_allocations = n; 
        current_allocation_index = 0;
        ++(number_of_contiguous_blocks_allocated);
        allocations_made += n;
        return true;
      }
      return false;
    }

    Place_Type* get_free() {
      if(remaining_allocations == 0) {
        reserve();
      }
      Place_Type* place_pointer = allocation_array + current_allocation_index;
      --(remaining_allocations);
      ++(current_allocation_index);
      return place_pointer;
    }

    int get_number_of_remaining_allocations() {
      return remaining_allocations;
    }

    int get_number_of_contiguous_blocks_allocated() {
      return number_of_contiguous_blocks_allocated;
    }

    int get_number_of_allocations_made() {
      return allocations_made;
    }

    Place_Type * get_base_pointer() {
      return allocation_array;
    }

    int size() {
      return allocations_made;
    }
  }; // end Place Allocator

};


#endif // _FRED_PLACE_H
