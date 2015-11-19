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

#include <vector>


#include "Geo.h"
#include "Global.h"
#include "Mixing_Group.h"

using namespace std;

class Neighborhood_Patch;
class Person;

class Place : public Mixing_Group {

public:
  
  // place type codes
  static char TYPE_HOUSEHOLD;
  static char TYPE_NEIGHBORHOOD;
  static char TYPE_SCHOOL;
  static char TYPE_CLASSROOM;
  static char TYPE_WORKPLACE;
  static char TYPE_OFFICE;
  static char TYPE_HOSPITAL;
  static char TYPE_COMMUNITY;
  static char TYPE_UNSET;

  static char SUBTYPE_NONE;
  static char SUBTYPE_COLLEGE;
  static char SUBTYPE_PRISON;
  static char SUBTYPE_MILITARY_BASE;
  static char SUBTYPE_NURSING_HOME;
  static char SUBTYPE_HEALTHCARE_CLINIC;
  static char SUBTYPE_MOBILE_HEALTHCARE_CLINIC;

  /**
   * Default constructor
   * Note: really only used by Allocator
   */
  Place();

  /**
   * Constructor with necessary parameters
   */
  Place(const char* lab, fred::geo lon, fred::geo lat);
  virtual ~Place() {
    if(this->vector_disease_data != NULL) {
      delete this->vector_disease_data;
    }
  }

  virtual void print(int disease_id);

  virtual void prepare();

  // daily update
  virtual void update(int sim_day);
  void reset_visualization_data(int sim_day);

  virtual bool is_open(int sim_day) {
    return true;
  }

  /**
   * Get the transmission probability for a given disease between two Person objects.
   *
   * @see Mixing_Group::get_transmission_probability(int disease_id, Person* i, Person* s)
   */
  virtual double get_transmission_probability(int disease_id, Person* i, Person* s) {
    return 1.0;
  }

  virtual double get_contacts_per_day(int disease_id) = 0; // access functions

  double get_contact_rate(int day, int disease_id);

  int get_contact_count(Person* infector, int disease_id, int sim_day, double contact_rate);

  /**
   * Determine if the place should be open. It is dependent on the disease_id and simulation day.
   *
   * @param day the simulation day
   * @param disease_id an integer representation of the disease
   * @return <code>true</code> if the place should be open; <code>false</code> if not
   */
  virtual bool should_be_open(int sim_day, int disease_id) = 0;

  // test place types
  bool is_household() {
    return this->get_type() == Place::TYPE_HOUSEHOLD;
  }
  
  bool is_neighborhood() {
    return this->get_type() == Place::TYPE_NEIGHBORHOOD;
  }
  
  bool is_school() {
    return this->get_type() == Place::TYPE_SCHOOL;
  }
  
  bool is_classroom() {
    return this->get_type() == Place::TYPE_CLASSROOM;
  }
  
  bool is_workplace() {
    return this->get_type() == Place::TYPE_WORKPLACE;
  }
  
  bool is_office() {
    return this->get_type() == Place::TYPE_OFFICE;
  }
  
  bool is_hospital() {
    return this->get_type() == Place::TYPE_HOSPITAL;
  }
  
  bool is_community() {
    return this->get_type() == Place::TYPE_COMMUNITY;
  }

  // test place subtypes
  bool is_college() {
    return this->get_subtype() == Place::SUBTYPE_COLLEGE;
  }
  
  bool is_prison() {
    return this->get_subtype() == Place::SUBTYPE_PRISON;
  }
  
  bool is_nursing_home() {
    return this->get_subtype() == Place::SUBTYPE_NURSING_HOME;
  }
  
  bool is_military_base() {
    return this->get_subtype() == Place::SUBTYPE_MILITARY_BASE;
  }
    
  bool is_healthcare_clinic() {
    return this->get_subtype() == Place::SUBTYPE_HEALTHCARE_CLINIC;
  }

  bool is_mobile_healthcare_clinic() {
    return this->get_subtype() == Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC;
  }
    
  bool is_group_quarters() {
    return (is_college() || is_prison() || is_military_base() || is_nursing_home());
  }

  // test for household types
  bool is_college_dorm(){
    return is_household() && is_college();
  }
  
  bool is_prison_cell(){
    return is_household() && is_prison();
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

  double get_distance(Place* place) {
    double x1 = this->get_x();
    double y1 = this->get_y();
    double x2 = place->get_x();
    double y2 = place->get_y();
    double distance = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    return distance;
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
   * Set the latitude.
   *
   * @param x the new latitude
   */
  void set_latitude(double x) {
    this->latitude = x;
  }

  /**
   * Set the longitude.
   *
   * @param x the new longitude
   */
  void set_longitude(double x) {
    this->longitude = x;
  }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will close.
   *
   * @param day the simulation day when the place will close
   */
  void set_close_date(int sim_day) {
    this->close_date = sim_day;
  }

  /**
   * Set the simulation day (an integer value of days from the start of the simulation) when the place will open.
   *
   * @param day the simulation day when the place will open
   */
  void set_open_date(int sim_day) {
    this->open_date = sim_day;
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
    return Geo::get_x(this->longitude);
  }

  double get_y() {
    return Geo::get_y(this->latitude);
  }

  void set_index(int _index) {
    this->index = _index;
  }
  
  int get_index() {
    return this->index;
  }
  
  int get_staff_size() {
    return this->staff_size;
  }
  
  void set_staff_size(int _staff_size) {
    this->staff_size = _staff_size;
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

  double get_seeds(int dis, int sim_day);

  /*
   * Vector Transmission methods
   */
  void setup_vector_model();

  void mark_vectors_as_not_infected_today() {
    this->vectors_have_been_infected_today = false;
  }

  void mark_vectors_as_infected_today() {
    this->vectors_have_been_infected_today = true;
  }

  bool have_vectors_been_infected_today() {
    return this->vectors_have_been_infected_today;
  }

  int get_vector_population_size() {
    return this->vector_disease_data->N_vectors;
  }

  int get_susceptible_vectors() {
    return this->vector_disease_data->S_vectors;
  }

  int get_infected_vectors(int disease_id) {
    return this->vector_disease_data->E_vectors[disease_id] +
      this->vector_disease_data->I_vectors[disease_id];
  }

  int get_infectious_vectors(int disease_id) {
    return this->vector_disease_data->I_vectors[disease_id];
  }

  void expose_vectors(int disease_id, int exposed_vectors) {
    this->vector_disease_data->E_vectors[disease_id] += exposed_vectors;
    this->vector_disease_data->S_vectors -= exposed_vectors;
  }

  vector_disease_data_t get_vector_disease_data () {
    assert(this->vector_disease_data != NULL);
    return (*this->vector_disease_data);
  }

  void update_vector_population(int sim_day);

  bool get_vector_control_status() {
    return this->vector_control_status;
  }
  void set_vector_control(){
    vector_control_status = true;
  }
  void stop_vector_control(){
      vector_control_status = false;
  }

protected:
  static double** prob_contact;

  fred::geo latitude;     // geo location
  fred::geo longitude;    // geo location
  int close_date;         // this place will be closed during:
  int open_date;          //   [close_date, open_date)
  double intimacy;	      // prob of intimate contact
  int index;		          // index for households
  int staff_size;			    // outside workers in this place

  int household_fips;
  int county_index;
  int census_tract_index;
  
  Neighborhood_Patch* patch;       // geo patch for this place

  // optional data for vector transmission model
  vector_disease_data_t* vector_disease_data;
  bool vectors_have_been_infected_today;
  bool vector_control_status;

  // Place_List, Neighborhood_Layer and Neighborhood_Patch are friends so that they can access
  // the Place Allocator.
  friend class Place_List;
  friend class Neighborhood_Layer;
  friend class Neighborhood_Patch;

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

    bool reserve(int n = 1) {
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

    Place_Type* get_base_pointer() {
      return allocation_array;
    }

    int size() {
      return allocations_made;
    }
  }; // end Place Allocator
};


#endif // _FRED_PLACE_H
