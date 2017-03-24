//
//  Relationships.cc
//
//
//  Author: Mina Kabiri. Created on 6/1/16.
//
//

#include "Condition.h"
#include "Relationships.h"
#include "Person.h"
#include "Global.h"
#include "Population.h"
#include "Place_List.h"
#include "Sexual_Transmission_Network.h"

//*******************************************************
//*******************************************************

Relationships::Relationships() {
  
  this->myself = NULL;
  this->num_partners_label = -1;
  this->num_partners_ever_day0 = -1;

  //this->num_matched_partners = -1;
//  this->num_partners_15_19 = -1;
//  this->num_partners_20_24 = -1;
//  this->num_partners_25_29 = -1;
//  this->num_partners_30_34 = -1;
//  this->num_partners_35_39 = -1;
//  this->num_partners_40_44 = -1;
//  this->num_partners_45_49 = -1;
//  this->num_partners_50_54 = -1;
//  this->num_partners_55over = -1;

  for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
    this->num_partner_in_age_group[i] = -1;
  }
  
  this->temp_partner_in_year= -1;
  this->current_age_group = -1;
  
  this->temp_age = -1;
  this->htype = -1;
  this->same_age_category = true;
  
  this->initial_15_cohort = false;
}

Relationships::~Relationships() {}

void Relationships::setup(Person* self) {
  
  this->myself = self;
  
  this->num_partners_label = -1;  //mina check
  this->num_partners_ever_day0 = 0;
//  this->num_partners_15_19 = 0;
//  this->num_partners_20_24 = 0;
//  this->num_partners_25_29 = 0;
//  this->num_partners_30_34 = 0;
//  this->num_partners_35_39 = 0;
//  this->num_partners_40_44 = 0;
//  this->num_partners_45_49 = 0;
//  this->num_partners_50_54 = 0;
//  this->num_partners_55over = 0;
  
  for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
    this->num_partner_in_age_group[i] = 0;
  }
 
  //this->count_matched_partners = 0;
  this->temp_partner_in_year= 0;
  this->set_htype();
  this->set_current_age_group(this->calculate_current_age_group());
  this->set_temp_age();   //this does not change unless person's integer age changes and then updated
  
  bool include_person = this->check_relationships_eligiblity();
  
  //copied from Health.cc : Assume that whoever is 15 years or older is in sexual transmission network
  //exclude people who are not eligible base on household type
  if (include_person) {
    
    if(Global::Enable_Transmission_Network) {   //mina check: disabled because we are adding to sexual transmission network. add person once. uncomment this and they are added twice!
      //FRED_VERBOSE(0, "Joining transmission network: id = %d\n", self->get_id());
      self->join_network(Global::Transmission_Network);
    }
    if(Global::Enable_Sexual_Partner_Network) {
      Condition* condition = Global::Conditions.get_condition(0);   //condition_id = 0 for HIV. replaced
      if(strcmp("sexual", condition->get_transmission_mode()) == 0) {
        Global::Sexual_Partner_Network->add_person_to_sexual_partner_network(self);
      }
    }
  }
  
}   //end of setup()

void Relationships::add_to_partner_list(Person* person){
  
  Partner_Struct * partner_ptr = new Partner_Struct();
  
  partner_ptr->partner = person;
  partner_ptr->dur = 0;
  partner_ptr->dur_left = 0;
  partner_ptr->dur_concurrent = 0;
  partner_ptr->start_day = 0;
  partner_ptr->end_day = 0;
  partner_ptr->long_term_status = false;
  partner_ptr->num_sexual_acts = 0;
  partner_ptr->sexual_act_today = false;
  
  this->partner_list.push_back(partner_ptr);
  //cout << "Partner ID " << id << " added to list. Partner_list size: " << this->partner_list.size() << endl;
  //"\t"; cout << "dur size: " << get_partner_dur_size() << "\tdur past size: " << get_time_past_in_relationship_size() << endl;
}

void Relationships::update_existing_partner(int i, Person* partner){    //sometime we add the partner 'place' but the partner is NULL. THis is to update that.
  //i: place in partner_list
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->partner = partner;
}

void Relationships::remove_from_partner_list(int i){
  Partner_Struct* tempB;
  Partner_Struct* tempZ;
  
  if (this->partner_list.size() == 1) {   //if there is only one partner on the list
    partner_list.pop_back();
  }
  else {
    tempZ = partner_list.at(i);
    tempB = partner_list.at(partner_list.size()-1);
    partner_list.at(i)= tempB;
    partner_list.at(partner_list.size()-1)=tempZ;
    partner_list.pop_back();
  }
  //this->decrease_num_matched_partners();
  
}

Relationships::Partner_Struct* Relationships::get_partner_struct_from_list(int i){
  //i: place in partner_list
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr;
}

Person* Relationships::get_partner_from_list(int i){
  //i: place in partner_list
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->partner;
}

void Relationships::set_partner_dur(int i, int dur){
  //add for each partner, the duration of relationship (in days).
  //i: place in partner_list
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->dur = dur;
}

int Relationships::get_partner_dur(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->dur;
}

int Relationships::get_partner_dur_left(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->dur_left;
}

void Relationships::set_sexual_act_today(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->sexual_act_today = true;
}

void Relationships::reset_sexual_act_today(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->sexual_act_today = false;
}

bool Relationships::if_sexual_act_today(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->sexual_act_today;
}

void Relationships::set_htype(){
  
  /* Household structure types:
   0  single-female
   1  single-male
   2  opp-sex-sim-age-pair
   3  opp-sex-dif-age-pair
   4  opp-sex-two-parent-family
   5  single-parent-family
   6  single-parent-multigen-family
   7  two-parent-multigen-family
   8  unattended_minors
   9  other-family
   10 young-roomies
   11 older-roomies
   12 mixed-roomies
   13 same-sex-sim-age-pair
   14 same-sex-dif-age-pair
   15 same-sex-two-parent-family
   */
  string htype[21] = {
    "single-female",
    "single-male",
    "opp-sex-sim-age-pair",
    "opp-sex-dif-age-pair",
    "opp-sex-two-parent-family",
    "single-parent-family",
    "single-parent-multigen-family",
    "two-parent-multigen-family",
    "unattended-minors",
    "other-family",
    "young-roomies",
    "older-roomies",
    "mixed-roomies",
    "same-sex-sim-age-pair",
    "same-sex-dif-age-pair",
    "same-sex-two-parent-family",
    "dorm-mates",
    "cell-mates",
    "barrack-mates",
    "nursing-home_mates",
    "unknown",
  };
  
  char* htype_label = myself->get_household_structure_label();
  
  if ( strcmp(htype[0].c_str(), htype_label) == 0 ) this->htype = 0;
  else if ( strcmp(htype[1].c_str(), htype_label) == 0) this->htype = 1;
  else if ( strcmp(htype[2].c_str(), htype_label) == 0) this->htype = 2;
  else if ( strcmp(htype[3].c_str(), htype_label) == 0) this->htype = 3;
  else if ( strcmp(htype[4].c_str(), htype_label) == 0) this->htype = 4;
  else if ( strcmp(htype[5].c_str(), htype_label) == 0) this->htype = 5;
  else if ( strcmp(htype[6].c_str(), htype_label) == 0) this->htype = 6;
  else if ( strcmp(htype[7].c_str(), htype_label) == 0) this->htype = 7;
  else if ( strcmp(htype[10].c_str(), htype_label) == 0) this->htype = 10;
  else if ( strcmp(htype[11].c_str(), htype_label) == 0) this->htype = 11;
  else if ( strcmp(htype[12].c_str(), htype_label) == 0) this->htype = 12;
  else this->htype = -1;
  
}

bool Relationships::check_relationships_eligiblity(){
  
  bool include_person = false;
  bool include_hh = check_household_eligibility();
  Person* self = this->myself;
  
  //self->get_id() <= Sexual_Transmission_Network::LAST_ID &&
  //if (this->get_current_age_group() >= 0 && this->get_current_age_group() < 6 && include_hh == true)
  //change: added older age groups
  if (this->get_current_age_group() >= 0 && this->get_current_age_group() < 9 && include_hh == true)
  
    include_person = true;
  
  //debug
  //  if(myself->get_id() == Global::TEST_ID){
 // if(Global::Simulation_Day ==1 && this->get_current_age_group() == 0)   cout << "\nAge " << myself->get_age() << "  current_age_group " << this->get_current_age_group() <<  "  include_hh " << include_hh  << "  include person " << include_person << endl;
  //    getchar();
  //  }
  
  return include_person;
}

bool Relationships::check_household_eligibility(){
  bool include_hh = false;
  
  if (this->get_htype() != -1)
    include_hh = true;    //mina check
  else
    include_hh = false;
  
  return include_hh;
}

void Relationships::set_temp_age(){
  //temp_age does not change unless person's integer age changes and then updated
  this->temp_age = myself->get_age();
}

int Relationships::calculate_current_age_group(){
  int age = myself->get_age();
  char sex = myself->get_sex();
  int hid = myself->get_household()->get_id();
  int age_group = -1;
  
  if (age < 15) age_group=-1;
  else if (age >= 15 && age < 20) age_group=0;
  else if (age >= 20 && age < 25) age_group=1;
  else if (age >= 25 && age < 30) age_group=2;
  else if (age >= 30 && age < 35) age_group=3;
  else if (age >= 35 && age < 40) age_group=4;
  else if (age >= 40 && age < 45) age_group=5;
  else if (age >= 45 && age < 49) age_group=6;
  else if (age >= 50 && age < 54) age_group=7;
  else if (age >= 55 && age < 75) age_group=8;    //cap: 75 years old!
  else age_group=9;
  
  return age_group;
}

//used to calculate number of partners ever, by a certain age
void Relationships::increase_partner_in_age_group(int age_group, int addition){
  
  this->num_partner_in_age_group[age_group] += addition;
  
  
//  if (age_group == 0) num_partners_15_19 += addition;
//  else if (age_group == 1) num_partners_20_24 += addition;
//  else if (age_group == 2) num_partners_25_29 += addition;
//  else if (age_group == 3) num_partners_30_34 += addition;
//  else if (age_group == 4) num_partners_35_39 += addition;
//  else if (age_group == 5) num_partners_40_44 += addition;
//  else if (age_group == 6) num_partners_45_54 += addition;
//  else if (age_group == 7) num_partners_55over += addition;
  
  //cout << num_partners_15_19 << endl;
}

int Relationships::get_partner_in_age_group(int age_group){
  
  if (age_group >= 0 && age_group < Sexual_Transmission_Network::ROW_age_group )
    return this->num_partner_in_age_group[age_group];
  else return -1;
  
  //  if (age_group == 0) return num_partners_15_19;
  //  else if (age_group == 1) return num_partners_20_24;
  //  else if (age_group == 2) return num_partners_25_29;
  //  else if (age_group == 3) return num_partners_30_34;
  //  else if (age_group == 4) return num_partners_35_39;
  //  else if (age_group == 5) return num_partners_40_44;
  //  else if (age_group == 6) return num_partners_45_54;
  //  else if (age_group == 7) return num_partners_55over;
}

int Relationships::get_matched_partner_id(int i){
  Person* partner = this->partner_list.at(i)->partner;
  return partner->get_id();
}

void Relationships::decrease_time_left_in_relationship(int i){
  //increase time_past_in_relationship by 1 DAY for partner i
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->dur_left--;
}

int Relationships::get_time_past_in_relationship(int i){
  //return time_past_in_relationship partner i
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  int dur_past = partner_ptr->dur - partner_ptr->dur_left;
  return dur_past;
}

int Relationships::get_time_left_in_relationship(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->dur_left;
}

void Relationships::set_num_sexual_act_partner_i(int i, int freq){
  //freq: frequency of sexual acts with that partner
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->num_sexual_acts = freq;
}

int Relationships::get_num_sexual_act_partner_i(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->num_sexual_acts;
}

void Relationships::add_sex_act_day(int i, int act_day){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->act_days_vec.push_back(act_day);
}

int Relationships::get_sex_act_day(int i, int act){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->act_days_vec.at(act);
  
}

void Relationships::sort_act_days_vec(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  std::sort ( partner_ptr->act_days_vec.begin(), partner_ptr->act_days_vec.end());
}

void Relationships::set_time_left_in_relationship(int i, int left){
  //add for each partner, the duration that has left in the relationship
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->dur_left = left;
}

void Relationships::set_concurrent_days_in_relationships(int i, int days){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->dur_concurrent = days;
}

int Relationships::get_concurrent_days_in_relationships(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->dur_concurrent;
}

void Relationships::set_partnerships_begin_end(int i, int begin, int end){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->start_day = begin;
  partner_ptr->end_day = end;
}

int Relationships::get_partner_start_day(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->start_day;
}

int Relationships::get_partner_end_day(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->end_day;
}

void Relationships::set_long_term_status(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->long_term_status = true;
}

bool Relationships::get_long_term_status(int i){
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  return partner_ptr->long_term_status;
}

void Relationships::reset_long_term_status(int i){      //not used
  Partner_Struct* partner_ptr = this->partner_list.at(i);
  partner_ptr->long_term_status = false;
}

void Relationships::terminate(int day, Sexual_Transmission_Network* sexual_network){
  FRED_VERBOSE(1, "TERMINATE RELATIONSHIPS for person %d day %d\n",
               this->myself->get_id(), day);
  
  //remove from partner's partner list
  for (int i = 0; i < this->get_partner_list_size(); i++) {
    Person* partner = this->get_partner_from_list(i);
    Relationships* partner_relationships = partner->get_relationships();
    
    for (int m = partner_relationships->get_partner_list_size(); m>0; m--) {
      int partner_id = partner_relationships->get_matched_partner_id(m-1);
      if (partner_id == myself->get_id()) {   //if the partner is 'myself'
        
        if (myself->get_id() == Global::TEST_ID) {
          cout << "Removing from partner list, becasue the person is deceased. deceased ID " << myself->get_id() << "  Partner ID " << partner->get_id() << endl;
          getchar();
        }
        partner_relationships->remove_from_partner_list(m-1);
        partner_relationships->set_person_num_partners_label((partner_relationships->get_person_num_partners_label())-1) ;
      }
    }
  }
  //FRED_VERBOSE(0, "update_enrollee_index: person %d place %d %s not found in daily activity locations: ",
  
  //remove from sexual transmission network
  if(myself->is_enrolled_in_network(sexual_network)) {
    myself->unenroll_network(sexual_network);
  }
}



void Relationships::print_relationship(){
  cout << "Print relationship Day " << Global::Simulation_Day << " - Person ID " << myself->get_id() << "  Sex " << myself->get_sex() << "  Age " << myself->get_age() << "  Age-group " << this->get_current_age_group() <<  "  Partners ever day 0: "  << this->get_person_num_partners_ever_day0() << "  Partners label " << this->get_person_num_partners_label()  << "  Matched partners " << this->get_partner_list_size() <<  endl;
}

void Relationships::set_initial_15cohort(){
  this->initial_15_cohort = true;
}

bool Relationships::get_initial_15cohort(){
  return this->initial_15_cohort;
}


