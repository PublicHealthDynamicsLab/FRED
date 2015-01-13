/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Place.h
//

#ifndef _FRED_PLACE_H
#define _FRED_PLACE_H

#define HOUSEHOLD 'H'
#define NEIGHBORHOOD 'N'
#define SCHOOL 'S'
#define CLASSROOM 'C'
#define WORKPLACE 'W'
#define OFFICE 'O'
#define HOSPITAL 'M'
#define COMMUNITY 'X'

#include <new>
#include <vector>
#include <deque>
#include <map>
using namespace std;

#include "Population.h"
#include "Global.h"
class Cell;
class Person;

class Place {
public:
  Place() {}
  ~Place() {}

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
  void setup(const char *lab, fred::geo lon, fred::geo lat, Place *cont, Population *pop);

  /**
   * Get this place ready
   */
  virtual void prepare();

  /**
   * Perform a daily update of this place.  The daily count arrays will all be reset and the vectors
   * containing infectious and symptomatics will be cleared.
   */
  void update(int day);

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
  virtual void enroll(Person * per);

  /**
   * Remove a person from the place. This method decrements the number of people in
   * the place.
   *
   * @param per a pointer to a Person object that may be removed to the place
   */
  virtual void unenroll(Person * per);

  /**
   * Add a susceptible person to the place. This method adds the person to the susceptibles vector and
   * increments the number of susceptibles in the place (S).
   *
   * @param disease_id an integer representation of the disease
   * @param per a pointer to a Person object that will be added to the place for a given diease
   */
  void add_susceptible(int disease_id, Person * per);

  /**
   * Adds multiple susceptible persons to the place. Adds the persons contained in the
   * temporary _susceptibles vector to the place's susceptibles vector and
   * increments the number of susceptibles in the place (S).
   *
   * @param disease_id an integer representation of the disease
   * @param _susceptibles reference to vector of Person objects that will be added
   * to the place's susceptibles vector for a given diease
   */
  void add_susceptibles( int disease_id, std::vector< Person * > & _susceptibles );



  /**
   * Add a infectious person to the place. This method adds the person to the infectious vector and
   * increments the number of infectious in the place (I).
   *
   * @param disease_id an integer representation of the disease
   * @param per a pointer to a Person object that will be added to the place for a given diease
   */
  virtual void add_infectious(int disease_id, Person * per);

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
  virtual void spread_infection(int day, int disease_id);

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
  bool is_infectious(int disease_id) { return I[disease_id] > 0; }

  /**
   * Allow update of incidence vector.
   *
   * @param disease_id an integer representation of the disease
   * @@param strain a vector of integers representing strains
   * @param incr is unused at this time
   */
  void modify_incidence_count(int disease_id, vector<int> strain, int incr);

  /**
   * Allow update of prevalence vector.
   *
   * @param disease_id an integer representation of the disease
   * @@param strain a vector of integers representing strains
   * @param incr is unused at this time
   */
  void modify_prevalence_count(int disease_id, vector<int> strain, int incr);

  /**
   * <bold>Deprecated.</bold>
   */
  void print_stats(int day, int disease_id);
  
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
  int get_id() { return id; }

  /**
   * Get the label.
   *
   * @return the label
   */
  char * get_label() { return label; }

  /**
   * Get the type (H)OME, (W)ORK, (S)CHOOL, (C)OMMUNITY).
   *
   * @return the type
   */
  char get_type() { return type; }

  bool is_workplace() { return type == 'W'; }

  /**
   * Get the latitude.
   *
   * @return the latitude
   */
  fred::geo get_latitude() { return latitude; }

  /**
   * Get the longitude.
   *
   * @return the longitude
   */
  fred::geo get_longitude() { return longitude; }

  /**
   * Get the count of (S)usceptibles for a given diease in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the suceptible count for the given diease
   */
  int get_S(int disease_id) { return S[disease_id]; }

  /**
   * Get the count of (I)nfectious for a given diease in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the infectious count for the given diease
   */
  int get_I(int disease_id) { return (int) (infectious[disease_id].size()); }

  /**
   * Get the count of (S)ymptomatics for a given diease in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the symptomatic count for the given diease
   */
  int get_symptomatic(int disease_id) { return Sympt[disease_id]; }

  /**
   * Get the count of agents in this place.
   *
   * @return the count of agents
   */
  int get_size() { return N; }

  /**
   * Get the simulation day (an integer value of days from the start of the simulation) when the place will close.
   *
   * @return the close_date
   */
  int get_close_date() { return close_date; }

  /**
   * Get the simulation day (an integer value of days from the start of the simulation) when the place will open.
   *
   * @return the open_date
   */
  int get_open_date() { return open_date; }

  /**
   * Get the population.
   *
   * @return the population
   */
  Population *get_population() { return population; }

  /**
   * Get the number of cases of a given diease for day.
   * The member variable cases gets reset when <code>update()</code> is called, which for now is on a daily basis.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of cases for a given diease
   */
  int get_daily_cases(int disease_id) { return cases[disease_id]; }

  /**
   * Get the number of deaths from a given diease for a day.
   * The member variable deaths gets reset when <code>update()</code> is called, which for now is on a daily basis.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of deaths for a given diease
   */
  int get_daily_deaths(int disease_id) { return deaths[disease_id]; }

  /**
   * Get the number of cases of a given diease for the simulation thus far.
   * This value is not reset when <code>update()</code> is called.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of cases for a given diease
   */
  int get_total_cases(int disease_id) { return total_cases[disease_id]; }

  /**
   * Get the number of deaths from a given diease for the simulation thus far.
   * This value is not reset when <code>update()</code> is called.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of deaths for a given diease
   */
  int get_total_deaths(int disease_id) { return total_deaths[disease_id]; }

  /**
   * Get the number of cases of a given diease for the simulation thus far divided by the
   * number of agents in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of rate of cases per people for a given diease
   */
  double get_incidence_rate(int disease_id) { return (double) total_cases[disease_id] / (double) N; }
  
  /**
   * Get the clincal attack rate = 100 * number of cases thus far divided by the
   * number of agents in this place.
   *
   * @param disease_id an integer representation of the disease
   * @return the count of rate of cases per people for a given diease
   */
  double get_clinical_attack_rate(int disease_id) { return (100.0*total_cases[disease_id])/ (double) N; }
  
  /**
   * Set the id.
   *
   * @param n the new id
   */
  //void set_id(int n) { id = n; }

  /**
   * Set the type.
   *
   * @param t the new type
   */
  void set_type(char t) { type = t; }

  /**
   * Set the latitude.
   *
   * @param x the new latitude
   */
  void set_latitude(fred::geo x) { latitude = x; }

  /**
   * Set the longitude.
   *
   * @param x the new longitude
   */
  void set_longitude(fred::geo x) { longitude = x; }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will close.
   *
   * @param day the simulation day when the place will close
   */
  void set_close_date(int day) { close_date = day; }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will open.
   *
   * @param day the simulation day when the place will open
   */
  void set_open_date(int day) { open_date = day; }

  /**
   * Set the population.
   *
   * @param p the new population
   */
  void set_population(Population *p) {  population = p; }

  /**
   * Set the container.
   *
   * @param cont the new container
   */
  void set_container(Place *cont) { container = cont; }

  Place * get_container() { return container; }

  /**
   * Increment the cases count. Note that the cases variable will be reset when <code>update()</code> is called
   */
  void add_case() { cases++; }

  /**
   * Increment the deaths count. Note that the deaths variable will be reset when <code>update()</code> is called
   */
  void add_deaths() { deaths++; }

  /**
   * Get the grid_cell where this place is.
   *
   * @return a pointer to the grid_cell where this place is
   */
  Cell * get_grid_cell() { return grid_cell; }

  /**
   * Set the grid_cell where this place will be.
   *
   * @param p the new grid_cell
   */
  void set_grid_cell(Cell *p) { grid_cell = p; }
  
  Place * select_neighborhood(double community_prob, double community_distance, double local_prob);

  int get_days_infectious() { return days_infectious; }

  double get_attack_rate() { return(N?(100.0*total_infections)/(double)N:0.0); }

protected:
  int id;                 // place id
  char label[32];         // external id
  char type;              // HOME, WORK, SCHOOL, COMMUNITY
  Place *container;       // container place
  fred::geo latitude;     // geo location
  fred::geo longitude;    // geo location
  int N;                  // total number of potential visitors
  vector <Person *> *susceptibles;    // list of susceptible visitors
  vector <Person *> *infectious;      // list of infectious visitors
  int *S;                 // susceptible count
  int *I;                 // infectious count
  int *Sympt;             // symptomatics count
  int close_date;         // this place will be closed during:
  int open_date;          //   [close_date, open_date)
  int * cases;            // symptomatic cases today
  int * deaths;           // deaths today
  int * total_cases;      // total symptomatic cases
  int * total_deaths;     // total deaths
  Population *population;
  Cell * grid_cell;       // geo grid_cell for this place
  vector< map<int, int> > incidence;
  vector< map<int, int> > prevalence;
  int days_infectious;
  int total_infections;

  double get_contact_rate(int day, int disease_id);
  int get_contact_count(Person * infector, int disease_id, int day, double contact_rate);
  void attempt_transmission(double transmission_prob, Person * infector, Person * infectee, int disease_id, int day);

  fred::Mutex mutex;

  // Place_List, Grid and Cell are friends so that they can access
  // the Place Allocator.  
  friend class Place_List;
  friend class Grid;
  friend class Cell;

  // friend Place_List can assign id
  void set_id( int _id ) {
    id = _id;
  }

  // Place Allocator reserves chunks of memory and hands out pointers for use
  // with placement new
  template< typename Place_Type >
  struct Allocator {

    Place_Type * allocation_array;
    int current_allocation_size, current_allocation_index;
    int number_of_contiguous_blocks_allocated, remaining_allocations;
    int allocations_made;

    Allocator() {
      remaining_allocations = 0;
      number_of_contiguous_blocks_allocated = 0;
      allocations_made = 0;
    }

    bool reserve( int n = 1 ) {
      if ( remaining_allocations == 0 ) {
        current_allocation_size = n;
        allocation_array = new Place_Type[ n ];
        remaining_allocations = n; 
        current_allocation_index = 0;
        ++( number_of_contiguous_blocks_allocated );
        allocations_made += n;
        return true;
      }
      return false;
    }

    Place_Type * get_free() {
      if ( remaining_allocations == 0 ) {
        reserve();
      }
      Place_Type * place_pointer = allocation_array + current_allocation_index;  
      --( remaining_allocations );
      ++( current_allocation_index );
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
  };

};
#endif // _FRED_PLACE_H
