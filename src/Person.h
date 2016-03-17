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
#include <vector>
using namespace std;

class Place;
class Household;
class Condition;
class Infection;
class Mixing_Group;
class Network;
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
   * Make this agent unsusceptible to the given condition
   * @param condition_id the id of the condition to reference
   */
  void become_unsusceptible(int condition_id) {
    this->health.become_unsusceptible(condition_id);
  }

  /**
   * Make this agent unsusceptible to the given condition
   * @param condition the condition to reference
   */
  void become_unsusceptible(Condition* condition) {
    this->health.become_unsusceptible(condition);
  }

  void become_exposed(int condition_id, Person* infector, Mixing_Group* mixing_group, int day) {
    this->health.become_exposed(condition_id, infector, mixing_group, day);
  }
  /**
   * Make this agent immune to the given condition
   * @param condition the condition to reference
   */
  void become_immune(Condition* condition);

  void print(FILE* fp, int condition_id);

  void infect(Person* infectee, int condition_id, Mixing_Group* mixing_group, int day) {
    this->health.infect(infectee, condition_id, mixing_group, day);
  }

  /**
   * @param day the simulation day
   * @see Demographics::update(int day)
   */
  void update_demographics(int day) {
    this->demographics.update(day);
  }

  void update_infection(int day, int condition_id) {
    this->health.update_infection(day, condition_id);
  }

  void update_health_interventions(int day) {
    this->health.update_interventions(day);
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

  bool is_present(int sim_day, Place *place) {
    return this->activities.is_present(sim_day, place);
  }

  void update_schedule(int sim_day) {
    this->activities.update_schedule(sim_day);
  }

  void update_activities_of_infectious_person(int sim_day) {
    this->activities.update_activities_of_infectious_person(sim_day);
  }

  void update_enrollee_index(Mixing_Group* mixing_group, int pos) {
    this->activities.update_enrollee_index(mixing_group, pos);
  }

  /**
   * @Activities::update_profile()
   */
  void update_activity_profile() {
    this->activities.update_profile();
  }

  void become_susceptible(int condition_id) {
    this->health.become_susceptible(condition_id);
  }

  void become_susceptible_by_vaccine_waning(int condition_id) {
    this->health.become_susceptible_by_vaccine_waning(condition_id);
  }

  void update_household_counts(int day, int condition_id);
  void update_school_counts(int day, int condition_id);

  /**
   * This agent will become infectious with the condition
   * @param condition a pointer to the Condition
   */
  void become_infectious(Condition* condition) {
    this->health.become_infectious(condition);
  }

  void become_noninfectious(Condition* condition) {
    this->health.become_noninfectious(condition);
  }

  /**
   * This agent will become symptomatic with the condition
   * @param condition a pointer to the Condition
   */
  void become_symptomatic(Condition* condition) {
    this->health.become_symptomatic(condition);
  }

  void resolve_symptoms(Condition* condition) {
    this->health.resolve_symptoms(condition);
  }

  /**
   * This agent will recover from the condition
   * @param condition a pointer to the Condition
   */
  void recover(int day, Condition* condition) {
    this->health.recover(condition, day);
  }

  void become_case_fatality(int day, Condition* condition);

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
      this->activities.assign_classroom();
    }
  }

  /**
   * Assign the agent to an Office
   * @see Activities::assign_office()
   */
  void assign_office() {
    this->activities.assign_office();
  }

  /**
   * Assign the agent a primary healthjare facility
   * @see Activities::assign_primary_healthcare_facility()
   */
  void assign_primary_healthcare_facility() {
    this->activities.assign_primary_healthcare_facility();
  }

  /**
   * Will print out a Person in a format similar to that read from population file
   * (with additional run-time values inserted (denoted by *)):<br />
   * (i.e Label *ID* Age Sex Married Occupation Household School *Classroom* Workplace *Office*)
   * @return a string representation of this Person object
   */
  string to_string();

  /**
   * Will print out a Person in JSON format or will default to the to_string() above
   * @param is_JSON whether or not the output should be in JSON format
   * @param is_inline whether or not the output should start on its own line or not
   * @param indent_level how many times to indent each line (2 space indent)
   * @return a JSON string representation of this Person object
   */
  string to_string(bool is_JSON, bool is_inline, int indent_level);

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

  void set_deceased() {
    this->demographics.set_deceased();
  }

  bool is_eligible_to_migrate() {
    return this->eligible_to_migrate;
  }

  void clear_eligible_to_migrate() {
    this->eligible_to_migrate = false;
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

  int is_symptomatic(int condition_id) {
    return this->health.is_symptomatic(condition_id);
  }

  int get_days_symptomatic() {
    return this->health.get_days_symptomatic();
  }

  bool is_immune(int condition_id) {
    return this->health.is_immune(condition_id);
  }

  /**
   * @param dis the condition to check
   * @return <code>true</code> if this agent is susceptible to condition, <code>false</code> otherwise
   * @see Health::is_susceptible(int dis)
   */
  bool is_susceptible(int dis) {
    return this->health.is_susceptible(dis);
  }

  /**
   * @param dis the condition to check
   * @return <code>true</code> if this agent is infectious with condition, <code>false</code> otherwise
   * @see Health::is_infectious(int condition)
   */
  bool is_infectious(int dis) {
    return this->health.is_infectious(dis);
  }

  bool is_recovered(int dis) {
    return this->health.is_recovered(dis);
  }

  /**
   * @param dis the condition to check
   * @return <code>true</code> if this agent is infected with condition, <code>false</code> otherwise
   * @see Health::is_infected(int condition)
   */
  bool is_infected(int dis) {
    return this->health.is_infected(dis);
  }

  /**
   * @param condition the condition to check
   * @return the specific Condition's susceptibility for this Person
   * @see Health::get_susceptibility(int condition)
   */
  double get_susceptibility(int condition) const {
    return this->health.get_susceptibility(condition);
  }

  /**
   * @param condition the condition to check
   * @param day the simulation day
   * @return the specific Condition's infectivity for this Person
   * @see Health::get_infectivity(int condition, int day)
   */
  double get_infectivity(int condition_id, int day) const {
    return this->health.get_infectivity(condition_id, day);
  }

  /**
   * @param condition the condition to check
   * @param day the simulation day
   * @return the Symptoms for this Person
   * @see Health::get_symptoms(int day)
   */
  double get_symptoms(int condition_id, int day) const {
    return this->health.get_symptoms(condition_id, day);
  }

  /*
   * Advances the course of the infection by moving the exposure date
   * backwards
   */
  void advance_seed_infection(int condition_id, int days_to_advance) {
    this->health.advance_seed_infection(condition_id, days_to_advance);
  }

  /**
   * @param condition the condition to check
   * @return the simulation day that this agent became exposed to condition
   */
  int get_exposure_date(int condition) const {
    return this->health.get_exposure_date(condition);
  }

  int get_infectious_start_date(int condition) const {
    return this->health.get_infectious_start_date(condition);
  }

  int get_infectious_end_date(int condition) const {
    return this->health.get_infectious_end_date(condition);
  }

  int get_symptoms_start_date(int condition) const {
    return this->health.get_symptoms_start_date(condition);
  }

  int get_symptoms_end_date(int condition) const {
    return this->health.get_symptoms_end_date(condition);
  }

  int get_immunity_end_date(int condition) const {
    return this->health.get_immunity_end_date(condition);
  }

  /**
   * @param condition the condition to check
   * @return the Person who infected this agent with condition
   */
  Person* get_infector(int condition) const {
    return this->health.get_infector(condition);
  }

  /**
   * @param condition the condition to check
   * @return the id of the location where this agent became infected with condition
   */
  int get_infected_mixing_group_id(int condition) const {
    return this->health.get_infected_mixing_group_id(condition);
  }

  /**
   * @param condition the condition to check
   * @return the pointer to the Place where this agent became infected with condition
   */
  Mixing_Group* get_infected_mixing_group(int condition) const {
    return this->health.get_infected_mixing_group(condition);
  }

  /**
   * @param condition the condition to check
   * @return the label of the location where this agent became infected with condition
   */
  char* get_infected_mixing_group_label(int condition) const {
    return this->health.get_infected_mixing_group_label(condition);
  }

  /**
   * @param condition the condition to check
   * @return the type of the location where this agent became infected with condition
   */
  char get_infected_mixing_group_type(int condition) const {
    return this->health.get_infected_mixing_group_type(condition);
  }

  /**
   * @param condition the condition in question
   * @return the infectees
   * @see Health::get_infectees(int condition)
   */
  int get_infectees(int condition) const {
    return this->health.get_infectees(condition);
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

  void reset_neighborhood() {
    this->activities.reset_neighborhood();
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
    this->activities.start_traveling(visited);
  }

  /**
   * Have this Person stop traveling
   * @see Activities::stop_traveling()
   */
  void stop_traveling() {
    this->activities.stop_traveling();
  }

  /**
   * @return <code>true</code> if the Person is traveling, <code>false</code> if not
   * @see Activities::get_travel_status()
   */
  bool get_travel_status() {
    return this->activities.get_travel_status();
  }

  int get_num_past_infections(int condition) {
    return this->health.get_num_past_infections(condition);
  }

  Past_Infection* get_past_infection(int condition, int i) {
    return this->health.get_past_infection(condition, i);
  }

  void clear_past_infections(int condition) {
    this->health.clear_past_infections(condition);
  }

  //void add_past_infection(int d, Past_Infection *pi){ health.add_past_infection(d, pi); }  
  void add_past_infection(int strain_id, int recovery_date, int age_at_exposure, Condition* dis) {
    this->health.add_past_infection(strain_id, recovery_date, age_at_exposure, dis);
  }

  void take_vaccine(Vaccine* vacc, int day, Vaccine_Manager* vm) {
    this->health.take_vaccine(vacc, day, vm);
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

  double get_transmission_modifier_due_to_hygiene(int condition_id) {
    return this->health.get_transmission_modifier_due_to_hygiene(condition_id);
  }

  double get_susceptibility_modifier_due_to_hygiene(int condition_id) {
    return this->health.get_susceptibility_modifier_due_to_hygiene(condition_id);
  }

  bool is_case_fatality(int condition_id) {
    return this->health.is_case_fatality(condition_id);
  }

  void terminate(int day);

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
    return this->activities.become_a_teacher(school);
  }

  bool is_teacher() {
    return this->activities.is_teacher();
  }

  bool is_student() {
    return this->activities.is_student();
  }

  void move_to_new_house(Place* house) {
    activities.move_to_new_house(house);
  }

  void change_school(Place* place) {
    this->activities.change_school(place);
  }

  void change_workplace(Place* place) {
    this->activities.change_workplace(place);
  }

  void change_workplace(Place* place, int include_office) {
    this->activities.change_workplace(place, include_office);
  }

  int get_visiting_health_status(Place* place, int day, int condition_id) {
    return this->activities.get_visiting_health_status(place, day, condition_id);
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
   * condition), <code>false</code> otherwise
   * @see Health::has_COPD()
   */
  bool has_COPD() {
    return this->health.has_COPD();
  }

  /**
   * @return <code>true</code> if agent has chronic renal condition, <code>false</code> otherwise
   * @see Health::has_chronic_renal_condition()
   */
  bool has_chronic_renal_condition() {
    return this->health.has_chronic_renal_condition();
  }

  /**
   * @return <code>true</code> if agent is diabetic, <code>false</code> otherwise
   * @see Health::is_diabetic()
   */
  bool is_diabetic() {
    return this->health.is_diabetic();
  }

  /**
   * @return <code>true</code> if agent has heart condition, <code>false</code> otherwise
   *  @see Health::has_heart_condition()
   */
  bool has_heart_condition() {
    return this->health.has_heart_condition();
  }

  /**
   * @return <code>true</code> if agent has heart condition, <code>false</code> otherwise
   *  @see Health::has_hypertension()
   */
  bool has_hypertension() {
    return this->health.has_hypertension();
  }

  /**
   * @return <code>true</code> if agent has heart condition, <code>false</code> otherwise
   *  @see Health::has_hypercholestrolemia()
   */
  bool has_hypercholestrolemia() {
    return this->health.has_hypercholestrolemia();
  }

  /**
   * @return <code>true</code> if agent has heart condition, <code>false</code> otherwise
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

  double get_x();

  double get_y();

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

  // convenience methods for Networks

  bool is_enrolled_in_network(Network* network) {
    return this->activities.is_enrolled_in_network(network);
  }

  void create_network_link_to(Person* person, Network* network) {
    this->activities.create_network_link_to(person, network);
  }

  void destroy_network_link_to(Person* person, Network* network) {
    this->activities.destroy_network_link_to(person, network);
  }

  void create_network_link_from(Person* person, Network* network) {
    this->activities.create_network_link_from(person, network);
  }

  void destroy_network_link_from(Person* person, Network* network) {
    this->activities.destroy_network_link_from(person, network);
  }

  void add_network_link_to(Person* person, Network* network) {
    this->activities.add_network_link_to(person, network);
  }

  void add_network_link_from(Person* person, Network* network) {
    this->activities.add_network_link_from(person, network);
  }

  void delete_network_link_to(Person* person, Network* network) {
    this->activities.delete_network_link_to(person, network);
  }

  void delete_network_link_from(Person* person, Network* network) {
    this->activities.delete_network_link_from(person, network);
  }

  void join_network(Network* network) {
    this->activities.join_network(network);
  }

  void print_network(FILE* fp, Network* network) {
    this->activities.print_network(fp, network);
  }

  bool is_connected_to(Person* person, Network* network) {
    return this->activities.is_connected_to(person, network);
  }

  bool is_connected_from(Person* person, Network* network) {
    return this->activities.is_connected_from(person, network);
  }

  int get_out_degree(Network* network) {
    return this->activities.get_out_degree(network);
  }

  int get_in_degree(Network* network) {
    return this->activities.get_in_degree(network);
  }

  void clear_network(Network* network) {
    return this->activities.clear_network(network);
  }
  
  Person* get_end_of_link(int n, Network* network) {
    return this->activities.get_end_of_link(n, network);
  }

  int get_health_state(int condition_id) {
    return this->health.get_health_state(condition_id);
  }

  void set_health_state(int condition_id, int s, int day) {
    return this->health.set_health_state(condition_id, s, day);
  }

  int get_last_health_transition_day(int condition_id) {
    return this->health.get_last_transition_day(condition_id);
  }

  int get_next_health_state(int condition_id) {
    return this->health.get_next_health_state(condition_id);
  }

  void set_next_health_state(int condition_id, int s, int day) {
    return this->health.set_next_health_state(condition_id, s, day);
  }

  int get_next_health_transition_day(int condition_id) {
    return this->health.get_next_transition_day(condition_id);
  }

  void update_health_conditions(int day) {
    this->health.update_health_conditions(day);
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
  bool eligible_to_migrate;

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
