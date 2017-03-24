//
//  Relationships.h
//
//
//  Author: Mina Kabiri. Created on 6/1/16.
//
//

#ifndef ____Relationships__
#define ____Relationships__

#include <stdio.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#include "Global.h"


using namespace std;
class Person;

static const int ROW_age_group = 9; //Sexual_Transmission_Network::ROW_age_group;

class Relationships {
  
public:
  Relationships();
  ~Relationships();
  
  struct Partner_Struct {
    Person* partner;
    int dur;
    int dur_left;
    int dur_concurrent;
    int start_day;
    int end_day;
    bool long_term_status;
    int num_sexual_acts;
    vector<int> act_days_vec;
    bool sexual_act_today;
  };
  
  Partner_Struct* get_partner_struct_from_list(int i);
  Person* get_partner_from_list(int i);
  static void setup();
  void set_htype();
  void setup(Person* self);
  bool check_household_eligibility();
  bool check_relationships_eligiblity();
  void add_to_partner_list(Person* person);
  void remove_from_partner_list(int i);
  void update_existing_partner(int i, Person* partner);
  void increase_partner_in_age_group(int age_group, int addition);
  int get_partner_in_age_group(int age_group);
  int get_matched_partner_id(int i);
  void set_partner_dur(int i, int dur);
  int get_partner_dur(int i);
  int get_partner_dur_left(int i);
  void increase_time_past_in_relationship(int i);
  int get_time_past_in_relationship(int i);
  void set_time_left_in_relationship(int i, int left);
  void decrease_time_left_in_relationship(int i);
  int get_time_left_in_relationship(int i);
  void set_concurrent_days_in_relationships(int i, int days);
  int get_concurrent_days_in_relationships(int i);
  void set_partnerships_begin_end(int i, int begin, int end);
  int get_partner_start_day(int i);
  int get_partner_end_day(int i);
  void set_num_sexual_act_partner_i(int i, int freq);
  int get_num_sexual_act_partner_i(int i);
  void add_sex_act_day(int i, int day);
  int get_sex_act_day(int i, int act);
  void sort_act_days_vec(int i);
  void set_temp_age();
  int calculate_current_age_group();
  void terminate(int day, Sexual_Transmission_Network* sexual_network);
  void print_relationship();
  void set_sexual_act_today(int i);
  void reset_sexual_act_today(int i);
  bool if_sexual_act_today(int i);
  
  void set_long_term_status(int i);
  bool get_long_term_status(int i);
  void reset_long_term_status(int i);
  
  void set_initial_15cohort();
  bool get_initial_15cohort();
  
  //***********************************
  int get_htype(){
    return this->htype;
  }
  
  void set_person_num_partners_day0(int i, int j){
    this->num_partners_label = i;
    // mina update: use num_partners_ever used just for day 0, and use it for calculations in update_stat_ever: will change
    this->num_partners_ever_day0 =  j;
  }
  
  void set_person_num_partners_label(int i){
    this->num_partners_label = i;
  }
  
  int get_person_num_partners_label(){
    return num_partners_label;
  }
  
  int get_person_num_partners_ever_day0(){
    return num_partners_ever_day0;
  }
  
  int get_partner_list_size(){
    return this->partner_list.size();
  }

  void set_current_age_group(int age_group){
    this->current_age_group = age_group;
  }
  
  int get_current_age_group(){
    return this->current_age_group;
  }
  
  int get_temp_age(){
    return this->temp_age;
  }
  
  void set_same_age_category(bool i){
    this->same_age_category = i;
  }
  
  bool is_same_age_category(){
    return this->same_age_category;
  }
  
  void add_dur_day0(int dur, int dur_left){
    this->day0_initial_dur.push_back(dur);
    this->day0_initial_dur_left.push_back(dur_left);
  }

  int get_dur_day0(){
    int dur = this->day0_initial_dur.at(day0_initial_dur.size()-1);
    this->day0_initial_dur.pop_back();
    return dur;
  }
  int get_dur_left_day0(){
    int dur = this->day0_initial_dur_left.at(day0_initial_dur_left.size()-1);
    this->day0_initial_dur_left.pop_back();
    return dur;
  }
  
  int get_dur_day0_size(){
    return day0_initial_dur.size();
  }
  
private:
  // link back to person
  Person * myself;
  
  vector<Partner_Struct*> partner_list;
  vector<int> day0_initial_dur;
  vector<int> day0_initial_dur_left;
  
  int num_partners_label;
  int num_partners_ever_day0;
  
//  int num_partners_15_19;   //0
//  int num_partners_20_24;   //1
//  int num_partners_25_29;   //2
//  int num_partners_30_34;   //3
//  int num_partners_35_39;   //4
//  int num_partners_40_44;   //5
//  int num_partners_45_49;   //6
//  int num_partners_50_54;   //7
//  int num_partners_55over;  //8 : cap: 75 years
  
  int num_partner_in_age_group[ROW_age_group];
  
  
  int temp_partner_in_year;
  int current_age_group;
  int temp_age;
  int htype;
  bool same_age_category;
  
  bool initial_15_cohort;
  
};

#endif /* defined(____Relationships__) */
