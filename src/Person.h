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
// File: Person.h
//
#ifndef _FRED_PERSON_H
#define _FRED_PERSON_H

#include "Global.h"
using namespace std;

class Place;
class Household;
class Disease;
class Infection;
class Population;

#include "Demographics.h"
#include "Health.h"
#include "Behavior.h"
#include "Activities.h"
#include <map>

class Person {
public:

  Person();
  ~Person();

  /**
   * Make this agent unsusceptible to the given disease
   * @param disease the disease to reference
   */
  void become_unsusceptible(Disease* disease) {
    this->health.become_unsusceptible(this, disease);
  }

  void become_exposed(int disease_id, Person* infector, Place* place, int day) {
    this->health.become_exposed(this, disease_id, infector, place, day);
  }
  /**
   * Make this agent immune to the given disease
   * @param disease the disease to reference
   */
  void become_immune(Disease* disease);

  /**
   * Print out information about this object with regards to a disease to a file.
   *
   * @param fp a pointer to the file where this print should write
   * @param disease the disease about which to get information
   */
  void print(FILE* fp, int disease);

  /**
   * @param day the simulation day
   * @param disease the disease to check
   * @return <code>true</code> if the exposure day to the disease = day, <code>false</code> otherwise
   */
  int is_new_case(int day, int disease) const {
    return (this->health.get_exposure_date(disease) == day);
  }

  int addInfected(int disease, vector<int> strains);

  void infect(Person* infectee, int disease_id, Place* place, int day) {
    this->health.infect(this, infectee, disease_id, place, day);
  }

  void increment_infectee_count(int disease_id, Person* infectee, Place* place, int day) {
    this->health.increment_infectee_count(this, disease_id, infectee, place, day);
  }

  /**
   * @param day the simulation day
   * @see Demographics::update(int day)
   */
  void update_demographics(int day) {
    this->demographics.update(day);
  }

  /**
   * @param day the simulation day
   * @see Health::update(int day)
   */
  void update_health(int day) {
    this->health.update(this, day);
  }

  void update_health_interventions(int day) {
    this->health.update_interventions(this, day);
  }

  /**
   * @param day the simulation day
   * @see Behavior::update(int day)
   */
  void update_behavior(int day) {
    this->behavior.update(this, day);
  }

  /**
   * @Activities::prepare()
   */
  void prepare_activities() {
    this->activities.prepare();
  }

  /**
   * @Activities::update_profile()
   */
  void update_activity_profile() {
    this->activities.update_profile(this);
  }

  /**
   * This agent will become susceptible to the disease
   * @param disease the disease
   */
  void become_susceptible(Disease* disease) {
    this->health.become_susceptible(this, disease);
  }

  void become_susceptible_by_vaccine_waning(Disease* disease) {
    this->health.become_susceptible_by_vaccine_waning(this, disease);
  }

  void update_household_counts(int day, int disease_id) {
    Place* hh = this->get_household();
    if(hh == NULL) {
      if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
        hh = this->get_permanent_household();
      }
    }
    if(hh != NULL) {
      this->health.update_place_counts(this, day, disease_id, hh);
    }
  }

  void update_school_counts(int day, int disease_id) {
    this->health.update_place_counts(this, day, disease_id, get_school());
  }

  /**
   * This agent will become infectious with the disease
   * @param disease a pointer to the Disease
   */
  void become_infectious(Disease* disease) {
    this->health.become_infectious(this, disease);
  }

  /**
   * This agent will become symptomatic with the disease
   * @param disease a pointer to the Disease
   */
  void become_symptomatic(Disease* disease) {
    this->health.become_symptomatic(this, disease);
  }

  /**
   * This agent will recover from the disease
   * @param disease a pointer to the Disease
   */
  void recover(Disease* disease) {
    this->health.recover(this, disease);
  }

  /**
   * This agent creates a new agent
   * @return a pointer to the new Person
   */
  Person* give_birth(int day);

  /**
   * Assign the agent to a Classroom
   * @see Activities::assign_classroom()
   */
  void assign_classroom() {
    if(this->activities.get_school()) {
      this->activities.assign_classroom(this);
    }
  }

  /**
   * Assign the agent to an Office
   * @see Activities::assign_office()
   */
  void assign_office() {
    this->activities.assign_office(this);
  }

  /**
   * Assign the agent a primary healthjare facility
   * @see Activities::assign_primary_healthcare_facility()
   */
  void assign_primary_healthcare_facility() {
    this->activities.assign_primary_healthcare_facility(this);
  }

  /**
   * Will print out a person in a format similar to that read from population file
   * (with additional run-time values inserted (denoted by *)):<br />
   * (i.e Label *ID* Age Sex Married Occupation Household School *Classroom* Workplace *Office*)
   * @return a string representation of this Person object
   */
  string to_string();

  // access functions:
  /**
   * The id is generated at runtime
   * @return the id of this Person
   */
  int get_id() const {
    return this->id;
  }

  /**
   * @return a pointer to this Person's Demographics
   */
  Demographics* get_demographics() {
    return &this->demographics;
  }

  /**
   * @return the Person's age
   * @see Demographics::get_age()
   */
  int get_age() {
    return this->demographics.get_age();
  }

  /**
   * @return the Person's initial age
   * @see Demographics::get_init_age()
   */
  int get_init_age() const {
    return this->demographics.get_init_age();
  }

  /**
   * @return the Person's age as a double value based on the number of days alive
   * @see Demographics::get_real_age()
   */
  double get_real_age() const {
    return this->demographics.get_real_age();
  }

  /**
   * @return the Person's sex
   */
  char get_sex() const {
    return this->demographics.get_sex();
  }

  /**
   * @return the Person's race
   * @see Demographics::get_race()
   */
  int get_race() {
    return this->demographics.get_race();
  }

  int get_relationship() {
    return this->demographics.get_relationship();
  }

  void set_relationship(int rel) {
    this->demographics.set_relationship(rel);
  }

  /**
   * @return <code>true</code> if this agent is deceased, <code>false</code> otherwise
   */
  bool is_deceased() {
    return this->demographics.is_deceased();
  }

  /**
   * @return <code>true</code> if this agent is an adult, <code>false</code> otherwise
   */
  bool is_adult() {
    return this->demographics.get_age() >= Global::ADULT_AGE;
  }

  /**
   * @return <code>true</code> if this agent is a chiild, <code>false</code> otherwise
   */
  bool is_child() {
    return this->demographics.get_age() < Global::ADULT_AGE;
  }

  /**
   * @return a pointer to this Person's Health
   */
  Health* get_health() {
    return &this->health;
  }

  /**
   * @return <code>true</code> if this agent is symptomatic, <code>false</code> otherwise
   * @see Health::is_symptomatic()
   */
  int is_symptomatic() {
    return this->health.is_symptomatic();
  }

  int get_days_symptomatic() {
    return this->health.get_days_symptomatic();
  }

  bool is_immune(Disease* dis) {
    return this->health.is_immune(dis);
  }

  /**
   * @param dis the disease to check
   * @return <code>true</code> if this agent is susceptible to disease, <code>false</code> otherwise
   * @see Health::is_susceptible(int dis)
   */
  bool is_susceptible(int dis) {
    return this->health.is_susceptible(dis);
  }

  /**
   * @param dis the disease to check
   * @return <code>true</code> if this agent is infectious with disease, <code>false</code> otherwise
   * @see Health::is_infectious(int disease)
   */
  bool is_infectious(int dis) {
    return this->health.is_infectious(dis);
  }

  bool is_recovered(int dis) {
    return this->health.is_recovered(dis);
  }

  /**
   * @param dis the disease to check
   * @return <code>true</code> if this agent is infected with disease, <code>false</code> otherwise
   * @see Health::is_infected(int disease)
   */
  bool is_infected(int dis) {
    return this->health.is_infected(dis);
  }

  /**
   * @param disease the disease to check
   * @return the specific Disease's susceptibility for this Person
   * @see Health::get_susceptibility(int disease)
   */
  double get_susceptibility(int disease) const {
    return this->health.get_susceptibility(disease);
  }

  /**
   * @param disease the disease to check
   * @param day the simulation day
   * @return the specific Disease's infectivity for this Person
   * @see Health::get_infectivity(int disease, int day)
   */
  double get_infectivity(int disease, int day) const {
    return this->health.get_infectivity(disease, day);
  }

  /**
   * @param disease the disease to check
   * @param day the simulation day
   * @return the Symptoms for this Person
   * @see Health::get_symptoms(int day)
   */
  double get_symptoms(int disease, int day) const {
    return this->health.get_symptoms(disease, day);
  }

  /*
   * Advances the course of the infection by moving the exposure date
   * backwards
   */
  void advance_seed_infection(int disease_id, int days_to_advance) {
    this->health.advance_seed_infection(disease_id, days_to_advance);
  }

  /**
   * @param disease the disease to check
   * @return the simulation day that this agent became exposed to disease
   */
  int get_exposure_date(int disease) const {
    return this->health.get_exposure_date(disease);
  }

  /**
   * @param disease the disease to check
   * @return the simulation day that this agent became infectious with disease
   */
  int get_infectious_date(int disease) const {
    return this->health.get_infectious_date(disease);
  }

  /**
   * @param disease the disease to check
   * @return the simulation day that this agent became recovered from disease
   */
  int get_recovered_date(int disease) const {
    return this->health.get_recovered_date(disease);
  }

  /**
   * @param disease the disease to check
   * @return the Person who infected this agent with disease
   */
  Person* get_infector(int disease) const {
    return this->health.get_infector(disease);
  }

  /**
   * @param disease the disease to check
   * @return the id of the location where this agent became infected with disease
   */
  int get_infected_place_id(int disease) const {
    return this->health.get_infected_place_id(disease);
  }

  /**
   * @param disease the disease to check
   * @return the pointer to the Place where this agent became infected with disease
   */
  Place* get_infected_place(int disease) const {
    return this->health.get_infected_place(disease);
  }

  /**
   * @param disease the disease to check
   * @return the label of the location where this agent became infected with disease
   */
  char* get_infected_place_label(int disease) const {
    return this->health.get_infected_place_label(disease);
  }

  /**
   * @param disease the disease to check
   * @return the type of the location where this agent became infected with disease
   */
  char get_infected_place_type(int disease) const {
    return this->health.get_infected_place_type(disease);
  }

  /**
   * @param disease the disease in question
   * @return the infectees
   * @see Health::get_infectees(int disease)
   */
  int get_infectees(int disease) const {
    return this->health.get_infectees(disease);
  }

  /**
   * @return a pointer to this Person's Activities
   */
  Activities* get_activities() {
    return &this->activities;
  }

  /**
   * @return the a pointer to this agent's Neighborhood
   */
  Place* get_neighborhood() {
    return this->activities.get_neighborhood();
  }

  /**
   * @return a pointer to this Person's Household
   * @see Activities::get_household()
   */
  Place* get_household() {
    return this->activities.get_household();
  }

  int get_exposed_household_index() {
    return this->exposed_household_index;
  }

  void set_exposed_household(int index_) {
    this->exposed_household_index = index_;
  }

  Place* get_permanent_household() {
    return this->activities.get_permanent_household();
  }

  unsigned char get_deme_id() {
    return this->activities.get_deme_id();
  }

  bool is_householder() {
    return this->demographics.is_householder();
  }

  void make_householder() {
    this->demographics.make_householder();
  }

  /**
   * @return a pointer to this Person's School
   * @see Activities::get_school()
   */
  Place* get_school() {
    return this->activities.get_school();
  }

  /**
   * @return a pointer to this Person's Classroom
   * @see Activities::get_classroom()
   */
  Place* get_classroom() {
    return this->activities.get_classroom();
  }

  /**
   * @return a pointer to this Person's Workplace
   * @see Activities::get_workplace()
   */
  Place* get_workplace() {
    return this->activities.get_workplace();
  }

  /**
   * @return a pointer to this Person's Office
   * @see Activities::get_office()
   */
  Place* get_office() {
    return this->activities.get_office();
  }

  /**
   * @return a pointer to this Person's Hospital
   * @see Activities::get_hospital()
   */
  Place* get_hospital() {
    return this->activities.get_hospital();
  }

  /**
   * @return a pointer to this Person's Ad Hoc location
   * @see Activities::get_ad_hoc()
   */
  Place* get_ad_hoc() {
    return this->activities.get_ad_hoc();
  }

  /**
   *  @return the number of other agents in an agent's neighborhood, school, and workplace.
   *  @see Activities::get_degree()
   */
  int get_degree() {
    return this->activities.get_degree();
  }

  int get_household_size() {
    return this->activities.get_group_size(Activity_index::HOUSEHOLD_ACTIVITY);
  }

  int get_neighborhood_size() {
    return this->activities.get_group_size(Activity_index::NEIGHBORHOOD_ACTIVITY);
  }

  int get_school_size() {
    return this->activities.get_group_size(Activity_index::SCHOOL_ACTIVITY);
  }

  int get_classroom_size() {
    return this->activities.get_group_size(Activity_index::CLASSROOM_ACTIVITY);
  }

  int get_workplace_size() {
    return this->activities.get_group_size(Activity_index::WORKPLACE_ACTIVITY);
  }

  int get_office_size() {
    return this->activities.get_group_size(Activity_index::OFFICE_ACTIVITY);
  }

  bool is_hospitalized() {
    return this->activities.is_hospitalized;
  }

  /**
   * Have this Person begin traveling
   * @param visited the Person this agent will visit
   * @see Activities::start_traveling(Person* visited)
   */
  void start_traveling(Person* visited) {
    this->activities.start_traveling(this, visited);
  }

  /**
   * Have this Person stop traveling
   * @see Activities::stop_traveling()
   */
  void stop_traveling() {
    this->activities.stop_traveling(this);
  }

  /**
   * @return <code>true</code> if the Person is traveling, <code>false</code> if not
   * @see Activities::get_travel_status()
   */
  bool get_travel_status() {
    return this->activities.get_travel_status();
  }

  int get_num_past_infections(int disease) {
    return this->health.get_num_past_infections(disease);
  }

  Past_Infection* get_past_infection(int disease, int i) {
    return this->health.get_past_infection(disease, i);
  }

  void clear_past_infections(int disease) {
    this->health.clear_past_infections(disease);
  }

  //void add_past_infection(int d, Past_Infection *pi){ health.add_past_infection(d, pi); }  
  void add_past_infection(int strain_id, int recovery_date, int age_at_exposure, Disease* dis) {
    this->health.add_past_infection(strain_id, recovery_date, age_at_exposure, dis);
  }

  void take_vaccine(Vaccine* vacc, int day, Vaccine_Manager* vm) {
    this->health.take_vaccine(this, vacc, day, vm);
  }

  // set up and access health behaviors
  void setup_behavior() {
    this->behavior.setup(this);
  }

  bool is_health_decision_maker() {
    return this->behavior.is_health_decision_maker();
  }

  Person* get_health_decision_maker() {
    return this->behavior.get_health_decision_maker();
  }

  void set_health_decision_maker(Person* p) {
    this->behavior.set_health_decision_maker(p);
  }

  void become_health_decision_maker(Person* self) {
    this->behavior.become_health_decision_maker(self);
  }

  bool adult_is_staying_home() {
    return this->behavior.adult_is_staying_home();
  }

  bool child_is_staying_home() {
    return this->behavior.child_is_staying_home();
  }

  bool acceptance_of_vaccine() {
    return this->behavior.acceptance_of_vaccine();
  }

  bool acceptance_of_another_vaccine_dose() {
    return this->behavior.acceptance_of_another_vaccine_dose();
  }

  bool child_acceptance_of_vaccine() {
    return this->behavior.child_acceptance_of_vaccine();
  }

  bool child_acceptance_of_another_vaccine_dose() {
    return this->behavior.child_acceptance_of_another_vaccine_dose();
  }

  bool is_sick_leave_available() {
    return this->activities.is_sick_leave_available();
  }

  double get_transmission_modifier_due_to_hygiene(int disease_id) {
    return this->health.get_transmission_modifier_due_to_hygiene(disease_id);
  }

  double get_susceptibility_modifier_due_to_hygiene(int disease_id) {
    return this->health.get_susceptibility_modifier_due_to_hygiene(disease_id);
  }

  void terminate();

  void die() {
    this->health.die();
  }

  void set_pop_index(int idx) {
    this->index = idx;
  }

  int get_pop_index() {
    return this->index;
  }

  void birthday(int day) {
    this->demographics.birthday(this, day);
  }

  bool become_a_teacher(Place* school) {
    return this->activities.become_a_teacher(this, school);
  }

  bool is_teacher() {
    return this->activities.is_teacher();
  }

  bool is_student() {
    return this->activities.is_student();
  }

  void move_to_new_house(Place* house) {
    activities.move_to_new_house(this, house);
  }

  void change_school(Place* place) {
    this->activities.change_school(this, place);
  }

  void change_workplace(Place* place) {
    this->activities.change_workplace(this, place);
  }

  void change_workplace(Place* place, int include_office) {
    this->activities.change_workplace(this, place, include_office);
  }

  int get_visiting_health_status(Place* place, int day, int disease_id) {
    return this->activities.get_visiting_health_status(this, place, day, disease_id);
  }

  /**
   * @return <code>true</code> if agent is asthmatic, <code>false</code> otherwise
   * @see Health::is_asthmatic()
   */
  bool is_asthmatic() {
    return this->health.is_asthmatic();
  }

  /**
   * @return <code>true</code> if agent has COPD (Chronic Obstructive Pulmonary
   * disease), <code>false</code> otherwise
   * @see Health::has_COPD()
   */
  bool has_COPD() {
    return this->health.has_COPD();
  }

  /**
   * @return <code>true</code> if agent has chronic renal disease, <code>false</code> otherwise
   * @see Health::has_chronic_renal_disease()
   */
  bool has_chronic_renal_disease() {
    return this->health.has_chronic_renal_disease();
  }

  /**
   * @return <code>true</code> if agent is diabetic, <code>false</code> otherwise
   * @see Health::is_diabetic()
   */
  bool is_diabetic() {
    return this->health.is_diabetic();
  }

  /**
   * @return <code>true</code> if agent has heart disease, <code>false</code> otherwise
   *  @see Health::has_heart_disease()
   */
  bool has_heart_disease() {
    return this->health.has_heart_disease();
  }

  /**
   * @return <code>true</code> if agent has heart disease, <code>false</code> otherwise
   *  @see Health::has_hypertension()
   */
  bool has_hypertension() {
    return this->health.has_hypertension();
  }

  /**
   * @return <code>true</code> if agent has heart disease, <code>false</code> otherwise
   *  @see Health::has_hypercholestrolemia()
   */
  bool has_hypercholestrolemia() {
    return this->health.has_hypercholestrolemia();
  }

  /**
   * @return <code>true</code> if agent has heart disease, <code>false</code> otherwise
   *  @see Health::has_chronic_condition()
   */
  bool has_chronic_condition() {
    return this->health.has_chronic_condition();
  }

  /**
   * @return <code>true</code> if agent is alive, <code>false</code> otherwise
   * @see Health::is_alive()
   */
  bool is_alive() {
    return this->health.is_alive();
  }

  /**
   * @return <code>true</code> if agent is dead, <code>false</code> otherwise
   * @see Health::is_alive()
   */
  bool is_dead() {
    return this->health.is_alive() == false;
  }

  double get_x() {
    Place* hh = this->get_household();

    if(hh == NULL) {
      if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
        hh = this->get_permanent_household();
      }
    }

    if(hh == NULL) {
      return 0.0;
    } else {
      return hh->get_x();
    }
  }

  double get_y() {
    Place* hh = this->get_household();

    if(hh == NULL) {
      if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
        hh = this->get_permanent_household();
      }
    }

    if(hh == NULL) {
      return 0.0;
    } else {
      return hh->get_y();
    }
  }

  bool is_prisoner() {
    return this->activities.is_prisoner();
  }

  bool is_college_dorm_resident() {
    return this->activities.is_college_dorm_resident();
  }

  bool is_military_base_resident() {
    return this->activities.is_military_base_resident();
  }

  bool is_nursing_home_resident() {
    return this->activities.is_nursing_home_resident();
  }

  bool lives_in_group_quarters() {
    return (is_college_dorm_resident() || is_military_base_resident()
	    || is_prisoner() || is_nursing_home_resident());
  }

  char get_profile() {
    return this->activities.get_profile();
  }

  int get_number_of_children() {
    return this->demographics.get_number_of_children();
  }

  int get_grade() {
    return this->activities.get_grade();
  }

  void set_grade(int n) {
    this->activities.set_grade(n);
  }

private:

  // id: Person's unique identifier (never reused)
  int id;
  // index: Person's location in population container; once set, will be unique at any given time,
  // but can be reused over the course of the simulation for different people (after death/removal)
  int index;
  int exposed_household_index;
  Health health;
  Demographics demographics;
  Activities activities;
  Behavior behavior;

protected:

  friend class Population;
  /**
   * Constructor that sets all of the attributes of a Person object
   * @param index the person id
   * @param age
   * @param sex (M or F)
   * @param house pointer to this Person's Household
   * @param school pointer to this Person's School
   * @param work pointer to this Person's Workplace
   * @param day the simulation day
   * @param today_is_birthday true if this is a newborn
   */
  void setup(int index, int id, int age, char sex, int race, int rel,
	     Place* house, Place* school, Place* work, int day,
	     bool today_is_birthday);

};

#endif // _FRED_PERSON_H
