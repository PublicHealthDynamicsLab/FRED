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

//
//
// File: Mixing_Group.h
//

#ifndef _FRED_MIXING_GROUP_H_
#define _FRED_MIXING_GROUP_H_

#include "Global.h"
#include "Person.h"

class Person;

typedef std::vector<Person*> person_vec_t;

class Mixing_Group {
public:

  static char TYPE_UNSET;
  static char SUBTYPE_NONE;

  Mixing_Group(const char* lab);
  virtual ~Mixing_Group();

  /**
   * Get the id.
   * @return the id
   */
  int get_id() const {
    return this->id;
  }

  void set_id(int _id) {
    this->id = _id;
  }

  /**
   * @return the type
   */
  char get_type() {
    return this->type;
  }

  void set_type(char _type) {
    this->type = _type;
  }

  /**
   * @return the subtype
   */
  char get_subtype() {
    return this->subtype;
  }

  void set_subtype(char _subtype) {
    this->subtype = _subtype;
  }

  /**
   * Get the label.
   *
   * @return the label
   */
  char* get_label() {
    return this->label;
  }

  // access methods:
  int get_adults();
  int get_children();

  /**
   * Get the age group for a person given a particular condition_id.
   *
   * @param condition_id an integer representation of the condition
   * @param per a pointer to a Person object
   * @return the age group for the given person for the given condition
   */
  virtual int get_group(int condition_id, Person* per) = 0;

  // enroll / unenroll:
  virtual int enroll(Person* per);
  virtual void unenroll(int pos);

  /**
   * Get the transmission probability for a given condition between two Person objects.
   *
   * @param condition_id an integer representation of the condition
   * @param i a pointer to a Person object
   * @param s a pointer to a Person object
   * @return the probability that there will be a transmission of condition_id from i to s
   */
  virtual double get_transmission_probability(int condition_id, Person* i, Person* s) = 0;

  virtual double get_transmission_prob(int condition_id, Person* i, Person* s) = 0;

  virtual double get_contacts_per_day(int condition_id) = 0;
  virtual double get_contact_rate(int day, int condition_id) = 0;
  virtual int get_contact_count(Person* infector, int condition_id, int sim_day, double contact_rate) = 0;

  /**
   * Get the count of agents in this place.
   *
   * @return the count of agents
   */
  int get_size() {
    return this->enrollees.size();
  }

  virtual int get_container_size() {
    return this->get_size();
  }

  virtual int get_orig_size() {
    return this->N_orig;
  }

  int get_recovereds(int condition_id);

  person_vec_t* get_enrollees() {
    return &(this->enrollees);
  }

  person_vec_t* get_infectious_people(int  condition_id) {
    return &(this->infectious_people[condition_id]);
  }

  Person* get_enrollee(int i) {
    return this->enrollees[i];
  }

  /*
   * Condition transmission
   */
  std::vector<Person*> get_potential_infectors(int condition_id) {
    return this->infectious_people[condition_id];
  }

  std::vector<Person*> get_potential_infectees(int condition_id) {
    return this->enrollees;
  }

  void record_infectious_days(int day);
  void print_infectious(int condition_id);

  // infectious people
  void clear_infectious_people(int condition_id) {
    this->infectious_people[condition_id].clear();
  }

  void add_infectious_person(int condition_id, Person* person);

  int get_number_of_infectious_people(int condition_id) {
    return this->infectious_people[condition_id].size();
  }
  
  Person* get_infectious_person(int condition_id, int n) {
    assert(n < this->infectious_people[condition_id].size());
    return this->infectious_people[condition_id][n];;
  }

  bool has_infectious_people(int condition_id) {
    return this->infectious_people[condition_id].size() > 0;
  }

  bool is_infectious(int condition_id) {
    return this->infectious_people[condition_id].size() > 0;
  }

  bool is_infectious() {
    return this->infectious_bitset.any();
  }

  bool is_human_infectious(int condition_id) {
    return this->human_infectious_bitset.test(condition_id);
  }

  void set_human_infectious(int condition_id) {
    if(!(this->human_infectious_bitset.test(condition_id))) {
      this->human_infectious_bitset.set(condition_id);
    }
  }

  void reset_human_infectious() {
    this->human_infectious_bitset.reset();
  }

  void set_exposed(int condition_id) {
    this->exposed_bitset.set(condition_id);
  }

  void reset_exposed() {
    this->exposed_bitset.reset();
  }

  bool is_exposed(int condition_id) {
    return this->exposed_bitset.test(condition_id);
  }

  void set_recovered(int condition_id) {
    this->recovered_bitset.set(condition_id);
  }

  void reset_recovered() {
    this->recovered_bitset.reset();
  }

  bool is_recovered(int condition_id) {
    return this->recovered_bitset.test(condition_id);
  }

  void reset_place_state(int condition_id) {
    this->infectious_bitset.reset(condition_id);
  }

  void increment_new_infections(int day, int condition_id) {
    if (this->last_update < day) {
      this->last_update = day;
      this->new_infections[condition_id] = 0;
    }
    this->new_infections[condition_id]++;
    this->total_infections[condition_id]++;
  }

  void increment_current_infections(int day, int condition_id) {
    if (this->last_update < day) {
      this->last_update = day;
      this->current_infections[condition_id] = 0;
    }
    this->current_infections[condition_id]++;
  }

  void increment_new_symptomatic_infections(int day, int condition_id) {
    if (this->last_update < day) {
      this->last_update = day;
      this->new_symptomatic_infections[condition_id] = 0;
    }
    this->new_symptomatic_infections[condition_id]++;
    this->total_symptomatic_infections[condition_id]++;
  }

  void increment_current_symptomatic_infections(int day, int condition_id) {
    if (this->last_update < day) {
      this->last_update = day;
      this->current_symptomatic_infections[condition_id] = 0;
    }
    this->current_symptomatic_infections[condition_id]++;
  }

  void increment_case_fatalities(int day, int condition_id) {
    if (this->last_update < day) {
      this->last_update = day;
      this->current_case_fatalities[condition_id] = 0;
    }
    this->current_case_fatalities[condition_id]++;
    this->total_case_fatalities[condition_id]++;
  }

  int get_current_case_fatalities(int day, int condition_id) {
    if (last_update < day) {
      return 0;
    }
    return current_case_fatalities[condition_id];
  }

  int get_total_case_fatalities(int condition_id) {
    return total_case_fatalities[condition_id];
  }

  int get_new_infections(int day, int condition_id) {
    if (last_update < day) {
      return 0;
    }
    return this->new_infections[condition_id];
  }

  int get_current_infections(int day, int condition_id) {
    if (last_update < day) {
      return 0;
    }
    return this->current_infections[condition_id];
  }

  int get_total_infections(int condition_id) {
    return this->total_infections[condition_id];
  }

  int get_new_symptomatic_infections(int day, int condition_id) {
    if (last_update < day) {
      return 0;
    }
    return this->new_symptomatic_infections[condition_id];
  }

  int get_current_symptomatic_infections(int day, int condition_id) {
    if (last_update < day) {
      return 0;
    }
    return this->current_symptomatic_infections[condition_id];
  }

  int get_total_symptomatic_infections(int condition_id) {
    return this->total_symptomatic_infections[condition_id];
  }

  /**
   * Get the number of cases of a given condition for the simulation thus far.
   *
   * @param condition_id an integer representation of the condition
   * @return the count of cases for a given condition
   */
  int get_total_cases(int condition_id) {
    return this->total_symptomatic_infections[condition_id];
  }

  /**
   * Get the number of cases of a given condition for the simulation thus far divided by the
   * number of agents in this place.
   *
   * @param condition_id an integer representation of the condition
   * @return the count of rate of cases per people for a given condition
   */
  double get_incidence_rate(int condition_id) {
    return static_cast<double>(this->total_symptomatic_infections[condition_id]) / static_cast<double>(get_size());
  }

  /**
   * Get the clincal attack rate = 100 * number of cases thus far divided by the
   * number of agents in this place.
   *
   * @param condition_id an integer representation of the condition
   * @return the count of rate of cases per people for a given condition
   */
  double get_symptomatic_attack_rate(int condition_id) {
    return (100.0 * this->total_symptomatic_infections[condition_id]) / static_cast<double>(get_size());
  }

  /**
   * Get the attack rate = 100 * number of infections thus far divided by the
   * number of agents in this place.
   *
   * @param condition_id an integer representation of the condition
   * @return the count of rate of cases per people for a given condition
   */
  double get_attack_rate(int condition_id) {
    int n = get_size();
    return(n > 0 ? (100.0 * this->total_infections[condition_id]) / static_cast<double>(n) : 0.0);
  }

  int get_first_day_infectious() {
    return this->first_day_infectious;
  }

  int get_last_day_infectious() {
    return this->last_day_infectious;
  }

  void resets(int condition_id) {
    new_infections[condition_id] = 0;
    current_infections[condition_id] = 0;
    new_symptomatic_infections[condition_id] = 0;
    current_symptomatic_infections[condition_id] = 0;
  }

protected:
  int id; // id
  char label[32]; // external id
  char type; // HOME, WORK, SCHOOL, COMMUNITY, etc;
  char subtype;
  int last_update;

  int N_orig;             // orig number of enrollees

  // lists of people
  person_vec_t* infectious_people;

  // epidemic counters
  int* new_infections;				// new infections today
  int* current_infections;	      // current active infections today
  int* new_symptomatic_infections;	   // new sympt infections today
  int* current_symptomatic_infections; // current active sympt infections
  int* total_infections;         // total infections over all time
  int* total_symptomatic_infections; // total sympt infections over all time
  int* current_case_fatalities;
  int* total_case_fatalities;

  // first and last days when visited by infectious people
  int first_day_infectious;
  int last_day_infectious;

  // lists of people
  person_vec_t  enrollees;

  // track whether or not place is infectious with each condition
  fred::condition_bitset infectious_bitset;
  fred::condition_bitset human_infectious_bitset;
  fred::condition_bitset recovered_bitset;
  fred::condition_bitset exposed_bitset;
};

#endif /* _FRED_MIXING_GROUP_H_ */
