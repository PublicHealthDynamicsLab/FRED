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
//  Author: Mina Kabiri. Created on 6/1/16.
//
//
//
// File: Sexual_Transmission_Network.cc
//
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <iomanip>      // std::setprecision??

#include "Sexual_Transmission_Network.h"
#include "Condition.h"
#include "Global.h"
#include "Params.h"
#include "Place_List.h"
#include "Random.h"
#include "Person.h"
#include "Population.h"
#include "Relationships.h"


clock_t tStart;
clock_t cycle_time = 0;

//*******************************************************
FILE * report1;
FILE * report2;
FILE * excel_1 = NULL;
FILE * excel_2 = NULL;
FILE * report_matched = NULL;
FILE * report_partners_output = NULL;
FILE * report_QC = NULL;
FILE * report_sexual_acts = NULL;

ofstream * Sexual_Transmission_Network::xls_output = NULL;
ofstream * Sexual_Transmission_Network::xls_output_male_ever = NULL;
ofstream * Sexual_Transmission_Network::xls_output_female_ever = NULL;
ofstream * Sexual_Transmission_Network::xls_output_ever = NULL;

ofstream * Sexual_Transmission_Network::xls_output3 = NULL;
ofstream * Sexual_Transmission_Network::txt_output4 = NULL;

ifstream * Sexual_Transmission_Network::inFile3 = NULL;

Sexual_Transmission_Network::people_struct * Sexual_Transmission_Network::people_str = NULL;

//*******************************************************
//********** Calibration static parameters initial values
bool Sexual_Transmission_Network::BASH_SCRIPT = false;
int Sexual_Transmission_Network::calib_age_group = -1;
int Sexual_Transmission_Network::calib_column = -1;
double Sexual_Transmission_Network::calib_col0 = 0.0;
double Sexual_Transmission_Network::calib_col1 = 0.0;
double Sexual_Transmission_Network::calib_col2 = 0.0;
double Sexual_Transmission_Network::calib_col3 = 0.0;
char* Sexual_Transmission_Network::calib_filename = NULL;

//*******************************************************
static const double prob_mono_1 = 0.9;
static const double prob_mono_2 = 0.9;

static const double prob_2partner_1 = 1;     //for individuals in non-family households
static const double prob_2partner_2 = 1;     //for individuals in family households
static const double prob_3partner_1 = 1;     //for individuals in non-family households
static const double prob_3partner_2 = 1;     //for individuals in family households

static const double prob_overlap = 0.1522;   //decide if male has concurrent partners or not

//probability of sexual act on a day
static const double sexual_freq_male_15_29 = 0.247;
static const double sexual_freq_male_30_39 = 0.235;
static const double sexual_freq_male_40_49 = 0.187;
static const double sexual_freq_male_50_59 = 0.135;
static const double sexual_freq_male_60_69 = 0.080;
static const double sexual_freq_male_over70 = 0.031;

//*******************************************************
double Sexual_Transmission_Network::partner_male_lifetime_stat[ROW_ever][COL_partner_categories_extended] = {{0.0}};
double Sexual_Transmission_Network::partner_female_lifetime_stat[ROW_ever][COL_partner_categories_extended] = {{0.0}};

double Sexual_Transmission_Network::f_partner_cumulative[ROW_age_group][COL_partner_categories_extended] = {{0.0}};
double Sexual_Transmission_Network::m_partner_cumulative[ROW_age_group][COL_partner_categories_extended] = {{0.0}};
double Sexual_Transmission_Network::f_mixing[3][3] = {{0.0}};
double Sexual_Transmission_Network::m_mixing[3][3] = {{0.0}};
double Sexual_Transmission_Network::f_partner_3ormore[6][5] = {{0.0}};
double Sexual_Transmission_Network::m_partner_3ormore[6][5] = {{0.0}};
double Sexual_Transmission_Network::f_dur[9][3] = {{0.0}};
double Sexual_Transmission_Network::m_dur[9][3] = {{0.0}};

double Sexual_Transmission_Network::m_partner_ever_day0[ROW_ever][COL_partner_categories] = {{0.0}};
double Sexual_Transmission_Network::f_partner_ever_day0[ROW_ever][COL_partner_categories] = {{0.0}};

double Sexual_Transmission_Network::cdf_lognorm[47] = {0.0};

//*******************************************************
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_15[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_16[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_17[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_18[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_19[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_20[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_21[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_22[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_23[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_24[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_25[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_26[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_27[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_28[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_29[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_30[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_31[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_32[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_33[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_34[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_35[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_36[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_37[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_38[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_39[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_40[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_41[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_42[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_43[10000];
Sexual_Transmission_Network::people_struct Sexual_Transmission_Network::people_44[10000];

//*******************************************************
vector<Person*> Sexual_Transmission_Network::females_to_match_1partner_FH;
vector<Person*> Sexual_Transmission_Network::males_to_match_1partner_FH;
vector<Person*> Sexual_Transmission_Network::females_to_match_1partner_nonFH;
vector<Person*> Sexual_Transmission_Network::males_to_match_1partner_nonFH;

vector<Person*> Sexual_Transmission_Network::females_to_match;
vector<Person*> Sexual_Transmission_Network::males_to_match;

vector<Person*> Sexual_Transmission_Network::females_to_match_1partner_byAge_FH[3];
vector<Person*> Sexual_Transmission_Network::males_to_match_1partner_byAge_FH[3];
vector<Person*> Sexual_Transmission_Network::females_to_match_1partner_byAge_nonFH[3];
vector<Person*> Sexual_Transmission_Network::males_to_match_1partner_byAge_nonFH[3];

vector<Person*> Sexual_Transmission_Network::females_to_match_byAge[3];
vector<Person*> Sexual_Transmission_Network::males_to_match_byAge[3];

vector<Person*> Sexual_Transmission_Network::females_to_label_monogamous[ROW_age_group];
vector<Person*> Sexual_Transmission_Network::females_to_label_2partner[ROW_age_group];
vector<Person*> Sexual_Transmission_Network::females_to_label_3partner[ROW_age_group];

//*******************************************************
double Sexual_Transmission_Network::f_partner[ROW_age_group][COL_partner_categories] = {{0.0}};
double Sexual_Transmission_Network::m_partner[ROW_age_group][COL_partner_categories] = {{0.0}};


int Sexual_Transmission_Network::female_labeled_1partner[ROW_age_group] = {0};
int Sexual_Transmission_Network::female_labeled_2partner[ROW_age_group] = {0};
int Sexual_Transmission_Network::female_labeled_3partner[ROW_age_group] = {0};

int Sexual_Transmission_Network::female_expected_partner[ROW_age_group][COL_partner_categories] = {{0}};
int Sexual_Transmission_Network::male_expected_partner[ROW_age_group][COL_partner_categories] = {{0}};
int Sexual_Transmission_Network::female_labeled_partner[ROW_age_group][COL_partner_categories] = {{0}};
int Sexual_Transmission_Network::male_labeled_partner[ROW_age_group][COL_partner_categories] = {{0}};

int Sexual_Transmission_Network::female_partner_inyear[ROW_age_group][COL_partner_categories_extended] = {{0}};
int Sexual_Transmission_Network::male_partner_inyear[ROW_age_group][COL_partner_categories_extended] = {{0}};

int Sexual_Transmission_Network::female_partner_ever[ROW_age_group][COL_partner_categories_extended] = {{0}};
int Sexual_Transmission_Network::male_partner_ever[ROW_age_group][COL_partner_categories_extended] = {{0}};

int Sexual_Transmission_Network::male_partner_inyear_matched[ROW_ever1][COL_partner_categories_extended] = {{0}};
int Sexual_Transmission_Network::female_partner_inyear_matched[ROW_ever1][COL_partner_categories_extended] = {{0}};

int Sexual_Transmission_Network::male_partner_matched_total[ROW_age_group] = {0};
int Sexual_Transmission_Network::female_partner_matched_total[ROW_age_group] = {0};

//*******************************************************
//counters for counting people based on sex and age- for network?

int Sexual_Transmission_Network::female[ROW_age_group] = {0};
int Sexual_Transmission_Network::male[ROW_age_group] = {0};
int Sexual_Transmission_Network::male_agegroup0[5] = {0};

int Sexual_Transmission_Network::total_adult = 0;
int Sexual_Transmission_Network::total_female_adult = 0;
int Sexual_Transmission_Network::total_male_adult = 0;
int Sexual_Transmission_Network::total_male_adult_agegroup0 = 0;

int Sexual_Transmission_Network::total_female_1partner = 0;
int Sexual_Transmission_Network::female_1partner_labeled = 0;
int Sexual_Transmission_Network::total_female_2partner = 0;
int Sexual_Transmission_Network::female_2partner_labeled = 0;
int Sexual_Transmission_Network::total_female_3partner = 0;
int Sexual_Transmission_Network::female_3partner_labeled = 0;

//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************

//Private static variables that will be set by parameter lookups
Sexual_Transmission_Network::Sexual_Transmission_Network(const char* lab) : Network(lab) {
  FRED_VERBOSE(0, "Sexual_Tranmission_Network %s constructor\n", lab);
  this->set_type(Network::TYPE_NETWORK);
  this->set_subtype(Network::SUBTYPE_SEXUAL_PARTNER);
  this->set_id(Global::Places.get_new_place_id());
  printf("STN: type %c subtype %c id %d\n", get_type(), get_subtype(), get_id());
  this->sexual_contacts_per_day = 0.0;
  this->probability_of_transmission_per_contact = 0.0;
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::get_parameters() {
  Params::get_param("sexual_partner_contacts", &(this->sexual_contacts_per_day));
  Params::get_param("sexual_trans_per_contact", &(this->probability_of_transmission_per_contact));
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::initialize_static_variables(){
  
  string filename;
  for (int i = 15; i < 46; i++) {
    filename = "person_" + std::to_string(i) + ".txt";
    //cout << "The filename is: " << filename << endl;
    inFile3 = new ifstream(filename.c_str(), ios::in );
    if(inFile3 == NULL){
      cout << "ERROR: Cannot open people_15 file." << endl;
      getchar();}
    int age, partner_ever, temp_partner_in_year, dur, dur_past;
    int k = 0;
    
    while (k < 10000){
      * inFile3 >> age >> partner_ever >> temp_partner_in_year ;
      
      if (i==15) {
        people_15[k].age = age; people_15[k].partner_ever = partner_ever; people_15[k].temp_partner_in_year = temp_partner_in_year;
        //cout << "people, k= " << k << "\t" << people_15[k].age << "\t" << people_15[k].partner_ever << "\t" << people_15[k].temp_partner_in_year;
        if (temp_partner_in_year != 0) {
          for (int j=0; j<temp_partner_in_year; j++) {
            * inFile3 >> dur >> dur_past ; people_15[k].dur.push_back(dur); people_15[k].dur_past.push_back(dur_past);
            //cout << "\t" << dur << "\t" << dur_past;
            //cout << "\t" << people_15[k].dur.at(j) << "\t" << people_15[k].dur_past.at(j);
          }
        }
        //cout << endl;
      }
      else if (i==16){
        people_16[k].age = age; people_16[k].partner_ever = partner_ever; people_16[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_16[k].dur.push_back(dur); people_16[k].dur_past.push_back(dur_past);}}}
      else if (i==17){
        people_17[k].age = age; people_17[k].partner_ever = partner_ever; people_17[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_17[k].dur.push_back(dur); people_17[k].dur_past.push_back(dur_past);}}}
      else if (i==18){
        people_18[k].age = age; people_18[k].partner_ever = partner_ever; people_18[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_18[k].dur.push_back(dur); people_18[k].dur_past.push_back(dur_past);}}}
      else if (i==19){
        people_19[k].age = age; people_19[k].partner_ever = partner_ever; people_19[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_19[k].dur.push_back(dur); people_19[k].dur_past.push_back(dur_past);}}}
      else if (i==20){
        people_20[k].age = age; people_20[k].partner_ever = partner_ever; people_20[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_20[k].dur.push_back(dur); people_20[k].dur_past.push_back(dur_past);}}}
      else if (i==21){
        people_21[k].age = age; people_21[k].partner_ever = partner_ever; people_21[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_21[k].dur.push_back(dur); people_21[k].dur_past.push_back(dur_past);}}}
      else if (i==22){
        people_22[k].age = age; people_22[k].partner_ever = partner_ever; people_22[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_22[k].dur.push_back(dur); people_22[k].dur_past.push_back(dur_past);}}}
      else if (i==23){
        people_23[k].age = age; people_23[k].partner_ever = partner_ever; people_23[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_23[k].dur.push_back(dur); people_23[k].dur_past.push_back(dur_past);}}}
      else if (i==24){
        people_24[k].age = age; people_24[k].partner_ever = partner_ever; people_24[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_24[k].dur.push_back(dur); people_24[k].dur_past.push_back(dur_past);}}}
      else if (i==25){
        people_25[k].age = age; people_25[k].partner_ever = partner_ever; people_25[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_25[k].dur.push_back(dur); people_25[k].dur_past.push_back(dur_past);}}}
      else if (i==26){
        people_26[k].age = age; people_26[k].partner_ever = partner_ever; people_26[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_26[k].dur.push_back(dur); people_26[k].dur_past.push_back(dur_past);}}}
      else if (i==27){
        people_27[k].age = age; people_27[k].partner_ever = partner_ever; people_27[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_27[k].dur.push_back(dur); people_27[k].dur_past.push_back(dur_past);}}}
      else if (i==28){
        people_28[k].age = age; people_28[k].partner_ever = partner_ever; people_28[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_28[k].dur.push_back(dur); people_28[k].dur_past.push_back(dur_past);}}}
      else if (i==29){
        people_29[k].age = age; people_29[k].partner_ever = partner_ever; people_29[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_29[k].dur.push_back(dur); people_29[k].dur_past.push_back(dur_past);}}}
      else if (i==30){
        people_30[k].age = age; people_30[k].partner_ever = partner_ever; people_30[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_30[k].dur.push_back(dur); people_30[k].dur_past.push_back(dur_past);}}}
      else if (i==31){
        people_31[k].age = age; people_31[k].partner_ever = partner_ever; people_31[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_31[k].dur.push_back(dur); people_31[k].dur_past.push_back(dur_past);}}}
      else if (i==32){
        people_32[k].age = age; people_32[k].partner_ever = partner_ever; people_32[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_32[k].dur.push_back(dur); people_32[k].dur_past.push_back(dur_past);}}}
      else if (i==33){
        people_33[k].age = age; people_33[k].partner_ever = partner_ever; people_33[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_33[k].dur.push_back(dur); people_33[k].dur_past.push_back(dur_past);}}}
      else if (i==34){
        people_34[k].age = age; people_34[k].partner_ever = partner_ever; people_34[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_34[k].dur.push_back(dur); people_34[k].dur_past.push_back(dur_past);}}}
      else if (i==35){
        people_35[k].age = age; people_35[k].partner_ever = partner_ever; people_35[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_35[k].dur.push_back(dur); people_35[k].dur_past.push_back(dur_past);}}}
      else if (i==36){
        people_36[k].age = age; people_36[k].partner_ever = partner_ever; people_36[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_36[k].dur.push_back(dur); people_36[k].dur_past.push_back(dur_past);}}}
      else if (i==37){
        people_37[k].age = age; people_37[k].partner_ever = partner_ever; people_37[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_37[k].dur.push_back(dur); people_37[k].dur_past.push_back(dur_past);}}}
      else if (i==38){
        people_38[k].age = age; people_38[k].partner_ever = partner_ever; people_38[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_38[k].dur.push_back(dur); people_38[k].dur_past.push_back(dur_past);}}}
      else if (i==39){
        people_39[k].age = age; people_39[k].partner_ever = partner_ever; people_39[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_39[k].dur.push_back(dur); people_39[k].dur_past.push_back(dur_past);}}}
      else if (i==40){
        people_40[k].age = age; people_40[k].partner_ever = partner_ever; people_40[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_40[k].dur.push_back(dur); people_40[k].dur_past.push_back(dur_past);}}}
      else if (i==41){
        people_41[k].age = age; people_41[k].partner_ever = partner_ever; people_41[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_41[k].dur.push_back(dur); people_41[k].dur_past.push_back(dur_past);}}}
      else if (i==42){
        people_42[k].age = age; people_42[k].partner_ever = partner_ever; people_42[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_42[k].dur.push_back(dur); people_42[k].dur_past.push_back(dur_past);}}}
      else if (i==43){
        people_43[k].age = age; people_43[k].partner_ever = partner_ever; people_43[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_43[k].dur.push_back(dur); people_43[k].dur_past.push_back(dur_past);}}}
      else if (i==44){
        people_44[k].age = age; people_44[k].partner_ever = partner_ever; people_44[k].temp_partner_in_year = temp_partner_in_year;
        if (temp_partner_in_year != 0) { for (int j=0; j<temp_partner_in_year; j++) { * inFile3 >> dur >> dur_past ; people_44[k].dur.push_back(dur); people_44[k].dur_past.push_back(dur_past);}}}
      
      k++;
    }   //end of while
  }
  
}   //end of initialize_static_variables()

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::open_input_files(){
  ifstream inFile1, inFile2;
  
  if (LABEL_FEMALES)
    inFile1.open("female_partner_age_cumulative.txt");
  else
    inFile1.open("f_partner_cumulative.txt");   //mina change values in this file
  
  inFile2.open("m_partner_cumulative.txt");
  
  for (int i=0; i<ROW_age_group; i++) {
    for (int j=0; j<COL_partner_categories_extended; j++) {
      inFile1 >> Sexual_Transmission_Network::f_partner_cumulative[i][j];
      inFile2 >> Sexual_Transmission_Network::m_partner_cumulative[i][j];
    }
  }
  inFile1.close(); inFile2.close();
  
  inFile1.open("female_partner_age.txt");
  inFile2.open("male_partner_age.txt");
  for (int i=0; i<ROW_age_group; i++) {
    for (int j=0; j<COL_partner_categories; j++) {
      inFile1 >> Sexual_Transmission_Network::f_partner[i][j];
      inFile2 >> Sexual_Transmission_Network::m_partner[i][j];
    }
  }
  inFile1.close(); inFile2.close();
  
  inFile1.open("female_mixing_cumulative.txt");
  inFile2.open("male_mixing_cumulative.txt");
  for (int i=0; i<3; i++) {
    for (int j=0; j<3; j++) {
      inFile1 >> Sexual_Transmission_Network::f_mixing[i][j];
      inFile2 >> Sexual_Transmission_Network::m_mixing[i][j];
    }
  }
  inFile1.close(); inFile2.close();
  inFile1.open("female_partner_3ormore.txt");
  inFile2.open("male_partner_3ormore.txt");
  for (int i=0; i<6; i++) {
    for (int j=0; j<5; j++) {
      inFile1 >> Sexual_Transmission_Network::f_partner_3ormore[i][j];
      inFile2 >> Sexual_Transmission_Network::m_partner_3ormore[i][j];
    }
  }
  inFile1.close(); inFile2.close();
  inFile1.open("female_dur.txt");
  inFile2.open("male_dur.txt");
  for (int i=0; i<9; i++) {
    for (int j=0; j<3; j++) {
      inFile1 >> Sexual_Transmission_Network::f_dur[i][j];
      inFile2 >> Sexual_Transmission_Network::m_dur[i][j];
    }
  }
  inFile1.close(); inFile2.close();
  
  inFile1.open("lognormal_4ormore.txt");
  for (int i=0; i<47; i++) {
    inFile1 >> Sexual_Transmission_Network::cdf_lognorm[i];
  }
  inFile1.close();
  
  inFile1.open("f_partner_ever_day0.txt");
  inFile2.open("m_partner_ever_day0.txt");
  for (int i=0; i<ROW_ever; i++) {
    for (int j=0; j<COL_partner_categories; j++) {
      inFile1 >> Sexual_Transmission_Network::f_partner_ever_day0[i][j];
      inFile2 >> Sexual_Transmission_Network::m_partner_ever_day0[i][j];
    }
  }
  inFile1.close(); inFile2.close();
} //end of open_input_files

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::open_output_files(){
  
  if( (report_matched = fopen( "report_matched.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  
  //  if( (report_partners_output = fopen( "report_partners_matched_IDs.txt", "w" )) == NULL ){
  //    printf("ERROR: The testing file was not opened\n" );
  //    exit(0);
  //  }
  
  if( (report_QC = fopen( "report_QC.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  
  if( (report_sexual_acts = fopen( "report_sexual_acts.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  
  //report number of adult individuals in each age group
  if( (report1 = fopen( "report_people.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  fprintf (report1, "Number of people based on age and sex.\n");
  fprintf (report1, "Age Group\tMale\tFemale\n");
  
  //report number of adult individuals in each age group
  if( (report2 = fopen( "report_expected_labeled.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  
  ostringstream xls_name1;
  ostringstream xls_name2;
  ostringstream xls_name3;
  ostringstream xls_name4;
  ostringstream txt_name4;
  
  if (BASH_SCRIPT == true){
    
    xls_name1 << "Males_EVER_" << calib_filename << ".txt";
    txt_name4 << "out_" << calib_filename << ".txt";
    //xls_name1 << "12months_" << filename1 << ".xls";
    //xls_name2 << "ever_" << filename1 << ".xls";
    //cout << txt_name4 << endl;
  }
  else {
    char filename1 = '1';
    //xls_name2 << "ever_" << filename1 << ".xls";
    txt_name4 << "out_" << filename1 << ".txt";
    xls_name1 << "Males_EVER_" << filename1 << ".txt";
  }
  
  xls_name3 << "person_age_debug.xls";

  xls_output = new ofstream(xls_name1.str().c_str(), ios::out );    //Males_EVR_1.txt
  
  xls_name2 << "ever_Male.xls";
  xls_name4 << "ever_Female.xls";
  xls_output_male_ever = new ofstream(xls_name2.str().c_str(), ios::out );   // "ever_Male.xls"
  xls_output_female_ever = new ofstream(xls_name4.str().c_str(), ios::out );   // "ever_Female.xls"

  xls_output3 = new ofstream(xls_name3.str().c_str(), ios::out ); //person debug file
  txt_output4 = new ofstream(txt_name4.str().c_str(), ios::out );   //new : out_*.txt
  
  * xls_output3
  << "Day" << '\t'
  << "ID" << '\t'
  << "temp age (in relationships) " << '\t'
  << "integer_age" << '\t'
  << "real_age" << '\t'
  << "age group" << '\t'
  << "Num partners"  << '\t'
  << "Num partners Ever" << '\t'
  << "Duration " << '\t'
  << "Duration past " << endl;
  
} // end of open_output_files

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::read_sexual_partner_file(char* sexual_partner_file) {
  FRED_VERBOSE(0, "read sexual partner file %s\n", sexual_partner_file);
  int id1, id2;
  FILE* fp = Utils::fred_open_file(sexual_partner_file);
  if (fp == NULL) {
    Utils::fred_abort("sexual_partner_file required\n");
  }
  
  while(fscanf(fp, "%d %d", &id1, &id2) == 2) {
    Person* partner1 = Global::Pop.get_person(id1);
    Relationships* relationships1 = partner1->get_relationships();
    
    if (relationships1->check_relationships_eligiblity()) {
      add_person_to_sexual_partner_network(partner1);
    }
    
    Person* partner2 = Global::Pop.get_person(id2);
    Relationships* relationships2 = partner2->get_relationships();
    
    if (relationships2->check_relationships_eligiblity()) {
      add_person_to_sexual_partner_network(partner2);
    }
    
    //cout << "read_sexual_partner_file ID1 " << partner1->get_id() << "  elig? " << relationships1->check_relationships_eligiblity() << "  ID2 " << partner2->get_id() << "  elig?" << relationships2->check_relationships_eligiblity() << endl;
    
    if (relationships1->check_relationships_eligiblity() && relationships2->check_relationships_eligiblity()){
      if(partner1->is_connected_to(partner2, this) == false) {
        partner1->create_network_link_to(partner2, this);
      }
      
      if(partner2->is_connected_to(partner1, this) == false) {
        partner2->create_network_link_to(partner1, this);
      }
    }
    
    //cout << "read_sexual_partner_file ID1 " << partner1->get_id() << "  ID2 " << partner2->get_id() << "  is connected? " << partner1->is_connected_to(partner2, this) << endl;
    
  }
  fclose(fp);
}

//*******************************************************
//*******************************************************

int Sexual_Transmission_Network::set_num_partner_day0(Person* &individual, double partner_ever_day0[ROW_ever][COL_partner_categories]){
  
  FRED_VERBOSE(1, "set_num_partner_day0 started. \n");
  int partner_number = -1;
  int temp = 0;
  double rand1 = ((double) rand() / (RAND_MAX));
  double rand2 = ((double) rand() / (RAND_MAX));
  double rand3 = ((double) rand() / (RAND_MAX));
  
  Relationships* relationships = individual->get_relationships();
  
  //partner_ever_day0 matrix JUST USED on Day 0
  //partner_ever_day0 was changed for age group 0 for day 0
  //partner_ever_day0. rows = 30 from 0 to 29. row in partner_ever_day0 matrix = age - 15.
  int row1 = individual->get_age() - 15;
  
  if (individual->get_age() >= 15 && individual->get_age() < 45 ) {
    //mina check: individual->get_age() <= 44 changed to individual->get_age() < 45
    if (rand1 <= partner_ever_day0[row1][0] )
      partner_number = 0;
    else if (rand1 > partner_ever_day0[row1][0] && rand1 <= partner_ever_day0[row1][1] )
      partner_number = 1;
    else if (rand1 > partner_ever_day0[row1][1] && rand1 <= partner_ever_day0[row1][2] )
      partner_number = 2;
    else if (rand1 > partner_ever_day0[row1][2] )  //3 or more= max is 3 now
      partner_number = 3;

  }
  
  FRED_VERBOSE(1, "set_num_partner_day0 finished. partner_number = %d \n", partner_number);
  
  if (partner_number == -1) {
    FRED_VERBOSE(0, "ERROR in set_num_partner_day0(): partner number = -1 \n");
    relationships->print_relationship();
    exit(0);
  }
  else
    return partner_number;
  
} // end of set num_partner day0

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::adjust_concurrency_day0(){
  
  FRED_VERBOSE(0, "MINA Adjust Concurrency day 0 started.\n" );
  
  //begin and end days for monogamous males and their partner
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    
    if (person->get_sex() == 'M' && num_matched_partners == 1 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true ) {
      int time_left_partner0 = relationships->get_time_left_in_relationship(0);
      relationships->set_concurrent_days_in_relationships(0, 0);  //0 concurrent days
      relationships->set_partnerships_begin_end(0, 0, time_left_partner0);
      
      int partner_place = 0;
      Person* partner_female = relationships->get_partner_from_list(partner_place);
      Relationships* relationships_female = partner_female->get_relationships();
      int partners_female = relationships_female->get_partner_list_size();
      
      //QC check: female should have at least one partner:
      if (partners_female <= 0 || relationships_female->get_person_num_partners_label() <= 0 || partners_female > relationships_female->get_person_num_partners_label()) {
        cout << "ERROR: adjust_concurrency()- Female does not have matched male partner on list. Press key to continue." << endl;
        getchar();  //for error
      }
      for (int k = 0; k < partners_female; k++) {
        Person* partner_male = relationships_female->get_partner_from_list(k);
        
        if (person->get_id() == partner_male->get_id()) {
          //if person (male) is the same as the male in k'th place on female's partner list
          relationships_female->set_concurrent_days_in_relationships(k, 0);  //0 concurrent days
          relationships_female->set_time_left_in_relationship(k, relationships->get_partner_dur_left(0));
          int start = relationships->get_partner_start_day(0);
          int end = relationships->get_partner_end_day(0);
          relationships_female->set_partnerships_begin_end(k, start, end);
        }
      }
    }
  }
  
  //prepare a vector of MALES in sexual network who has 2 or more matched partners
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    
    if (person->get_sex() == 'M' && relationships->get_person_num_partners_label() >= 2 && num_matched_partners >= 2 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true ) {
      p_vector.push_back(person);
      
      //debug
      if (relationships->get_person_num_partners_label() < num_matched_partners) {
        cout << "ERROR in adjust_concurrency() num_partners() < num_matched_partners.press key to continue." << endl;
        getchar();  //for error
      }
    }
  }
  
  int this_year = Global::Simulation_Day / 365;
  int start_day_this_year = this_year*365;
  int end_day_this_year = (this_year+1)*365;
  
  for (int i = 0; i < p_vector.size(); i++) {
    Person* person = p_vector.at(i);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    int concurrent1 = 0;
    //pick partner with longest partnership duration left
    int longest_index = 0;
    for (int j = 0; j < num_matched_partners-1; j++) {
      if (relationships->get_partner_dur_left(j+1) > relationships->get_partner_dur_left(longest_index))
        longest_index = j+1;
    }
    //now do concurrency two by two
    for (int j = 0; j < num_matched_partners; j++) {
      if (j != longest_index) {
        int time_left_partner_longest = relationships->get_time_left_in_relationship(longest_index);
        int time_left_partner1 = relationships->get_time_left_in_relationship(j);
        
        if (time_left_partner_longest >= 365 && time_left_partner1 >= 365){
          concurrent1 = 365;
          relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
          relationships->set_concurrent_days_in_relationships(j, concurrent1);
          relationships->set_partnerships_begin_end(longest_index, 0, time_left_partner_longest);
          relationships->set_partnerships_begin_end(j, 0, time_left_partner1);
        }
        else if (time_left_partner_longest >= 365 && time_left_partner1 < 365){
          concurrent1 = relationships->get_time_left_in_relationship(j);
          relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
          relationships->set_concurrent_days_in_relationships(j, concurrent1);
          relationships->set_partnerships_begin_end(longest_index, 0, time_left_partner_longest);
          //partner j could start any time during the year as long as all the duration fit in the same year.
          int latest_start_day = end_day_this_year - time_left_partner1;
          int start_day = get_uniform(start_day_this_year, latest_start_day);
          relationships->set_partnerships_begin_end(j, start_day, start_day + time_left_partner1);
          
        }
        //        else if (time_left_partner_longest < 365 && time_left_partner1 >= 365){
        //          concurrent1 = relationships->get_time_left_in_relationship(longest_index);
        //          relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
        //          relationships->set_concurrent_days_in_relationships(j, concurrent1);
        //          relationships->set_partnerships_begin_end(longest_index, 0, time_left_partner_longest);//partner longest_index could start later in the year, but since we are on day 0, we let her start on day 0 as well to make it simple
        //          relationships->set_partnerships_begin_end(j, 0, time_left_partner1);
        //        }
        
        else if (time_left_partner_longest < 365 && time_left_partner1 < 365){
          
          //if the sum of two <365 dur relationships is great that 365 days, they still would overlap in the year: min = the difference of durations left, max = the shorter duration left among the two
          if (relationships->get_time_left_in_relationship(longest_index) + relationships->get_time_left_in_relationship(j) > 365) {
            int min = int(abs(time_left_partner_longest + time_left_partner1 - 365));
            int max = std::min(time_left_partner_longest, time_left_partner1);
            
            concurrent1 = get_uniform(min, max);
            //pick which relationships starts first
            double rand1 = ((double) rand() / (RAND_MAX));
            int first_partner = 0;
            int second_partner = 0;
            int time_left_partner_first = 0;
            int time_left_partner_second = 0;
            
            if (rand1 <= 0.5) {   //longest_index starts first
              first_partner = longest_index;
              second_partner = j;
              time_left_partner_first = time_left_partner_longest;
              time_left_partner_second = time_left_partner1;
            }
            else {      //partner j starts first
              first_partner = j;
              second_partner = longest_index;
              time_left_partner_first = time_left_partner1;
              time_left_partner_second = time_left_partner_longest;
            }
            
            //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
            int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
            int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
            relationships->set_partnerships_begin_end(first_partner, start_day_first, start_day_first + time_left_partner_first);
            int start_day_partner_second = start_day_first + time_left_partner_second - concurrent1;
            int end_day_partner_second = start_day_partner_second + time_left_partner_second;
            relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
          }
          
          //if the sume of two <365 dur relationships is less that 365 days, they MIGHT overlap in the year: min = 1, max = shorter duration among the two
          else {
            double rand2 = ((double) rand() / (RAND_MAX));
            if (rand2 <= prob_overlap) {    //overlap
              int min = 1;
              int max = std::min(time_left_partner_longest, time_left_partner1);
              concurrent1 = get_uniform(min, max);
              
              //pick which relationships starts first
              double rand1 = ((double) rand() / (RAND_MAX));
              int first_partner = 0;
              int second_partner = 0;
              int time_left_partner_first = 0;
              int time_left_partner_second = 0;
              
              if (rand1 <= 0.5) {   //longest_index starts first
                first_partner = longest_index;
                second_partner = j;
                time_left_partner_first = time_left_partner_longest;
                time_left_partner_second = time_left_partner1;
              }
              else {      //partner j starts first
                first_partner = j;
                second_partner = longest_index;
                time_left_partner_first = time_left_partner1;
                time_left_partner_second = time_left_partner_longest;
              }
              
              //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
              int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
              int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
              relationships->set_partnerships_begin_end(first_partner, start_day_first, start_day_first + time_left_partner_first);
              int start_day_partner_second = start_day_first + time_left_partner_second - concurrent1;
              int end_day_partner_second = start_day_partner_second + time_left_partner_second;
              relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
              
            }
            else {    //no overlap
              int min = 0;
              int max = 0;
              concurrent1 = 0;
              
              //pick which relationships starts first
              double rand1 = ((double) rand() / (RAND_MAX));
              int first_partner = 0;
              int second_partner = 0;
              int time_left_partner_first = 0;
              int time_left_partner_second = 0;
              
              if (rand1 <= 0.5) {   //longest_index starts first
                first_partner = longest_index;
                second_partner = j;
                time_left_partner_first = time_left_partner_longest;
                time_left_partner_second = time_left_partner1;
              }
              else {      //partner j starts first
                first_partner = j;
                second_partner = longest_index;
                time_left_partner_first = time_left_partner1;
                time_left_partner_second = time_left_partner_longest;
              }
              
              //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
              int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
              int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
              int end_day_first = start_day_first + time_left_partner_first;
              
              relationships->set_partnerships_begin_end(first_partner, start_day_first, end_day_first);
              int latest_start_day_second = end_day_this_year - time_left_partner_second;
              
              int start_day_partner_second = get_uniform(end_day_first + 1, latest_start_day_second);
              int end_day_partner_second = start_day_partner_second + time_left_partner_second;
              relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
            }
          }
          
          //mina change: calculate these, for partners who do not have longest partnership...
          relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
          relationships->set_concurrent_days_in_relationships(j, concurrent1);
          
          if (person->get_id() == Global::TEST_ID){
            cout << "\nDEBUG1 Adjust concurrency. " << endl;
            relationships->print_relationship();
            cout << "\nlongest_index " << longest_index << "  concurrent1 " << concurrent1 << endl;
            
            for (int j = 0; j < num_matched_partners; j++) {
              cout << "Partner " << j << "\tDur total\t" << relationships->get_partner_dur(j) << "\tDur left\t" << relationships->get_time_left_in_relationship(j) << "\tStart day\t" << relationships->get_partner_start_day(j) << "\tEnd day\t" << relationships->get_partner_end_day(j) << endl;
              //getchar();    //for TEST_ID
            }
          }
          
        }
        
      }
    }
    
    //mina check: use later: int draw_concurrent_days = 30* int(get_lognormal2()); //convert to days
    
    //set up the dur left and start/end dates for this male's female partners:
    for (int j = 0; j < num_matched_partners; j++) {
      Person* partner_female = relationships->get_partner_from_list(j);
      Relationships* relationships_female = partner_female->get_relationships();
      int partners_female = relationships_female->get_partner_list_size();
      
      //QC check: female should have at least one partner:
      if (partners_female <= 0 || relationships_female->get_person_num_partners_label() <= 0 || partners_female > relationships_female->get_person_num_partners_label()) {
        cout << "ERROR: adjust_concurrency()- Female does not have matched male partner on list. Press key to continue." << endl;
        getchar();  //for error
      }
      
      for (int k = 0; k < partners_female; k++) {
        Person* partner_male = relationships_female->get_partner_from_list(k);
        
        if (person->get_id() == partner_male->get_id()) {
          //if person (male) is the same as the male in k'th place on female's partner list
          int concurrent_days = relationships->get_concurrent_days_in_relationships(j);
          relationships_female->set_concurrent_days_in_relationships(k, concurrent_days);
          relationships_female->set_time_left_in_relationship(k, relationships->get_partner_dur_left(j));
          int start = relationships->get_partner_start_day(j);
          int end = relationships->get_partner_end_day(j);
          relationships_female->set_partnerships_begin_end(k, start, end);
        }
      }
    } //end of setup for female partners of this male
    
  }  //end of for
  p_vector.clear();
  FRED_VERBOSE(1, "MINA Adjust Concurrency day 0 finished.\n" );
}   //end of adjust_concurrency day 0

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::setup(int argc, char* argv[]) {
  
  //read input files
  initialize_static_variables();
  open_input_files();
  
  if (argc == ARGC_expected ) {
    Sexual_Transmission_Network::BASH_SCRIPT = true;
    cout << " MINA: argc == " << ARGC_expected << endl;
    
    //**********************************
    //replace some parameters in partner_ever matrix
    //set_num_partner_day0() uses partner_ever matrix only.
    //set_num_partner_day() uses partner_cumulative matrix only.
    
    calib_filename = argv[2];
    calib_age_group = atoi( argv[3] );
    calib_column = atoi( argv[4] );
    
    calib_col0 = atof(argv[5]);
    calib_col1 = atof(argv[6]);
    calib_col2 = atof(argv[7]);
    calib_col3 = atof(argv[8]);
    
    //replace proability values in matrix
    //    m_partner_cumulative[calib_age_group][0] = calib_col0;
    //    m_partner_cumulative[calib_age_group][1] = calib_col1;
    //    m_partner_cumulative[calib_age_group][2] = calib_col2;
    //    m_partner_cumulative[calib_age_group][3] = calib_col3;
    
    for (int k = 0; k <3; k++) {   // k=0 is 15 year olds, k=1 16 yr, k=2 17 yr
      m_partner_ever_day0[k][0] = atof(argv[9]);
      m_partner_ever_day0[k][1] = atof(argv[10]);
      m_partner_ever_day0[k][2] = atof(argv[11]);
    }
    cout << "Calibrating age group " << calib_age_group << " and column " << calib_column << endl;
    //cout << m_partner_ever_day0[0][0] << '\t' << m_partner_ever_day0[0][1] << '\t' << m_partner_ever_day0[0][2] << endl;
  }
  
  open_output_files();  //depends on BASH_SCRIPT. so keep it after if (argc == ARGC_expected )
  
  //set optional parameters. moved from the beginning of this method.
  Params::disable_abort_on_failure();
  char sexual_partner_file[FRED_STRING_SIZE];
  strcpy(sexual_partner_file, "");
  Params::get_param("sexual_partner_file", sexual_partner_file);
  if (strcmp(sexual_partner_file,"") != 0) {
    read_sexual_partner_file(sexual_partner_file);
  }
  
  //**********************************
  FRED_VERBOSE(0, "setup relationships for adults\n");
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    relationships->setup(person);
  }
  FRED_VERBOSE(0, "setup relationships for adults finished\n");
  
  /**********************************
  if (LABEL_FEMALES) {
    // count adult females : need to be before labeling females
    count_adult_females();
    // count expected females in network: should be after count_adult_females();
    label_females_day0();
  } //otherwise, we have another method to label females
  
  **********************************/
  //labeling males - set up initial network on day 0 for MALES
  FRED_VERBOSE(0, "setup initial network on day 0 for MALES\n");
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    
    Person* person = Global::Pop.get_person(p);
    FRED_VERBOSE(0, "get_relationships for person %d\n", person->get_id());
    Relationships* relationships = person->get_relationships();
    
    //**********************************
    //****** MALES
    //****** age group 0 (15-19).
    if (person->get_sex() == 'M' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() == 0){
      
      int num_partners_time0 = set_num_partner_day0(person, m_partner_ever_day0);   //return a value >= 0. exits if <0 to exclude -1s as well
      relationships->set_person_num_partners_day0(num_partners_time0, num_partners_time0);
      
       int age_group = 0;
      
      //******* Calibration
      if (person->get_age() == 16) {
        double rand1 = ((double) rand() / (RAND_MAX));
        if (rand1 <= 0.742429)
          relationships->set_person_num_partners_day0(num_partners_time0, 0);
        else if (rand1 > 0.742429 && rand1 <= (0.742429 + 0.154024) )
          relationships->set_person_num_partners_day0(num_partners_time0, 1);
        else if (rand1 > (0.742429 + 0.154024) && rand1 <= (0.742429 + 0.154024+ 0.037929))
          relationships->set_person_num_partners_day0(num_partners_time0, 2);
        else if (rand1 > (0.742429 + 0.154024+ 0.037929) )
          relationships->set_person_num_partners_day0(num_partners_time0, 3);
      }
      
      else if (person->get_age() == 17) {
        double rand1 = ((double) rand() / (RAND_MAX));
        if (rand1 <= 0.5484)
          relationships->set_person_num_partners_day0(num_partners_time0, 0);
        else if (rand1 > 0.5484 && rand1 <= (0.5484 + 0.1991) )
          relationships->set_person_num_partners_day0(num_partners_time0, 1);
        else if (rand1 > (0.5484 + 0.1991) && rand1 <= (0.5484 + 0.1991+ 0.0762))
          relationships->set_person_num_partners_day0(num_partners_time0, 2);
        else if (rand1 > (0.5484 + 0.1991+ 0.0762) )
          relationships->set_person_num_partners_day0(num_partners_time0, 3);
      }
      
      else if (person->get_age() == 18) {
        double rand1 = ((double) rand() / (RAND_MAX));
        if (rand1 <= 0.3995)
          relationships->set_person_num_partners_day0(num_partners_time0, 0);
        else if (rand1 > 0.3995 && rand1 <= (0.3995 + 0.2075) )
          relationships->set_person_num_partners_day0(num_partners_time0, 1);
        else if (rand1 > (0.3995 + 0.2075) && rand1 <= (0.3995 + 0.2075+ 0.0832))
          relationships->set_person_num_partners_day0(num_partners_time0, 2);
        else if (rand1 > (0.3995 + 0.2075+ 0.0832) )
          relationships->set_person_num_partners_day0(num_partners_time0, 3);
      }

      else if (person->get_age() == 19) {
        double rand1 = ((double) rand() / (RAND_MAX));
        if (rand1 <= 0.2970)
          relationships->set_person_num_partners_day0(num_partners_time0, 0);
        else if (rand1 > 0.2970 && rand1 <= (0.2970 + 0.1828) )
          relationships->set_person_num_partners_day0(num_partners_time0, 1);
        else if (rand1 > (0.2970 + 0.1828) && rand1 <= (0.2970 + 0.1828+ 0.0855))
          relationships->set_person_num_partners_day0(num_partners_time0, 2);
        else if (rand1 > (0.2970 + 0.1828+ 0.0855) )
          relationships->set_person_num_partners_day0(num_partners_time0, 3);
      }

    }
    
    //****** MALES age groups 1-5
    
    if (person->get_sex() == 'M' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() >= 1 && relationships->get_current_age_group() < 6){
      
      int num_partners_ever_time0 = set_num_partner_day0(person, m_partner_ever_day0);    //can only be equal to 0, 1, 2 or 3 for now
      
      bool found_match = false;
      int num_tries = 0;
      while (!found_match || num_tries < 2000) {
        int rand_num1 = rand() % 10000;  //random number from 0 to 9999
        if (rand_num1 == 10000) {cout << "ERROR " << endl; getchar(); } //for error
        int age = person->get_age();
     
        if (age == 20) people_str = &people_20[rand_num1];
        else if (age == 21) people_str = &people_21[rand_num1];
        else if (age == 22) people_str = &people_22[rand_num1];
        else if (age == 23) people_str = &people_23[rand_num1];
        else if (age == 24) people_str = &people_24[rand_num1];
        else if (age == 25) people_str = &people_25[rand_num1];
        else if (age == 26) people_str = &people_26[rand_num1];
        else if (age == 27) people_str = &people_27[rand_num1];
        else if (age == 28) people_str = &people_28[rand_num1];
        else if (age == 29) people_str = &people_29[rand_num1];
        else if (age == 30) people_str = &people_30[rand_num1];
        else if (age == 31) people_str = &people_31[rand_num1];
        else if (age == 32) people_str = &people_32[rand_num1];
        else if (age == 33) people_str = &people_33[rand_num1];
        else if (age == 34) people_str = &people_34[rand_num1];
        else if (age == 35) people_str = &people_35[rand_num1];
        else if (age == 36) people_str = &people_36[rand_num1];
        else if (age == 37) people_str = &people_37[rand_num1];
        else if (age == 38) people_str = &people_38[rand_num1];
        else if (age == 39) people_str = &people_39[rand_num1];
        else if (age == 40) people_str = &people_40[rand_num1];
        else if (age == 41) people_str = &people_41[rand_num1];
        else if (age == 42) people_str = &people_42[rand_num1];
        else if (age == 43) people_str = &people_43[rand_num1];
        else if (age == 44) people_str = &people_44[rand_num1];
        
        if (people_str->partner_ever == num_partners_ever_time0 && num_partners_ever_time0 >= 0) {
          
          int temp_partner_in_year = people_str->temp_partner_in_year;
          
          relationships->set_person_num_partners_day0(temp_partner_in_year, num_partners_ever_time0);
          
          //duration for these individuals is read from input
          if (temp_partner_in_year > 0) {
            for (int j = 0; j < temp_partner_in_year; j++) {
              
              //********** big changes here
              //**********
              //**********
              // REMOVED relationships->add_to_partner_list(NULL); //mina check : be careful here. some people might not find enough partners for matching, to fill the empty spot created for the partner here
              //relationships->set_partner_dur(j, 30*(people_str->dur.at(j)));   //convert to days
              //relationships->set_time_past_in_relationship(j, 30*(people_str->dur_past.at(j)));
              // int dur_left = relationships->get_partner_dur(j) - relationships->get_time_past_in_relationship(j);
              //relationships->set_time_left_in_relationship(j, dur_left);
              
              int duration = 30*(people_str->dur.at(j));   //convert to days
              int duration_past = 30*(people_str->dur_past.at(j));
              int duration_left = duration - duration_past;
              relationships->add_dur_day0(duration, duration_left);
              //**********
              //**********
              //**********
              
            }
          }
          
          break;
        }
        
        num_tries++;
      } //end of while
    }
    
    //****** MALES age group >= 6
    else if (person->get_sex() == 'M' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() >= 6){
      relationships->set_person_num_partners_day0(0, 0);
    }
    
    //**********************************
    //****** FEMALES
    if (!LABEL_FEMALES) {
      //****** age group 0 (15-19)
      if (person->get_sex() == 'F' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() == 0 ){
        
        int num_partners_time0 = set_num_partner_day0(person, f_partner_ever_day0);   //return a value >= 0. exits if <0 to exclude -1s as well
        relationships->set_person_num_partners_day0(num_partners_time0, num_partners_time0);
        
        int age_group = 0;
        
        //******* Calibration
        if (person->get_age() == 16) {
          double rand1 = ((double) rand() / (RAND_MAX));
          if (rand1 <= 0.742429)
            relationships->set_person_num_partners_day0(num_partners_time0, 0);
          else if (rand1 > 0.742429 && rand1 <= (0.742429 + 0.154024) )
            relationships->set_person_num_partners_day0(num_partners_time0, 1);
          else if (rand1 > (0.742429 + 0.154024) && rand1 <= (0.742429 + 0.154024+ 0.037929))
            relationships->set_person_num_partners_day0(num_partners_time0, 2);
          else if (rand1 > (0.742429 + 0.154024+ 0.037929) )
            relationships->set_person_num_partners_day0(num_partners_time0, 3);
        }
        
        else if (person->get_age() == 17) {
          double rand1 = ((double) rand() / (RAND_MAX));
          if (rand1 <= 0.5484)
            relationships->set_person_num_partners_day0(num_partners_time0, 0);
          else if (rand1 > 0.5484 && rand1 <= (0.5484 + 0.1991) )
            relationships->set_person_num_partners_day0(num_partners_time0, 1);
          else if (rand1 > (0.5484 + 0.1991) && rand1 <= (0.5484 + 0.1991+ 0.0762))
            relationships->set_person_num_partners_day0(num_partners_time0, 2);
          else if (rand1 > (0.5484 + 0.1991+ 0.0762) )
            relationships->set_person_num_partners_day0(num_partners_time0, 3);
        }
        
        else if (person->get_age() == 18) {
          double rand1 = ((double) rand() / (RAND_MAX));
          if (rand1 <= 0.3995)
            relationships->set_person_num_partners_day0(num_partners_time0, 0);
          else if (rand1 > 0.3995 && rand1 <= (0.3995 + 0.2075) )
            relationships->set_person_num_partners_day0(num_partners_time0, 1);
          else if (rand1 > (0.3995 + 0.2075) && rand1 <= (0.3995 + 0.2075+ 0.0832))
            relationships->set_person_num_partners_day0(num_partners_time0, 2);
          else if (rand1 > (0.3995 + 0.2075+ 0.0832) )
            relationships->set_person_num_partners_day0(num_partners_time0, 3);
        }
        
        else if (person->get_age() == 19) {
          double rand1 = ((double) rand() / (RAND_MAX));
          if (rand1 <= 0.2970)
            relationships->set_person_num_partners_day0(num_partners_time0, 0);
          else if (rand1 > 0.2970 && rand1 <= (0.2970 + 0.1828) )
            relationships->set_person_num_partners_day0(num_partners_time0, 1);
          else if (rand1 > (0.2970 + 0.1828) && rand1 <= (0.2970 + 0.1828+ 0.0855))
            relationships->set_person_num_partners_day0(num_partners_time0, 2);
          else if (rand1 > (0.2970 + 0.1828+ 0.0855) )
            relationships->set_person_num_partners_day0(num_partners_time0, 3);
        }
        
      }
      
      //****** FEMALES age groups 1-5
      if (person->get_sex() == 'F' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() >= 1 && relationships->get_current_age_group() < 6){
        
        int num_partners_ever_time0 = set_num_partner_day0(person, f_partner_ever_day0);    //can only be equal to 0, 1, 2 or 3 for now
        
        //if (num_partners_ever_time0 == 2 ) cout << num_partners_ever_time0 << endl;
        
        bool found_match = false;
        int num_tries = 0;
        while (!found_match || num_tries < 2000) {
          int rand_num1 = rand() % 10000;  //random number from 0 to 9999
          if (rand_num1 == 10000) {cout << "ERROR " << endl; getchar(); } //for error
          int age = person->get_age();
          
          if (age == 20) people_str = &people_20[rand_num1];
          else if (age == 21) people_str = &people_21[rand_num1];
          else if (age == 22) people_str = &people_22[rand_num1];
          else if (age == 23) people_str = &people_23[rand_num1];
          else if (age == 24) people_str = &people_24[rand_num1];
          else if (age == 25) people_str = &people_25[rand_num1];
          else if (age == 26) people_str = &people_26[rand_num1];
          else if (age == 27) people_str = &people_27[rand_num1];
          else if (age == 28) people_str = &people_28[rand_num1];
          else if (age == 29) people_str = &people_29[rand_num1];
          else if (age == 30) people_str = &people_30[rand_num1];
          else if (age == 31) people_str = &people_31[rand_num1];
          else if (age == 32) people_str = &people_32[rand_num1];
          else if (age == 33) people_str = &people_33[rand_num1];
          else if (age == 34) people_str = &people_34[rand_num1];
          else if (age == 35) people_str = &people_35[rand_num1];
          else if (age == 36) people_str = &people_36[rand_num1];
          else if (age == 37) people_str = &people_37[rand_num1];
          else if (age == 38) people_str = &people_38[rand_num1];
          else if (age == 39) people_str = &people_39[rand_num1];
          else if (age == 40) people_str = &people_40[rand_num1];
          else if (age == 41) people_str = &people_41[rand_num1];
          else if (age == 42) people_str = &people_42[rand_num1];
          else if (age == 43) people_str = &people_43[rand_num1];
          else if (age == 44) people_str = &people_44[rand_num1];
          
          if (people_str->partner_ever == num_partners_ever_time0 && num_partners_ever_time0 >= 0) {
            
            int temp_partner_in_year = people_str->temp_partner_in_year;
            
            relationships->set_person_num_partners_day0(temp_partner_in_year, num_partners_ever_time0);
            
            //duration for these individuals is read from input
            if (temp_partner_in_year > 0) {
              for (int j = 0; j < temp_partner_in_year; j++) {
                
                int duration = 30*(people_str->dur.at(j));   //convert to days
                int duration_past = 30*(people_str->dur_past.at(j));
                int duration_left = duration - duration_past;
                //relationships->add_dur_day0(duration, duration_left);  //mina check for female
              }
            }
            
            break;
          }
          num_tries++;
        } //end of while
      }
      
      //****** FEMALES age group >= 6
      else if (person->get_sex() == 'F' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_current_age_group() >= 6){
        relationships->set_person_num_partners_day0(0, 0);
      }
    }
    
    //**********************************/
    
    
    //debug
    if(person->get_id() == Global::TEST_ID && relationships->check_relationships_eligiblity()){
      cout << "\nNetwork initialization." << endl <<  "Day " << Global::Simulation_Day << "  ID " << person->get_id() << "  integer_age: " << person->get_age() << "  real_age: " << person->get_real_age() <<  "   age group: " << relationships->get_current_age_group() << "  Num partners label: "  << relationships->get_person_num_partners_label() << "  Num partners Ever day 0: "  << relationships->get_person_num_partners_ever_day0() <<  "  Num partners matched: "  << relationships->get_partner_list_size() << endl;
      //getchar();    //for TEST_ID
      
      * xls_output3
      << Global::Simulation_Day << '\t'
      << person->get_id() << '\t'
      << relationships->get_temp_age() << '\t'
      << person->get_age() << '\t'
      << person->get_real_age() << '\t'
      << relationships->get_current_age_group() << '\t'
      << relationships->get_person_num_partners_label() << '\t'
      << relationships->get_person_num_partners_ever_day0() ;
      * xls_output3 << endl;
    } //end of TEST_ID
    
  } // end of for setup initial network
  FRED_VERBOSE(0, "setup initial network on day 0 for MALES finished\n");
  
  //*********************************************
  
  if (!LABEL_FEMALES) count_adult_females();
  count_adult_males();
  report_adult_population();
  prepare_report_labeled();
  
  FRED_VERBOSE(0, "matching process\n");
  matching_process();
  FRED_VERBOSE(0, "matching process finished\n");
  adjust_concurrency_day0();
  FRED_VERBOSE(0, "adjust_concurrency finished\n");
  prepare_report_matched();
  
  //restore requiring parameters
  Params::set_abort_on_failure();
  
  //*********************************************
  //QC checkpoint:
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int num_partner = relationships->get_person_num_partners_label();
    int num_matched_partners = relationships->get_partner_list_size();
    
    if(person->get_sex() == 'M' && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true){
      
      if (num_partner < num_matched_partners ) {
        cout << "ERROR: sexual network setup(). Press key to continue. " << endl;
        relationships->print_relationship();
        getchar();  //for error
        exit(0);
      }
      
      fprintf(report_QC, "\nMale ID %d\tAge %d\t num_partner %d\tnum_matched_partners %d" , person->get_id(), person->get_age(), num_partner, num_matched_partners);
      
      for (int j = 0; j < num_matched_partners; j++) {
        Person* partner = relationships->get_partner_from_list(j);
        Relationships* partner_relationships = partner->get_relationships();
        
        int sexual_freq = relationships->get_num_sexual_act_partner_i(j);
        int dur_left = relationships->get_partner_dur_left(j);  //dur_left of relationship between male and his j-th partner
        int partner_partner_list_size = partner_relationships->get_partner_list_size(); //number of female's matched partner
        
        fprintf(report_QC, "\nFemale Partner %d\tID %d\tAge %d\tDur total %d\tDur left %d\tStart day %d\tEnd day %d\t", j, partner->get_id(), partner->get_age(), relationships->get_partner_dur(j), relationships->get_time_left_in_relationship(j), relationships->get_partner_start_day(j), relationships->get_partner_end_day(j));
        
        //        if (sexual_freq > 0) {
        //          fprintf(report_QC, "Act days this year: ");
        //          for (int k = 0; k < sexual_freq; k++) {
        //            int act_day = relationships->get_sex_act_day( j, k);
        //            fprintf(report_QC, "\t%d", act_day);
        //          }
        //        }
        
        //print partner j's partners: debug
        for (int m = 0; m < partner_partner_list_size; m++) {
          int male_partner_id = partner_relationships->get_matched_partner_id(m); //get male's ID for female
          if (male_partner_id == person->get_id()) {   //person is the same male partner we have here
            
            int acts = partner_relationships->get_num_sexual_act_partner_i(m);   //get males number of sexual acts for this female partner
            
            fprintf(report_QC, "\nMale Partner %d\tID %d\tAge %d\tDur total %d\tDur left %d\tStart day %d\tEnd day %d\t", j, person->get_id(), person->get_age(), partner_relationships->get_partner_dur(m), partner_relationships->get_time_left_in_relationship(m), partner_relationships->get_partner_start_day(m), partner_relationships->get_partner_end_day(m));
            
            if (acts > 0) {
              fprintf(report_QC, "Act days: ");
              for (int k = 0; k < acts; k++) {
                int act_day = partner_relationships->get_sex_act_day(m, k);
                fprintf(report_QC, "\t%d", act_day);
              }
            }
          }
        } //end of for m
      }
      fprintf(report_QC,"\n\n");
    }
  }
  
  //*********************************************
  int initial_15 = 0;
  
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    //    // calibration for males
    //    if (person->get_real_age() >= 15.0 && person->get_real_age() < 16.0 && person->get_sex() == 'F' ) {
    //      relationships->set_initial_15cohort();
    //      initial_15++;
    //    }
    
    if ( (person->get_real_age() >= 19.0 && person->get_age() < 20.0) ||
        (person->get_real_age() >= 24.0 && person->get_age() < 25.0) ||
        (person->get_real_age() >= 29.0 && person->get_age() < 30.0) ||
        (person->get_real_age() >= 34.0 && person->get_age() < 35.0) ||
        (person->get_real_age() >= 30.0 && person->get_age() < 40.0) ||
        (person->get_real_age() >= 44.0 && person->get_age() < 45.0) )  {
      
      update_stats_ever(person);
    }
  }
  
  //cout << "initial_15 people: " << initial_15 << endl;
  prepare_report4(0);
  
  FRED_VERBOSE(0, "Mina: Sexual Transmission Network Setup() Finished.\n");
  print();
  
}   //end of setup()

//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************
//*******************          **************************
//*******************  UPDATE  **************************
//*******************          **************************
//*******************************************************

void Sexual_Transmission_Network::update(int day){
  
  FRED_VERBOSE(0, "Sexual network update started.\n");
  
  //right at the beginning of day: update time in relationship to account for the day past (previous day) for people who have partners. Remove partners whose partnerships duration is past
  update_time_past_remove_partnerships();
  
  //****************************
  //at the beginning of day, add people (male or female) who are now 15 to the network (all people have a relationship object already):
  add_new_people_to_network();
  
  //****************************
  //update number of partners for males today, IF male's integer age has changed. So we updated person's number of partners after his birthday!
  FRED_VERBOSE(1, "Update number of partners for males and females today started.\n");

  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    relationships->set_same_age_category(1);    //reset
    int num_partners_addition = 0;
    
    //    if(person->get_id() == Global::TEST_ID){
    //      cout << "\n\nTEST_ID Sexual Network UPDATE() before recalculating num_partners_label:\n";
    //      relationships->print_relationship();
    //      cout << "  integer_age: " << person->get_age() << "  real_age: " << person->get_real_age();
    //      if (relationships->check_relationships_eligiblity())
    //        cout << "  relationship eligibiliy: YES." << endl;
    //      else
    //        cout << "  relationship eligibiliy: NO." << endl;
    //      //getchar();  //for TEST_ID
    //    }
    
    
    //****************************
    //****************************
    //**************************** Mina check: included females in the following :
    
    if (day != 0 /*&& person->get_sex() == 'M'*/ && relationships->check_relationships_eligiblity() ){
      
      int age_group_now = relationships->calculate_current_age_group();
      int age_group_on_day_before = relationships->get_current_age_group();
      
      //      if(person->get_id() == Global::TEST_ID){
      //        cout << "\n\nTEST_ID ********** " << endl;
      //        relationships->print_relationship();
      //        cout << "age_group now " << age_group_now << "  age-group day before: " << age_group_on_day_before << "  temp_age " << relationships->get_temp_age() << endl;
      //        cout << "************ " << endl;
      //        //getchar();  //for TEST_ID
      //      }
      
      //if integer age has changed today (different age today, but same age group): patient can update partner numbers
      if (relationships->get_temp_age() < person->get_age() && age_group_now == age_group_on_day_before){
        
        //        if(person->get_id() == Global::TEST_ID){
        //          cout << "TEST_ID ********** " << endl;
        //          cout << "double(person->get_age())  " << double(person->get_age()) << endl;
        //          cout << "person->get_real_age()  " << person->get_real_age() << endl;
        //          cout << "ceil(person->get_real_age() - 1)  " << ceil(person->get_real_age() - 1)  << endl;
        //          cout << "************ " << endl;
        //          //getchar();  //for TEST_ID
        //        }
        
        if (person->get_sex() == 'M')
          num_partners_addition = set_num_partner(person, m_partner_cumulative);
        else if (person->get_sex() == 'F' && !LABEL_FEMALES)
          num_partners_addition = set_num_partner(person, f_partner_cumulative);
        
        
      } //end of different age, same age group
      
      else if (relationships->get_temp_age() < person->get_age() && age_group_now != age_group_on_day_before){   //different age, different age group
        
        //        if(person->get_id() == Global::TEST_ID){
        //          cout << "TEST_ID **** " << endl;
        //          cout << "double(person->get_age())  " << double(person->get_age()) << endl;
        //          cout << "person->get_real_age()  " << person->get_real_age() << endl;
        //          cout << "ceil(person->get_real_age() - 1)  " << ceil(person->get_real_age() - 1)  << endl;
        //          cout << "************ " << endl;
        //          //getchar();   //for TEST_ID
        //        }
        
        //if age_group has changed this day, update statistics for person's previous age group, then recalculate num_pertners and update current age group
        //example: update at age 20, but stats are for 15-19
        //update lifetime partners stats when age group of person changes, before assigning new partners for new age. because we want the statistics of lifetime partners by certain ages in the population: for example if new age today is 20, we update lifetime partners for person from 15 to right before the age of 20
        relationships->set_same_age_category(0);
        relationships->set_current_age_group(relationships->calculate_current_age_group());
        
        // set_num_partner() returns the number of 'ADDITIONAL' partners to be added this year.
        if (person->get_sex() == 'M')
          num_partners_addition = set_num_partner(person, m_partner_cumulative);
        else if (person->get_sex() == 'F' && !LABEL_FEMALES)
          num_partners_addition = set_num_partner(person, f_partner_cumulative);

        
      }   //end of else: different age and different age group
      
      //****************************
      if (num_partners_addition >= 0) {
        
        int num_partners_now =  num_partners_addition + relationships->get_person_num_partners_label();
        relationships->set_person_num_partners_label(num_partners_now);
      }
    }
    
    relationships->set_temp_age(); //temp_age does not change unless person's integer age changes and then updated
    
    if(person->get_id() == Global::TEST_ID){
      * xls_output3
      << Global::Simulation_Day << '\t'
      << person->get_id() << '\t'
      << relationships->get_temp_age() << '\t'
      << person->get_age() << '\t'
      << person->get_real_age() << '\t'
      << relationships->get_current_age_group() << '\t'
      << relationships->get_person_num_partners_label() << '\t'
      << relationships->get_person_num_partners_ever_day0();
      
      //debug
      //      if(person->get_id() == Global::TEST_ID){
      //        cout << "TEST_ID Sexual Network UPDATE() AFTER recalculating num_partners_label:\n";
      //        relationships->print_relationship();
      //        // getchar();  //for TEST_ID
      //      }
      
      for (int j=0; j < relationships->get_partner_list_size(); j++) {
        cout << "dur: " << relationships->get_partner_dur(j) << "  dur_past: " << relationships->get_time_past_in_relationship(j) << endl;
        * xls_output3 << '\t' << relationships->get_partner_dur(j) << '\t' << relationships->get_time_past_in_relationship(j);
        if (relationships->get_partner_dur(j) < relationships->get_time_past_in_relationship(j)) {
          cout << "ERROR: partner_dur < time past relationship for TEST_ID. TEST_ID sex " << person->get_sex() << endl;
          getchar();  //for  error
        }
      }
      * xls_output3 << endl;
    }
    
  } //end of for all people
  
  FRED_VERBOSE(1, "Update number of partners for males and females today finished.\n");
  
  //****************************
  
  if (Global::Simulation_Day != 0) {
    
    //label_females();    //new females are added to network. label them here. //mina check and move
    
    matching_process();
    adjust_concurrency(day);
    set_sexual_acts(day);
    
    //**************************** for calibration
    //****************************
    int male_0partner_15cohort_ever = 0;
    int male_1partner_15cohort_ever = 0;
    int male_2partner_15cohort_ever = 0;
    int male_3partner_15cohort_ever = 0;
    double total_15_cohort_ever = 0;
    
    int male_0partner_15cohort = 0;
    int male_1partner_15cohort = 0;
    int male_2partner_15cohort = 0;
    int male_3partner_15cohort = 0;
    double total_15_cohort = 0;
    //****************************
    //****************************
    
    //update total stats for partners in lifetime EVERY YEAR, and then reset counters every year
    if (Global::Simulation_Day%365 == 0) {
      
      count_adult_males();
      report_adult_population();
      
      int initial_15 = 0;
      for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
        Person* person = Global::Pop.get_person(p);
        Relationships* relationships = person->get_relationships();
        
        //if (!(relationships->is_same_age_category()) || person->get_age() == 44 )
        update_stats_ever(person);
        
        //15 year olds who are older now
        //        if (relationships->get_initial_15cohort() == true){
        //
        //          initial_15++;
        //
        //          if (relationships->get_partner_in_age_group(0) + relationships->get_person_num_partners_ever_day0() == 0) {
        //            male_0partner_15cohort_ever++;
        //          }
        //          else if (relationships->get_partner_in_age_group(0) + relationships->get_person_num_partners_ever_day0() == 1) {
        //            male_1partner_15cohort_ever++;
        //          }
        //          else if (relationships->get_partner_in_age_group(0) + relationships->get_person_num_partners_ever_day0() == 2) {
        //            male_2partner_15cohort_ever++;
        //          }
        //          else if (relationships->get_partner_in_age_group(0) + relationships->get_person_num_partners_ever_day0() == 3) {
        //            male_3partner_15cohort_ever++;
        //          }
        //          //*************************
        //
        //          if (relationships->get_partner_list_size() == 0) {
        //            male_0partner_15cohort++;
        //          }
        //          else if (relationships->get_partner_list_size() == 1) {
        //            male_1partner_15cohort++;
        //          }
        //          else if (relationships->get_partner_list_size() == 2) {
        //            male_2partner_15cohort++;
        //          }
        //          else if (relationships->get_partner_list_size() == 3) {
        //            male_3partner_15cohort++;
        //          }
        //        }
      }
      
      //cout << "initial_15 people: " << initial_15 << endl;
      
      //total_15_cohort = double(male_0partner_15cohort + male_1partner_15cohort + male_2partner_15cohort + male_3partner_15cohort);
      
      //cout << double(male_0partner_15cohort)/total_15_cohort << "\t" << double(male_1partner_15cohort)/total_15_cohort << "\t" << double(male_2partner_15cohort)/total_15_cohort << "\t" <<  double(male_3partner_15cohort)/total_15_cohort << endl;
      
      //*************************
      
      //total_15_cohort_ever = double(male_0partner_15cohort_ever + male_1partner_15cohort_ever + male_2partner_15cohort_ever + male_3partner_15cohort_ever);
      
      //cout << double(male_0partner_15cohort_ever)/total_15_cohort_ever << "\t" << double(male_1partner_15cohort_ever)/total_15_cohort_ever << "\t" << double(male_2partner_15cohort_ever)/total_15_cohort_ever << "\t" <<  double(male_3partner_15cohort_ever)/total_15_cohort_ever << endl;

      prepare_report4(int(Global::Simulation_Day/365));
    }
  }
}   //end of update(int day)

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::add_person_to_sexual_partner_network(Person* person) {
  if (Global::Enable_Transmission_Network && person->is_enrolled_in_network(this)==false) {
    person->join_network(Global::Sexual_Partner_Network);
  }
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::add_new_people_to_network(){
  FRED_VERBOSE(1, "MINA add_new_people_to_network() started.\n");
  
  int test_female = 0;
  
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    
    Person* person = Global::Pop.get_person(p);
    
    if (Global::Simulation_Day != 0 && person->get_real_age() >= 15.0 && person->get_real_age() < 16.0 && !(person->is_enrolled_in_network(this)) ){
      
      Relationships* relationships = person->get_relationships();
      
      //need to update age group for newly 15 year olds
      relationships->set_current_age_group(relationships->calculate_current_age_group());
      
      if (relationships->check_relationships_eligiblity()) {
        if(Global::Enable_Transmission_Network) {
          FRED_VERBOSE(1, "Joining transmission network: %d\n", person->get_id());
          person->join_network(Global::Transmission_Network);
          
        }
        if(Global::Enable_Sexual_Partner_Network) {
          Condition* condition = Global::Conditions.get_condition(0);   //condition_id = 0 for HIV.
          if(strcmp("sexual", condition->get_transmission_mode()) == 0) {
            add_person_to_sexual_partner_network(person);
          }
        }
        
        if (person->get_sex() == 'M'){
          int num_partners_time0 = set_num_partner_day0(person, m_partner_ever_day0);   //return a value >= 0. exits if <0 to exclude -1s as well
          relationships->set_person_num_partners_label(num_partners_time0);
          
        }
        else if (person->get_sex() == 'F' && !LABEL_FEMALES ){
          int num_partners_time0 = set_num_partner_day0(person, f_partner_ever_day0);
          relationships->set_person_num_partners_label(num_partners_time0);
          
          test_female++;
        }
      } // end of if (relationships->check_relationships_eligiblity()
    }
  } //end of for all people
  
  //cout << " number of 15 year olds (FEMALE) added today: " << test_female << endl;

  FRED_VERBOSE(1, "MINA add_new_people_to_network() finished.\n");
}

//*******************************************************
//*******************************************************

bool Sexual_Transmission_Network::age_mixing(int i, int j, double m_mixing[3][3]){    // i is male's age and j is female's age
  double rand1 = ((double) rand() / (RAND_MAX));
  bool ok_mixing = false;
  
  if (i<20) {
    if (j<20 && rand1<=m_mixing[0][0]) ok_mixing = true;
    else if (j>=20 && j<30 && rand1>m_mixing[0][0] && rand1<=m_mixing[0][1]) ok_mixing = true;
    else if (j>=30 && j<45 && rand1>m_mixing[0][1]) ok_mixing = true;
  }
  else if (i>=20 && i<30) {
    if (j<20 && rand1<=m_mixing[1][0]) ok_mixing = true;
    else if (j>=20 && j<30 && rand1>m_mixing[1][0] && rand1<=m_mixing[1][1]) ok_mixing = true;
    else if (j>=30 && j<45 && rand1>m_mixing[1][1]) ok_mixing = true;
  }
  else if (i>=30) {
    if (j<20 && rand1<=m_mixing[2][0]) ok_mixing = true;
    else if (j>=20 && j<30 && rand1>m_mixing[2][0] && rand1<=m_mixing[2][1]) ok_mixing = true;
    else if (j>=30 && rand1>m_mixing[2][1]) ok_mixing = true;
  }
  
  //  else if (i>=30 && i<45) {
  //    if (j<20 && rand1<=m_mixing[2][0]) ok_mixing = true;
  //    else if (j>=20 && j<30 && rand1>m_mixing[2][0] && rand1<=m_mixing[2][1]) ok_mixing = true;
  //    else if (j>=30 && j<45 && rand1>m_mixing[2][1]) ok_mixing = true;
  //  }
  else{
    cout << "ERROR in age_mixing method. Ages " << i << "\t" << j << endl;
    getchar(); //for error
  }
  
  return ok_mixing;
  
} // end of age_mixing

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::adjust_concurrency(int today){
  FRED_VERBOSE(1, "MINA Adjust Concurrency day %d started.\n", today );
  int this_year = Global::Simulation_Day / 365;
  int start_day_this_year = this_year*365;
  int end_day_this_year = (this_year+1)*365;
  
  //*******************************
  //begin and end days for monogamous males and their partner
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    
    
    if (person->get_sex() == 'M' && num_matched_partners == 1 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true ) {
      int partner_index = 0;
      
      //mina check
      //      if (relationships->get_partner_end_day(j) >= today)
      //        p_do_not_change_dates.push_back(j);
      //
   
      int time_left_partner0 = relationships->get_time_left_in_relationship(partner_index);
      relationships->set_concurrent_days_in_relationships(partner_index, 0);  //0 concurrent days
      relationships->set_partnerships_begin_end(partner_index, today, time_left_partner0);
      Person* partner_female = relationships->get_partner_from_list(partner_index);
      Relationships* relationships_female = partner_female->get_relationships();
      int partners_female = relationships_female->get_partner_list_size();
      
      
      if (person->get_id() == Global::TEST_ID) {
        relationships->print_relationship();
        cout << "time_left_partner0  " << time_left_partner0 << endl;
        cout << "Partner ID " << partner_female->get_id() << endl;
        relationships_female->print_relationship();
      }
      
      //QC check: female should have at least one partner:
      if (partners_female <= 0 || relationships_female->get_person_num_partners_label() <= 0 || partners_female > relationships_female->get_person_num_partners_label()) {
        cout << "ERROR: adjust_concurrency()- Female does not have matched male partner on list. Press key to continue." << endl;
        getchar();    //for error
      }
      
      for (int k = 0; k < partners_female; k++) {
        Person* partner_male = relationships_female->get_partner_from_list(k);
        
        if (person->get_id() == partner_male->get_id()) {
          //if person (male) is the same as the male in k'th place on female's partner list
          relationships_female->set_concurrent_days_in_relationships(k, 0);  //0 concurrent days. mina change: zero concurrent days is for the male and his own partners. the female might have other partners and might have concurrent days???
          relationships_female->set_time_left_in_relationship(k, relationships->get_partner_dur_left(partner_index));
          int start = relationships->get_partner_start_day(partner_index);
          int end = relationships->get_partner_end_day(partner_index);
          relationships_female->set_partnerships_begin_end(k, start, end);
        }
      }
    }
  }
  
  //*******************************
  //prepare a vector of MALES in sexual network who has 2 or more matched partners
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    
    if (person->get_sex() == 'M' && relationships->get_person_num_partners_label() >= 2 && num_matched_partners >= 2 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true ) {
      p_vector.push_back(person);
      
      //debug
      if (relationships->get_person_num_partners_label() < num_matched_partners) {
        cout << "ERROR in adjust_concurrency() num_partners() < num_matched_partners. Press key to continue." << endl;
        getchar();    //for error
      }
    }
  }
  
  //*******************************
  //adjust concurrency for non-monogamous males
  for (int i = 0; i < p_vector.size(); i++) {
    Person* person = p_vector.at(i);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    int concurrent1 = 0;
    
    //the partners whose partnerships have started the previous year and their partnership end-day is in this year, should not be assigned new begin and end days. Identify these partners and keep their partner_index in a vector to check for them later:
    vector<int> p_do_not_change_dates;
    for (int j = 0; j < num_matched_partners; j++) {
      if (relationships->get_partner_end_day(j) >= today)
        p_do_not_change_dates.push_back(j);
    }
    
    //pick partner with longest partnership duration left
    int longest_index = 0;
    for (int j = 0; j < num_matched_partners-1; j++) {
      if (relationships->get_partner_dur_left(j+1) > relationships->get_partner_dur_left(longest_index))
        longest_index = j+1;
    }
    
    //now do concurrency two by two
    for (int j = 0; j < num_matched_partners; j++) {
      concurrent1 = 0;
      if (j != longest_index) {
        bool j_do_not_change = false;
        bool longest_index_do_not_change = false;
        int time_left_partner_longest = relationships->get_time_left_in_relationship(longest_index);
        int time_left_partner_j = relationships->get_time_left_in_relationship(j);
        int end_day_partner_j = relationships->get_partner_end_day(j);
        
        for (int temp = 0; temp < p_do_not_change_dates.size(); temp++) {
          if (j == p_do_not_change_dates.at(temp))
            j_do_not_change = true;
          if (longest_index == p_do_not_change_dates.at(temp))
            longest_index_do_not_change = true;
        }
        
        if (j_do_not_change && longest_index_do_not_change) ; //do nothing
        else if (j_do_not_change && !longest_index_do_not_change) {  //j is fixed. adjust longest_index
          
          if (time_left_partner_longest >= 365){
            concurrent1 = 365;
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            relationships->set_partnerships_begin_end(longest_index, today, time_left_partner_longest);
          }
          //add longest_index to do not change vector
          p_do_not_change_dates.push_back(longest_index);
          longest_index_do_not_change = true;
        }
        else if (!j_do_not_change && longest_index_do_not_change) {  //longest_index is fixed. adjust j
          
          if (time_left_partner_longest >= 365 &&  time_left_partner_j >= 365){
            concurrent1 = 365;
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            relationships->set_partnerships_begin_end(j, today, time_left_partner_j);
            p_do_not_change_dates.push_back(j);
            j_do_not_change = true;
          }
          else if (time_left_partner_longest >= 365 && time_left_partner_j < 365){
            concurrent1 = time_left_partner_j;
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            //partner j could start any time during the year as long as all the duration fit in the same year.
            int latest_start_day = end_day_this_year - time_left_partner_j;
            int start_day = get_uniform(start_day_this_year, latest_start_day);
            int end_day = start_day + time_left_partner_j;
            relationships->set_partnerships_begin_end(j, start_day, end_day);
            p_do_not_change_dates.push_back(j);
            j_do_not_change = true;
          }
          else if (time_left_partner_longest < 365 && time_left_partner_j < 365){
            
            //if the sum of two <365 dur relationships is great that 365 days, they still would overlap in the year: min = the difference of durations left, max = the shorter duration left among the two
            if (time_left_partner_longest + time_left_partner_j > 365) {
              int min = int(abs(time_left_partner_longest + time_left_partner_j - 365));
              int max = std::min(time_left_partner_longest, time_left_partner_j);
              concurrent1 = get_uniform(min, max);
              int start_day_partner_j = today + time_left_partner_j - concurrent1;
              int end_day_partner_j = start_day_partner_j + time_left_partner_j;
              relationships->set_partnerships_begin_end(j, start_day_partner_j, end_day_partner_j);
            }
            
            //if the sume of two <365 dur relationships is less that 365 days, they MIGHT overlap in the year: min = 1, max = shorter duration among the two
            else {
              double rand2 = ((double) rand() / (RAND_MAX));
              if (rand2 <= prob_overlap) {    //overlap
                int min = 1;
                int max = std::min(time_left_partner_longest, time_left_partner_j);
                concurrent1 = get_uniform(min, max);
                
                //longest_index is fixed and partner j's duration should fit in this year, so:
                int start_day = relationships->get_partner_end_day(longest_index) - concurrent1;
                int end_day = start_day + time_left_partner_j;
                relationships->set_partnerships_begin_end(j, start_day, end_day);
                p_do_not_change_dates.push_back(j);
                j_do_not_change = true;
              }
              else {    //no overlap
                int min = 0;
                int max = 0;
                concurrent1 = 0;
                
                //partner j duration left should fit in this year
                int earliest_start_day_j = relationships->get_partner_end_day(longest_index) + 1;
                int latest_start_day_j = end_day_this_year - time_left_partner_j;
                int start_day_j = get_uniform(earliest_start_day_j, latest_start_day_j);
                int end_day_j = start_day_j + time_left_partner_j;
                relationships->set_partnerships_begin_end(j, start_day_j, end_day_j);
                p_do_not_change_dates.push_back(j);
                j_do_not_change = true;
              }
            }
            
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            
            if (person->get_id() == Global::TEST_ID){
              cout << "\nDEBUG1 Adjust concurrency. " << endl;
              relationships->print_relationship();
              cout << "\nlongest_index " << longest_index << "  concurrent1 " << concurrent1 << endl;
              
              for (int j = 0; j < num_matched_partners; j++) {
                //  cout << "Partner " << j << "\tDur total\t" << relationships->get_partner_dur(j) << "\tDur left\t" << relationships->get_time_left_in_relationship(j) << "\tStart day\t" << relationships->get_partner_start_day(j) << "\tEnd day\t" << relationships->get_partner_end_day(j) << endl;
                //getchar();  //for TEST_ID
              }
            }
            
          }
          
        }
        else if (!j_do_not_change && !longest_index_do_not_change) {  //neither are fixed. follow the usual process
          
          if (time_left_partner_longest >= 365 && time_left_partner_j >= 365){
            concurrent1 = 365;
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            relationships->set_partnerships_begin_end(longest_index, today , time_left_partner_longest);
            relationships->set_partnerships_begin_end(j, today, time_left_partner_j);
            
            p_do_not_change_dates.push_back(longest_index);
            longest_index_do_not_change = true;
            p_do_not_change_dates.push_back(j);
            j_do_not_change = true;
          }
          else if (time_left_partner_longest >= 365 && time_left_partner_j < 365){
            concurrent1 = time_left_partner_j;
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            relationships->set_partnerships_begin_end(longest_index, today, time_left_partner_longest);
            //partner j could start any time during the year as long as all the duration fit in the same year.
            int latest_start_day = end_day_this_year - time_left_partner_j;
            int start_day = get_uniform(start_day_this_year, latest_start_day);
            relationships->set_partnerships_begin_end(j, start_day, start_day + time_left_partner_j);
            
            p_do_not_change_dates.push_back(longest_index);
            longest_index_do_not_change = true;
            p_do_not_change_dates.push_back(j);
            j_do_not_change = true;
          }
          else if (time_left_partner_longest < 365 && time_left_partner_j < 365){
            
            //if the sum of two <365 dur relationships is great that 365 days, they still overlap in the year: min = the difference of durations left, max = the shorter duration left among the two
            if (time_left_partner_longest + time_left_partner_j > 365) {
              int min = int(abs(time_left_partner_longest + time_left_partner_j - 365));
              int max = std::min(time_left_partner_longest, time_left_partner_j);
              
              concurrent1 = get_uniform(min, max);
              //pick which relationships starts first
              double rand1 = ((double) rand() / (RAND_MAX));
              int first_partner = 0;
              int second_partner = 0;
              int time_left_partner_first = 0;
              int time_left_partner_second = 0;
              
              if (rand1 <= 0.5) {   //longest_index starts first
                first_partner = longest_index;
                second_partner = j;
                time_left_partner_first = time_left_partner_longest;
                time_left_partner_second = time_left_partner_j;
              }
              else {      //partner j starts first
                first_partner = j;
                second_partner = longest_index;
                time_left_partner_first = time_left_partner_j;
                time_left_partner_second = time_left_partner_longest;
              }
              
              //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
              int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
              int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
              relationships->set_partnerships_begin_end(first_partner, start_day_first, start_day_first + time_left_partner_first);
              int start_day_partner_second = start_day_first + time_left_partner_second - concurrent1;
              int end_day_partner_second = start_day_partner_second + time_left_partner_second;
              relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
            }
            
            //if the sume of two <365 dur relationships is less that 365 days, they MIGHT overlap in the year: min = 1, max = shorter duration among the two
            else {
              double rand2 = ((double) rand() / (RAND_MAX));
              if (rand2 <= prob_overlap) {    //overlap
                int min = 1;
                int max = std::min(time_left_partner_longest, time_left_partner_j);
                concurrent1 = get_uniform(min, max);
                
                //pick which relationships starts first
                double rand1 = ((double) rand() / (RAND_MAX));
                int first_partner = 0;
                int second_partner = 0;
                int time_left_partner_first = 0;
                int time_left_partner_second = 0;
                
                if (rand1 <= 0.5) {   //longest_index starts first
                  first_partner = longest_index;
                  second_partner = j;
                  time_left_partner_first = time_left_partner_longest;
                  time_left_partner_second = time_left_partner_j;
                }
                else {      //partner j starts first
                  first_partner = j;
                  second_partner = longest_index;
                  time_left_partner_first = time_left_partner_j;
                  time_left_partner_second = time_left_partner_longest;
                }
                
                //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
                int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
                int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
                relationships->set_partnerships_begin_end(first_partner, start_day_first, start_day_first + time_left_partner_first);
                int start_day_partner_second = start_day_first + time_left_partner_second - concurrent1;
                int end_day_partner_second = start_day_partner_second + time_left_partner_second;
                relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
                
              }
              else {    //no overlap
                int min = 0;
                int max = 0;
                concurrent1 = 0;
                
                //pick which relationships starts first
                double rand1 = ((double) rand() / (RAND_MAX));
                int first_partner = 0;
                int second_partner = 0;
                int time_left_partner_first = 0;
                int time_left_partner_second = 0;
                
                if (rand1 <= 0.5) {   //longest_index starts first
                  first_partner = longest_index;
                  second_partner = j;
                  time_left_partner_first = time_left_partner_longest;
                  time_left_partner_second = time_left_partner_j;
                }
                else {      //partner j starts first
                  first_partner = j;
                  second_partner = longest_index;
                  time_left_partner_first = time_left_partner_j;
                  time_left_partner_second = time_left_partner_longest;
                }
                
                //second_partner duration left should fit in this year, so: (latest_start_day for first + time_left_partner_second - min + time_left_partner_second <= end_day_this_year)
                int latest_start_day_first = end_day_this_year - time_left_partner_first + min - time_left_partner_second;
                int start_day_first = get_uniform(start_day_this_year, latest_start_day_first);
                int end_day_first = start_day_first + time_left_partner_first;
                
                relationships->set_partnerships_begin_end(first_partner, start_day_first, end_day_first);
                int latest_start_day_second = end_day_this_year - time_left_partner_second;
                
                int start_day_partner_second = get_uniform(end_day_first + 1, latest_start_day_second);
                int end_day_partner_second = start_day_partner_second + time_left_partner_second;
                relationships->set_partnerships_begin_end(second_partner, start_day_partner_second, end_day_partner_second);
              }
            }
            
            //mina change: calculate these, for partners who do not have longest partnership...
            relationships->set_concurrent_days_in_relationships(longest_index, concurrent1);
            relationships->set_concurrent_days_in_relationships(j, concurrent1);
            
            if (person->get_id() == Global::TEST_ID){
              cout << "\nDEBUG1 Adjust concurrency. " << endl;
              relationships->print_relationship();
              cout << "\nlongest_index " << longest_index << "  concurrent1 " << concurrent1 << endl;
              
              for (int j = 0; j < num_matched_partners; j++) {
                cout << "Partner " << j << "\tDur total\t" << relationships->get_partner_dur(j) << "\tDur left\t" << relationships->get_time_left_in_relationship(j) << "\tStart day\t" << relationships->get_partner_start_day(j) << "\tEnd day\t" << relationships->get_partner_end_day(j) << endl;
                //getchar();  //for TEST_ID
              }
            }
          }
        }
      }   //end of if (j != longest_index)
    } //end of for j - checking partners two by two
    
    //mina check: use later: int draw_concurrent_days = 30* int(get_lognormal2()); //convert to days
    //set up the dur left and start/end dates for this male's female partners:
    for (int j = 0; j < num_matched_partners; j++) {
      Person* partner_female = relationships->get_partner_from_list(j);
      Relationships* relationships_female = partner_female->get_relationships();
      int partners_female = relationships_female->get_partner_list_size();
      
      //QC check: female should have at least one partner:
      if (partners_female <= 0 || relationships_female->get_person_num_partners_label() <= 0 || partners_female > relationships_female->get_person_num_partners_label()) {
        cout << "ERROR: adjust_concurrency()- Female does not have matched male partner on list. Press key to continue." << endl;
        getchar();  //for error
      }
      
      for (int k = 0; k < partners_female; k++) {
        Person* partner_male = relationships_female->get_partner_from_list(k);
        
        if (person->get_id() == partner_male->get_id()) {
          //if person (male) is the same as the male in k'th place on female's partner list
          int concurrent_days = relationships->get_concurrent_days_in_relationships(j);
          relationships_female->set_concurrent_days_in_relationships(k, concurrent_days);
          relationships_female->set_time_left_in_relationship(k, relationships->get_partner_dur_left(j));
          int start = relationships->get_partner_start_day(j);
          int end = relationships->get_partner_end_day(j);
          relationships_female->set_partnerships_begin_end(k, start, end);
        }
      }
    } //end of setup for female partners of this male
    
    p_do_not_change_dates.clear();    //mina added new *******
    
  }  //end of for
  p_vector.clear();
  FRED_VERBOSE(1, "MINA Adjust Concurrency day %d finished.\n", today );
  
}   //end of adjust_concurrency

//*******************************************************
//*******************************************************

bool Sexual_Transmission_Network::check_partner_duplicate(Person* person1, Person* person2){
  // check that a male is not matched with a female more than once.
  // person1: male , person2: female
  bool duplicate = false;
  Relationships* relationships1 = person1->get_relationships();
  Relationships* relationships2 = person2->get_relationships();
  
  if (relationships1->get_partner_list_size()>0){
    for (int k=0; k < relationships1->get_partner_list_size(); k++) {
      int partner_id = relationships1->get_matched_partner_id(k);
      if (person2->get_id() == partner_id) duplicate=true;
      
    }
  }
  
  return duplicate;   // 0: not duplicated partner, 1: duplicated partner
  
} // end of check_partner_duplicate

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::calculate_dur_partnership_day0(Person* &individual, double dur[9][3]){
  
  /* duration in days. calculations are done in month, then duration is returned in days. new version with reduced number of categories:
   The column in matrix:
   up to 1 year, 1-12 months        0
   1 to 3 years, 13 to 36 months    1
   >36 months                       2
   */
  
  //set the duration of relationship for each partner. run for age group 0 only, on day 0
  Relationships* relationships = individual->get_relationships();
  int age_group = relationships->get_current_age_group();
  int age_at_first_marriage_category = -1;   //Categories: under 25= 0, 20-24=1, 25 and over =3 (based on CDC doc)
  int rand_longterm_partner = -1;   //should not be 0
  
  int total_matched_partners = relationships->get_partner_list_size();
  int partner_addition = 0;
  
  double rand2 = ((double) rand() / (RAND_MAX));
  double prob_first_marriage = get_prob_first_marriage(individual->get_age());
  bool long_term = false;
  bool already_has_long_term = false;
  vector<int> partner_addition_index ;  // vector of index for partners who need duration
  
  for (int j = 0; j < total_matched_partners; j++) {
    if (relationships->get_long_term_status(j) == true)
      already_has_long_term = true;
    if (relationships->get_partner_dur(j) == 0)
      
      partner_addition_index.push_back(j);
  }
  
  partner_addition = partner_addition_index.size() ;   //number of new partners who need duration
  
  //identify if person would have a long-term relationship based on probability of first marriage by age
  if (!already_has_long_term) {
    if (age_group==0 && rand2 <= 0.1 ) long_term = true; //the function get_prob does not work for age group 0
    else if (age_group==1 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==2 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==3 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==4 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==5 && rand2 <= prob_first_marriage)  long_term = true;
  }
  
  if (long_term == true){
    rand_longterm_partner = rand() % partner_addition;  //pick one partner at random for long-term relationship
    if (age_group==0) age_at_first_marriage_category = 0;
    //else if (age_group==1) age_at_first_marriage_category = 1;
    //else age_at_first_marriage_category = 2;
  }
  
  
  for (int i = 0; i < partner_addition; i++) {
    int partner_index = partner_addition_index.at(i);
    int duration = 0;
    if (i == rand_longterm_partner) {
      bool long_term_this_partner = true;
      
      relationships->set_long_term_status(partner_index);
      duration = set_dur_utility (age_group, long_term_this_partner, age_at_first_marriage_category, dur);
    }
    else {
      bool long_term_this_partner = false;
      duration = set_dur_utility (age_group, long_term_this_partner, age_at_first_marriage_category, dur);
    }
    
    relationships->set_partner_dur(partner_index, duration);
    int dur_left = duration;
    relationships->set_time_left_in_relationship(partner_index, dur_left);
  }
  
  int num_matched_partners = relationships->get_partner_list_size();
  
  //  if (individual->get_id() == Global::TEST_ID) {
  //    cout << "\n\ncalculate_dur_partnership_day0 for TEST_ID:" << endl;
  //    for (int j = 0; j < num_matched_partners; j++) {
  //      cout << "Partner " << j << "\tDur total\t" << relationships->get_partner_dur(j) << "\tDur left\t" << relationships->get_time_left_in_relationship(j) << "\tStart day\t" << relationships->get_partner_start_day(j) << "\tEnd day\t" << relationships->get_partner_end_day(j) << endl;
  //      //getchar();    //for TEST_ID
  //    }
  //  }
  
  partner_addition_index.clear();
  
} //end of calculate_dur_partnership_day0

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::calculate_dur_partnership(Person* &individual, double dur[9][3]){
  FRED_VERBOSE(1, "Day %d\tcalculate_dur_partnership started.\n", Global::Simulation_Day);
  /* duration in days. calculations are done in month, then duration is returned in days. new version with reduced number of categories:
   The column in matrix:
   up to 1 year, 1-12 months        0
   1 to 3 years, 13 to 36 months    1
   >36 months                       2
   */
  
  Relationships* relationships = individual->get_relationships();
  
  int total_matched_partners = relationships->get_partner_list_size();
  int age_group = relationships->get_current_age_group();
  int age_at_first_marriage_category = -1;   //Categories: under 25= 0, 20-24=1, 25 and over =3 (based on CDC doc)
  int rand_longterm_partner = -1;   //should not be 0
  int partner_addition = 0;
  
  double rand2 = ((double) rand() / (RAND_MAX));
  double prob_first_marriage = get_prob_first_marriage(individual->get_age());
  bool long_term = false;
  bool already_has_long_term = false;
  
  vector<int> partner_addition_index ;  // vector of index for partners who need duration
  
  for (int j = 0; j < total_matched_partners; j++) {
    if (relationships->get_long_term_status(j) == true)
      already_has_long_term = true;
    if (relationships->get_partner_dur(j) == 0)
      partner_addition_index.push_back(j);
  }
  partner_addition = partner_addition_index.size() ;   //number of new partners who need duration
  
  
  if (already_has_long_term == false) {
    if (age_group==0 && rand2 <= 0.1 ) long_term = true; //the function get_prob does not work for  age group 0
    else if (age_group==1 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==2 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==3 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==4 && rand2 <= prob_first_marriage)  long_term = true;
    else if (age_group==5 && rand2 <= prob_first_marriage)  long_term = true;
  }
  
  if (long_term == true){
    //randomly pick one partner for long-term relationship among partners who do not have a duration
    rand_longterm_partner = rand() % partner_addition;
    
    if (age_group==0) age_at_first_marriage_category = 0;
    else if (age_group==1) age_at_first_marriage_category = 1;
    else age_at_first_marriage_category = 2;
  }
  
  for (int k = 0; k < partner_addition; k++) {
    //cout << " k = " << k << endl;
    int partner_index = partner_addition_index.at(k);
    int duration = 0;
    
    if (k == rand_longterm_partner) {
      bool long_term_this_partner = true;
      relationships->set_long_term_status(partner_index);
      duration = set_dur_utility (age_group, long_term_this_partner, age_at_first_marriage_category, dur);
    }
    else {
      bool long_term_this_partner = false;
      duration = set_dur_utility (age_group, long_term_this_partner, age_at_first_marriage_category, dur);
    }
    
    relationships->set_partner_dur(partner_index, duration);
    int dur_left = duration;
    relationships->set_time_left_in_relationship(partner_index, dur_left);
  }
  
  partner_addition_index.clear();
  
  //QC checkpoint:   check if each MALE has only one long-term relationship + if the "matched FEMALE" is NULL ****** Important
  int count_long_term = 0;
  for (int i = 0; i < relationships->get_partner_list_size() ; i++) {
    //cout << "Person " << p << "  age group " << relationships->get_current_age_group() << " ID " << person->get_id() << "  partner " << i << "  dur is " << relationships->get_partner_dur(i) << endl;
    if (relationships->get_partner_from_list(i) == NULL){
      FRED_VERBOSE(0, "Day %d\tERROR in calculate_dur_partnership(). Matched partner Person* is NULL pointer. MALE ID = %d \n", Global::Simulation_Day, individual->get_id());
      getchar();  //for error
    }
    if (relationships->get_partner_dur(i) == 0) {   //matched partners without duration
      FRED_VERBOSE(0, "Day %d\tERROR in calculate_dur_partnership(). Matched partner duration is 0. MALE ID = %d \n", Global::Simulation_Day, individual->get_id());
      getchar();  //for error
    }
    if (relationships->get_long_term_status(i) == true)
      count_long_term++;
    
  }
  
  if (count_long_term > 1 ){
    FRED_VERBOSE(0, "Day %d\tERROR in calculate_dur_partnership(). Person has more than one long-term relationship. MALE ID = %d \n", Global::Simulation_Day, individual->get_id());
    getchar();  //for error
  }
  
  
  FRED_VERBOSE(1, "Day %d\tcalculate_dur_partnership finished.\n", Global::Simulation_Day);
  
} //end of calculate_dur_partnership

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::count_adult_females(){
  //people who are less than 15: age group is -1
  //num_partners is -1 in setup.
  //reset numbers
  for (int i = 0 ; i < ROW_age_group; i++) {
    female[i] = 0;
  }
  total_female_adult = 0;
  
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if(person->get_sex() == 'F' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() == 1){
      if (relationships->get_current_age_group() == 0)   female[0]++;
      else if (relationships->get_current_age_group() == 1)  female[1]++;
      else if (relationships->get_current_age_group() == 2)  female[2]++;
      else if (relationships->get_current_age_group() == 3)  female[3]++;
      else if (relationships->get_current_age_group() == 4)  female[4]++;
      else if (relationships->get_current_age_group() == 5)  female[5]++;
      else if (relationships->get_current_age_group() == 6)  female[6]++;
      else if (relationships->get_current_age_group() == 7)  female[7]++;

    }
  }
  
  for (int i = 0; i < ROW_age_group; i++) {
    Sexual_Transmission_Network::total_female_adult += Sexual_Transmission_Network::female[i];
  }
} //end of count_adult_females()

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::count_adult_males(){
  //people who are less than 15: age group is -1
  //num_partners is -1 in setup.
  //reset numbers
  for (int i = 0 ; i < ROW_age_group; i++) {
    male[i] = 0;
  }
  for (int i = 0 ; i < 5; i++) {
    male_agegroup0[i] = 0;
  }
  total_male_adult = 0;
  total_male_adult_agegroup0 = 0;
  
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if(person->get_sex() == 'M' && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() == 1 && relationships->get_person_num_partners_label() >= 0){
      if (relationships->get_current_age_group() == 0)   male[0]++;
      else if (relationships->get_current_age_group() == 1)  male[1]++;
      else if (relationships->get_current_age_group() == 2)  male[2]++;
      else if (relationships->get_current_age_group() == 3)  male[3]++;
      else if (relationships->get_current_age_group() == 4)  male[4]++;
      else if (relationships->get_current_age_group() == 5)  male[5]++;
      else if (relationships->get_current_age_group() == 6)  male[6]++;
      else if (relationships->get_current_age_group() == 7)  male[7]++;
      
      if (person->get_age() == 15)   male_agegroup0[0]++;
      else if (person->get_age() == 16)  male_agegroup0[1]++;
      else if (person->get_age() == 17)  male_agegroup0[2]++;
      else if (person->get_age() == 18)  male_agegroup0[3]++;
      else if (person->get_age() == 19)  male_agegroup0[4]++;
    }
  } //end of for all people
  
  for (int i = 0; i < ROW_age_group; i++) {
    Sexual_Transmission_Network::total_male_adult += Sexual_Transmission_Network::male[i];
  }
  
  Sexual_Transmission_Network::total_adult = Sexual_Transmission_Network::total_female_adult + Sexual_Transmission_Network::total_male_adult ;
  
  for (int i = 0; i < 5; i++) {
    total_male_adult_agegroup0 += male_agegroup0[i];
  }
  
} //end of count_adult_males()

//*******************************************************
//*******************************************************

int Sexual_Transmission_Network::get_uniform(int i, int j) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(i, j);
  return dis(gen);
}

double Sexual_Transmission_Network::get_normal(double mean, double sd) {
  std::random_device rd;
  std::normal_distribution<> dis(mean, sd);
  return dis(rd);
}

double Sexual_Transmission_Network::get_lognormal2() {
  double mean = 1.6;
  double sd = 1.767;
  
  std::random_device rd;
  std::lognormal_distribution<> dis(mean, sd);
  return dis(rd);
}

int Sexual_Transmission_Network::get_lognormal1(double cdf[46], double rand3) {
  int num_p = -1;
  int i;
  
  if(rand3 < cdf[0]) num_p =  4;
  else if(rand3 >= cdf[46]) num_p = 46 + 5;
  else{
    for (i=1; i<46; i++) {
      if(rand3 >= cdf[i-1] && rand3 < cdf[i]) num_p = i + 4;
    }
  }
  return num_p;
}

//*******************************************************
//*******************************************************

double Sexual_Transmission_Network::get_prob_first_marriage(double age){
  //works for ages 20 and over only
  //age should be in years to use the distribution: person's age is in years already
  double prob;
  prob = 1- (3.357 * exp(-0.07*(age))) ;   //based on trendline calculated in excel-survival analysis cdf F(t)
  return prob;
}

//*******************************************************
//*******************************************************

int Sexual_Transmission_Network::get_marriage_dur(double random_number, int age_at_first_marriage_category ){
  
  //added +3 because we count long term relationship as 3+ years
  
  double duration;      //calculated in years. we need to change it to months
  if (age_at_first_marriage_category == 0) {
    duration = (-log(random_number)/0.035) + log(0.793) ;      //based on trendline calculated in excel
  }
  else if (age_at_first_marriage_category == 1) {
    duration = (-log(random_number)/0.027) + log(0.9222) ;
  }
  else if (age_at_first_marriage_category == 2) {
    duration = (-log(random_number)/0.021) + log(0.9353) ;
  }
  //cout << "Duration (in years): " << duration << endl;
  return round(12.0* (duration+3));      //return value in months
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::matching_process(){
  FRED_VERBOSE(1, "MINA matching_process() started.\n" );
  
  //prepare a vector of people in sexual network who are eligible to have partners
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if (relationships->get_person_num_partners_label() == 1 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true && relationships->get_partner_list_size() < relationships->get_person_num_partners_label()){
      p_vector.push_back(person);
    }
  }
  
  prepare_matching_vectors(1, p_vector);  //type 1: group men and women who are monogamous, and categorize them by age group
  
  tStart = clock();
  
  match_monogamous();
  
  printf ("match_monogamous. Timer = %f seconds.\n", ((float)(clock()-tStart)/CLOCKS_PER_SEC)); fflush(stdout);

  
  
  FRED_VERBOSE(1, "match_males_rest() started.\n");
  
  //prepare a vector of people in sexual network who are eligible to have partners
  p_vector.clear();
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if (relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true && relationships->get_partner_list_size() < relationships->get_person_num_partners_label()){
      p_vector.push_back(person);
    }
  }
  
  //DEBUG: check if there are monogamous males o females who are not matched yet.
  //fprintf (report2, "\ncheck who is not matched monogamous yet:" );
  //prepare_matching_vectors(1, p_vector);
  
  
  prepare_matching_vectors(2, p_vector);
  
  //  tStart = clock();
  
  //match_males method takes all males and females to be matched. pick randomly from males, and match him with a female based on age-mixing
  match_males(males_to_match, females_to_match_byAge, m_mixing);
  
 // printf ("match_males(). Timer = %f seconds.\n", ((float)(clock()-tStart)/CLOCKS_PER_SEC)); fflush(stdout);
  //check who is left
  //fprintf (report2, "\nCheck males and females who are not matched yet, AFTER match_males_rest():\n" );
  //prepare_matching_vectors(2, p_vector);
  
  p_vector.clear();
  
  FRED_VERBOSE(1, "match_males_rest finished.\n" );
  
} // end of matching_process

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::match_monogamous(){
  
  FRED_VERBOSE(1, "MINA match_monogamous() started.\n" );
  
  //  cout << "\n************family household types matching " << endl;
  //  printf ( "\nmales_to_match_1partner_FH %lu \n", males_to_match_1partner_FH.size());
  //  printf ( "\nmales_to_match_1partner_nonFH %lu \n", males_to_match_1partner_nonFH.size());
  
  //*************** family household types matching. household type: 2, 4, 7
  for (int i = 0; i < males_to_match_1partner_FH.size(); i++) {
    
    Person* person1 = males_to_match_1partner_FH.at(i);
    Relationships* relationships1 = person1->get_relationships();
    Household* household1 = person1->get_permanent_household();
    vector<Person*> inhabitants = household1->get_inhabitants();
    
    if (person1->get_id() == Global::TEST_ID) {
      cout << "\nBefore matching i = " << i << "\t person1 id " << person1->get_id() << "\t htype " << relationships1->get_htype() <<  "\t h size " << inhabitants.size() << "\t person1 age " << person1->get_age() << "\t matched partners " << relationships1->get_partner_list_size() << endl;
      getchar();
      
    }
    
    bool ok_mixing = false;
    
    for (int k = 0; k < inhabitants.size(); k++) {
      Person* person2 = inhabitants.at(k);
      Relationships* relationships2 = person2->get_relationships();
      
      if (person1->get_id() != person2->get_id() && person2->get_sex()=='F' && relationships2->get_person_num_partners_label() == 1 && relationships1->get_partner_list_size() < relationships1->get_person_num_partners_label() && relationships2->get_partner_list_size() < relationships2->get_person_num_partners_label()) {
        
        ok_mixing = age_mixing(person1->get_age(), person2->get_age(), m_mixing);

        if (ok_mixing) {
          
          if (person1->get_id() == Global::TEST_ID) {
            cout << endl;
            cout << "\tk = " << k << "\t person2 id " << person2->get_id() << "\t htype " << relationships2->get_htype() <<  "\t h size " << inhabitants.size() << "\t person2 age " << person2->get_age() << "\t matched partners " << relationships2->get_partner_list_size() << endl;
            //   getchar(); //for TEST_ID
          }
          
          //****************** for male
          relationships1->add_to_partner_list(person2);  //for male
          relationships1->increase_partner_in_age_group(relationships1->get_current_age_group(), 1);
          
          if (Global::Simulation_Day == 0){
            if (relationships1->get_current_age_group() == 0) {
              calculate_dur_partnership_day0(person1, m_dur);
            }
            else if (relationships1->get_current_age_group() >= 1 && relationships1->get_current_age_group() < 6) {
              int duration = relationships1->get_dur_day0();
              int duration_left = relationships1->get_dur_left_day0();
              relationships1->set_partner_dur(0, duration);
              relationships1->set_time_left_in_relationship(0, duration_left);
            }
          }
          else {
            calculate_dur_partnership(person1, m_dur);
          }
          
          //****************** for female
          relationships2->add_to_partner_list(person1);   //for female
          relationships2->increase_partner_in_age_group(relationships2->get_current_age_group(), 1);
          relationships2->set_partner_dur(0, relationships1->get_partner_dur(0)); //this is 1 partner matching: so take the only duration on the list of male partner and add it to female's.
          relationships2->set_time_left_in_relationship(0, relationships1->get_time_left_in_relationship(0));
          
          //****************** create network links:
          if(person1->is_connected_to(person2, this) == false) {    //link FROM male TO female
            person1->create_network_link_to(person2, this);
          }
          if(person2->is_connected_from(person1, this) == false) {  //link TO female FROM male
            person2->create_network_link_from(person1, this);
          }
          
          //debug
          for (int n = 0; n < relationships1->get_partner_list_size(); n++) {
            if (relationships1->get_partner_dur(n) < relationships1->get_time_past_in_relationship(n)) {
              cout << "ERROR: partner_dur < time past relationship for Male. Press key to continue." << endl;
              getchar();  //for error
            }
          }
          for (int n = 0; n < relationships2->get_partner_list_size(); n++) {
            if (relationships2->get_partner_dur(n) < relationships2->get_time_past_in_relationship(n)) {
              cout << "ERROR: partner_dur < time past relationship for Female. Press key to continue." << endl;
              getchar();  //for error
            }
          }
          if (person1->get_id() == Global::TEST_ID) {
            cout << "Male ID1 " << person1->get_id() << "  Female ID2 " << person2->get_id() << "  is connected to? " << person1->is_connected_to(person2, this) << endl;
            cout << "Female ID2 " << person2->get_id() << "  Male ID1 " << person1->get_id() << "  is connected from? " << person2->is_connected_from(person1, this) << endl;
          }
          
          //  if (Global::Simulation_Day == 1) cout << "MATCHED \n" << endl;
          break;    // ***************
          
          
        } //end of if (ok_mixing)
      }
    } //end of all inhabitats in Person1's household
    
    if (person1->get_id() == Global::TEST_ID) {
      cout << "\nAfter matching i = " << i << "\t person1 id " << person1->get_id() << "\t htype " << relationships1->get_htype() <<  "\t h size " << inhabitants.size() << "\t person1 age " << person1->get_age() << "\t matched partners " << relationships1->get_partner_list_size() << endl;
    }
  } // family HH type
  
  
  // cout << "\n\n\n************ family household types matching finsihed.\n\n" << endl;
  // cout << "\n************ non-family household types matching " << endl;
  
  //*************** non-family household types matching
  if  (males_to_match_1partner_nonFH.size() != 0 && females_to_match_1partner_nonFH.size() != 0 )
    match_males(males_to_match_1partner_nonFH, females_to_match_1partner_byAge_nonFH, m_mixing);
  
  //cout << "\n************ non-family household types matching finished. " << endl;
  
  //DEBUG: check in-degrees and out-degrees for males and females who are matched. in-degree/out-degree should equal the number of matched partner = 1 (monogamous here)
  //  for (int i = 0; i < p_vector.size(); i++) {
  //    Person* person = p_vector.at(i);
  //    Relationships* relationships = person->get_relationships();
  //
  //    if (person->get_sex() == 'M' && relationships->get_person_num_partners_label() == 1) {
  //      int num_matched_partners = relationships->get_partner_list_size();
  //      int out_degree = person->get_out_degree(this);
  //      int partner_list_size = relationships->get_partner_list_size();
  //
  //      if (num_matched_partners != out_degree && relationships->get_partner_from_list(0) != NULL) {
  //        cout << "ERROR: End of match_monogamous(). out_degree is not qual to num_matched_partners for male. Press key to continue.\n" << endl;
  //        cout << "out_degree " << out_degree << "  num_matched_partners " << num_matched_partners << endl;
  //        relationships->print_relationship();
  //        getchar();  //for error
  //      }
  //      else {
  //        for (int j = 0; j < out_degree; j++) {
  //          Person* partner = person->get_end_of_link(j,this);
  //          Relationships* partner_relationships = partner->get_relationships();
  //          int partner_num_matched_partners = partner_relationships->get_partner_list_size();   //number of female's matched partner
  //          int in_degree = partner->get_in_degree(this);
  //          int partner_partner_list_size = partner_relationships->get_partner_list_size();
  //
  //          if (partner->get_sex() != 'F') {
  //            cout << "ERROR: End of match_monogamous(). matched partner of the male is not female. Press key to continue.\n" << endl;
  //            getchar();  //for error
  //          }
  //          if (partner_relationships->get_person_num_partners_label() == 1 && partner_num_matched_partners != in_degree && partner_relationships->get_partner_from_list(0) != NULL) {
  //            cout << "ERROR: End of match_monogamous(). in_degree is not qual to num_matched_partners for female. Press key to continue.\n" << endl;
  //            partner_relationships->print_relationship();
  //            getchar();  //for error
  //          }
  //          if (partner_relationships->get_person_num_partners_label() == 1 && partner_num_matched_partners != partner_partner_list_size && partner_relationships->get_partner_from_list(0) != NULL) {
  //            cout << "ERROR: End of match_monogamous(). partner_partner_list_size is not qual to num_matched_partners for female. Press key to continue.\n" << endl;
  //            partner_relationships->print_relationship();
  //            getchar();  //for error
  //          }
  //        }
  //      }
  //    } //end of if male
  //
  //
  //
  //  } //end of for
  
  
  FRED_VERBOSE(1, "match_monogamous() finished.\n");
  
} //end of match_monogamous

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::match_males(vector<Person*> &male_vector, vector<Person*> (&females_vec_byAge)[3], double m_mixing[3][3]){
  FRED_VERBOSE(1, "MINA match_males() started.\n" );
  // k=0 : age group of males, 15-19
  // k=1 : age groupd of males, 20-29
  // k=2 : age group of males, over 30
  
  int chances = 20;
  
  //*******************************
  //*********  OPEN MP test********
  //  #pragma omp parallel
  //  {
  //    cout << "Hello World" << endl;
  //  }
  //  
  //*******************************
  //*******************************

  
  while (male_vector.size() != 0 ) {
    
    int k=-1;
    int num_tries = 0;    //mina: I decide to try to find a match for a male 2000 (=chances) times, if not, I move on to the next person
    int rand_num0 = rand() % male_vector.size() ;    //randomly pick a male
    Person* person1 = male_vector.at(rand_num0);     //get male
    Relationships* relationships1 = person1->get_relationships();
    
    if (person1->get_id() == Global::TEST_ID) {
      cout << "\n\nDEBUG0000 before while loop for matching" << endl;
      relationships1->print_relationship();
    }
    
    //set age group
    if (person1->get_age() >=15 && person1->get_age() <20 ) k=0;
    else if (person1->get_age() >=20 && person1->get_age() <30 ) k=1;
    else if (person1->get_age() >=30 && person1->get_age() <45) k=2;
    
    
    while (relationships1->get_partner_list_size() < relationships1->get_person_num_partners_label() && num_tries < chances) {
      
      int rand_num1 = -1;
      double rand1 = ((double) rand() / (RAND_MAX));
      int matching_age_group = 0;
      
      if (k != -1) {    //exclude age group 6 for now
        if (rand1 < m_mixing[k][0])
          matching_age_group = 0;
        else if (rand1 >=m_mixing[k][0] && rand1 < m_mixing[k][1])
          matching_age_group = 1;
        else if (rand1 >=m_mixing[k][1])
          matching_age_group = 2;
      }
      
      
      if (females_vec_byAge[matching_age_group].size() != 0) {   //match to female of certain age group
        rand_num1 = rand() % females_vec_byAge[matching_age_group].size() ;       //randomly pick a female
        Person* person2 = females_vec_byAge[matching_age_group].at(rand_num1);    //get female
        Relationships* relationships2 = person2->get_relationships();
        
        if (person1->get_id() == Global::TEST_ID) {
          cout << "\n\nDEBUG1111 matching_age_group " << matching_age_group << endl;
          relationships1->print_relationship();
          relationships2->print_relationship();
        }
        
        if (check_partner_duplicate(person1, person2) == false && relationships2->get_partner_list_size() < relationships2->get_person_num_partners_label()) {
          
          
          if (person1->get_id() == Global::TEST_ID) {
            cout << "DEBUG2222 before matching " << endl;
            relationships1->print_relationship();
            relationships2->print_relationship();
          }
          
          
          //****************** for male
          relationships1->add_to_partner_list(person2);
          relationships1->increase_partner_in_age_group(relationships1->get_current_age_group(), 1);
          int p_position = relationships1->get_partner_list_size() - 1;   //index number on list
          
          if (Global::Simulation_Day == 0){
            if (relationships1->get_current_age_group() == 0) {
              calculate_dur_partnership_day0(person1, m_dur);
            }
            else if (relationships1->get_current_age_group() >= 1 && relationships1->get_current_age_group() < 6) {
              int duration = relationships1->get_dur_day0();
              int duration_left = relationships1->get_dur_left_day0();
              relationships1->set_partner_dur(p_position, duration);
              relationships1->set_time_left_in_relationship(p_position, duration_left);
            }
          }
          else {
            calculate_dur_partnership(person1, m_dur);
          }
          
          //****************** for female
          relationships2->add_to_partner_list(person1);
          relationships2->increase_partner_in_age_group(relationships2->get_current_age_group(), 1);
          int p_p_position = relationships2->get_partner_list_size() - 1;  //index number on list
          relationships2->set_partner_dur(p_p_position, relationships1->get_partner_dur(p_position));
          relationships2->set_time_left_in_relationship(p_p_position, relationships1->get_time_left_in_relationship(p_position));
          
          //create network links:
          if(person1->is_connected_to(person2, this) == false) {    //For male: link TO female
            person1->create_network_link_to(person2, this);
          }
          if(person2->is_connected_from(person1, this) == false) {  //For female: link FROM male
            person2->create_network_link_from(person1, this);
          }
        }
        
        if (person1->get_id() == Global::TEST_ID) {
          cout << "DEBUG3333 after matching" << endl;
          relationships1->print_relationship();
          relationships2->print_relationship();
          //getchar(); //for TEST_ID
        }
        
        if (relationships2->get_partner_list_size() == relationships2->get_person_num_partners_label()){
          remove_person(females_vec_byAge[matching_age_group], rand_num1);
        }
      }
      
      //else if (females_vec_byAge[matching_age_group].size() == 0){
      //cout << "females_vec_byAge[matching_age_group].size() == 0" << endl;
      //}
      
      num_tries++;
      if (num_tries == chances)
        //cout << "ERROR: cannot find partner for individual after " << num_tries << " tries." << endl;
      
      if (females_vec_byAge[0].size()==0 && females_vec_byAge[1].size()==0 && females_vec_byAge[2].size()==0)
        break;
      
    } //end of while for each male (person1)
    
    if (person1->get_id() == Global::TEST_ID) {
      cout << "DEBUG4444 after while loop. num_tries " << num_tries << endl;
      cout << "current age group:  " <<relationships1->get_current_age_group() << endl;
      relationships1->print_relationship();
      cout << "match_males()- partner in age group 0 = " << relationships1->get_partner_in_age_group(0);
      //getchar();  //for TEST_ID
    }
    
    remove_person(male_vector, rand_num0);
    
    if (females_vec_byAge[0].size()==0 && females_vec_byAge[1].size()==0 && females_vec_byAge[2].size()==0)
      break;
    
  } //end of while (male_vector.size() != 0 )
  
  FRED_VERBOSE(1, "MINA match_males() finished.\n" );
  
} //end of match_males

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::prepare_matching_vectors(int type, vector<Person*> &p_vector){
  FRED_VERBOSE(1, "MINA prepare_matching_vectors() started.\n");
  //type 1: group men and women who are monogamous, and categorize them by age group
  //type 2: group all men. separately gorup all women labeled 1, 2, 3 or more, based on age group. type 2 used in match_males_rest()
  
  if (type == 1) {
    
    reset_1partner_vectors();
    
    for (int i=0; i< p_vector.size(); i++) {
      
      Person* person = p_vector.at(i);
      Relationships* relationships = person->get_relationships();
      Household* household = person->get_permanent_household();
      vector<Person*> inhabitants = household->get_inhabitants();
      
      int age_group = relationships->get_current_age_group();
      double rand1 = ((double) rand() / (RAND_MAX));    //assumption: 95% of monogamous non-family males will match with monogamous non-family females.
      
      if (relationships->get_htype() == 2 || relationships->get_htype() == 4 || relationships->get_htype() == 7) {
        
        if (person->get_sex() == 'M' && inhabitants.size() > 1 ) {
          
          if (age_group >= 0 && age_group < 6) males_to_match_1partner_FH.push_back(person);
          
          if (person->get_age() >= 15 && person->get_age() < 20)
            males_to_match_1partner_byAge_FH[0].push_back(person);
          else if (person->get_age() >= 20 && person->get_age() < 30)
            males_to_match_1partner_byAge_FH[1].push_back(person);
          else if (person->get_age() >= 30 && person->get_age() < 45)
            males_to_match_1partner_byAge_FH[2].push_back(person);
        }
        
        else if (person->get_sex() == 'F' && inhabitants.size() > 1 ) {
          
          if (age_group >= 0 && age_group < 6) females_to_match_1partner_FH.push_back(person);
          
          if (person->get_age() >= 15 && person->get_age() < 20)
            females_to_match_1partner_byAge_FH[0].push_back(person);
          else if (person->get_age() >= 20 && person->get_age() < 30)
            females_to_match_1partner_byAge_FH[1].push_back(person);
          else if (person->get_age() >= 30 && person->get_age() < 45)
            females_to_match_1partner_byAge_FH[2].push_back(person);
        }
      }
      
      else if (relationships->get_htype() !=2 && relationships->get_htype() !=4 && relationships->get_htype() !=7 && rand1 <= 0.95) {
        
        if (person->get_sex() == 'M') {
          
          if (age_group >= 0 && age_group < 6) males_to_match_1partner_nonFH.push_back(person);
          
          if (person->get_age() >= 15 && person->get_age() <20)
            males_to_match_1partner_byAge_nonFH[0].push_back(person);
          else if (person->get_age() >= 20 && person->get_age() <30)
            males_to_match_1partner_byAge_nonFH[1].push_back(person);
          else if (person->get_age() >= 30)
            males_to_match_1partner_byAge_nonFH[2].push_back(person);
        }
        else if (person->get_sex() == 'F') {
          
          if (age_group >= 0 && age_group < 6) females_to_match_1partner_nonFH.push_back(person);
          
          if (person->get_age() >= 15 && person->get_age() <20)
            females_to_match_1partner_byAge_nonFH[0].push_back(person);
          else if (person->get_age() >= 20 && person->get_age() <30)
            females_to_match_1partner_byAge_nonFH[1].push_back(person);
          else if (person->get_age() >= 30)
            females_to_match_1partner_byAge_nonFH[2].push_back(person);
        }
      }
      
    } //end of for
    
    fprintf (report2, "\n\nsize of matching vectors 1 partner - Family Household types:\n" );
    fprintf (report2, "Age\tmales\tfemales\n" );
    fprintf (report2, "15-19\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[0].size(), females_to_match_1partner_byAge_FH[0].size());
    fprintf (report2, "29-29\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[1].size(), females_to_match_1partner_byAge_FH[1].size());
    fprintf (report2, "Over 30\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[2].size(), females_to_match_1partner_byAge_FH[2].size());
    
    //    printf ( "\n\n");
    //    printf ( "\n\nsize of matching vectors 1 partner - Family Household types:\n" );
    //    printf ( "Age\tmales\tfemales\n" );
    //    printf ( "15-19\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[0].size(), females_to_match_1partner_byAge_FH[0].size());
    //    printf ( "29-29\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[1].size(), females_to_match_1partner_byAge_FH[1].size());
    //    printf ( "Over 30\t%lu\t%lu\n", males_to_match_1partner_byAge_FH[2].size(), females_to_match_1partner_byAge_FH[2].size());
    //    printf ( "\n\n");
    
    fprintf (report2, "\n\nsize of matching vectors 1 partner - Non-Family Household types:\n" );
    fprintf (report2, "Age\tmales\tfemales\n" );
    fprintf (report2, "15-19\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[0].size(), females_to_match_1partner_byAge_nonFH[0].size());
    fprintf (report2, "29-29\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[1].size(), females_to_match_1partner_byAge_nonFH[1].size());
    fprintf (report2, "Over 30\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[2].size(), females_to_match_1partner_byAge_nonFH[2].size());
    
    //    printf ( "\n\n");
    //    printf ( "\n\nsize of matching vectors 1 partner - Family Household types:\n" );
    //    printf ( "Age\tmales\tfemales\n" );
    //    printf ( "15-19\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[0].size(), females_to_match_1partner_byAge_nonFH[0].size());
    //    printf ( "29-29\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[1].size(), females_to_match_1partner_byAge_nonFH[1].size());
    //    printf ( "Over 30\t%lu\t%lu\n", males_to_match_1partner_byAge_nonFH[2].size(), females_to_match_1partner_byAge_nonFH[2].size());
    //    printf ( "\n\n");
    
   
  } //end of type==1
  
  else if (type == 2){
    
    reset_matching_vectors();
    
    for (int i=0; i< p_vector.size(); i++) {
      Person* person = p_vector.at(i);
      Relationships* relationships = person->get_relationships();
      int age_group = relationships->get_current_age_group();
      
      
      if (person->get_sex() == 'M') {
        
        if (age_group >= 0 && age_group < 6) males_to_match.push_back(person);
        
        if (person->get_age() >= 15 && person->get_age() <20)
          males_to_match_byAge[0].push_back(person);
        else if (person->get_age() >= 20 && person->get_age() <30)
          males_to_match_byAge[1].push_back(person);
        else if (person->get_age() >= 30)
          males_to_match_byAge[2].push_back(person);
      }
      else if (person->get_sex() == 'F') {
        int repeat = relationships->get_person_num_partners_label() - relationships->get_partner_list_size();

        if (age_group >= 0 && age_group < 6) females_to_match.push_back(person);
        
        if (person->get_age() >= 15 && person->get_age() <20){
          for (int j= 0; j < repeat; j++) {
            females_to_match_byAge[0].push_back(person);
          }
        }
        else if (person->get_age() >= 20 && person->get_age() <30){
          for (int j= 0; j < repeat; j++) {
            females_to_match_byAge[1].push_back(person);
          }
        }
        else if (person->get_age() >= 30){
          for (int j= 0; j < repeat; j++) {
            females_to_match_byAge[2].push_back(person);
          }
        }
      }
    }
    
    //fprintf (report2, "Age\tmales\tfemales\n" );
    //fprintf (report2, "15-19\t%lu\t%lu\n", males_to_match_byAge[0].size(), females_to_match_byAge[0].size());
    //fprintf (report2, "29-29\t%lu\t%lu\n", males_to_match_byAge[1].size(), females_to_match_byAge[1].size());
    //fprintf (report2, "Over 30\t%lu\t%lu\n", males_to_match_byAge[2].size(), females_to_match_byAge[2].size());
    
  } //end of type==2
  
  FRED_VERBOSE(1, "MINA prepare_matching_vectors() finished.\n");
}  //end of prepare_matching_vectors

//*******************************************************
//*******************************************************

// report number of labeled
void Sexual_Transmission_Network::prepare_report_labeled(){
  
  int labeled_people_15[4]={0};
  int labeled_people_16[4]={0};
  int labeled_people_17[4]={0};
  int labeled_people_18[4]={0};
  int labeled_people_19[4]={0};
  
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int age_group = relationships->get_current_age_group();
    int num_partner = relationships->get_person_num_partners_label();
    
    if (relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true ) {
      for (int group = 0; group < ROW_age_group; group++) {   // now include older age groups.
        for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
          if (group == age_group && num_partner == partner_cat) {
            if (person->get_sex() == 'M') {
              male_labeled_partner[age_group][partner_cat]++;
              
              if (person->get_age() == 15)
                labeled_people_15[partner_cat]++;
              else if (person->get_age() == 16)
                labeled_people_16[partner_cat]++;
              else if (person->get_age() == 17)
                labeled_people_17[partner_cat]++;
              else if (person->get_age() == 18)
                labeled_people_18[partner_cat]++;
              else if (person->get_age() == 19)
                labeled_people_19[partner_cat]++;
            }
            else if (person->get_sex() == 'F')
              female_labeled_partner[age_group][partner_cat]++;
          }
        }
      }
    }
  }
  
  // for males
  fprintf (report2, "Assigned labels for males.\n");
  fprintf (report2, "***** Year %d\n", Global::Simulation_Day/365);
  fprintf (report2, "Age group\t0\t1\t2\t3+\n");
  for (int group = 0; group < ROW_age_group; group++) {
    fprintf (report2, "%d\t", group);
    for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
      fprintf (report2, "%d\t" , male_labeled_partner[group][partner_cat]);
    }
    fprintf (report2, "\n");
  }
  fprintf (report2, "\n");
  
  
  // for females
  fprintf (report2, "Assigned labels for females.\n");
  fprintf (report2, "***** Year %d\n", Global::Simulation_Day/365);
  fprintf (report2, "Age group\t0\t1\t2\t3+\n");
  for (int group = 0; group < ROW_age_group; group++) {
    fprintf (report2, "%d\t", group);
    for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
      fprintf (report2, "%d\t" , female_labeled_partner[group][partner_cat]);
    }
    fprintf (report2, "\n");
  }
  fprintf (report2, "\n");
  
  // for males age group 0 (15 to 19)
  fprintf (report2, "Assigned labels for males age group 0 males.\n");
  fprintf (report2, "***** Year %d\n", Global::Simulation_Day/365);
  fprintf (report2, "Age\t0\t1\t2\t3+\n");
  fprintf (report2, "15\t");
  for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
    fprintf (report2, "%d\t" , labeled_people_15[partner_cat]);
  }
  fprintf (report2, "\n");
  fprintf (report2, "16\t");
  for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
    fprintf (report2, "%d\t" , labeled_people_16[partner_cat]);
  }
  fprintf (report2, "\n");
  fprintf (report2, "17\t");
  for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
    fprintf (report2, "%d\t" , labeled_people_17[partner_cat]);
  }
  fprintf (report2, "\n");
  fprintf (report2, "18\t");
  for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
    fprintf (report2, "%d\t" , labeled_people_18[partner_cat]);
  }
  fprintf (report2, "\n");
  fprintf (report2, "19\t");
  for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
    fprintf (report2, "%d\t" , labeled_people_19[partner_cat]);
  }
  fprintf (report2, "\n");
  
  
} //end of prepare_report_labeled

//*******************************************************
//*******************************************************
// report number of matched
void Sexual_Transmission_Network::prepare_report_matched(){
  
  //*******************************************************
  //report matched individuals in a text file. Format: each line has 2 IDs. First the male's and second the female's.
  /*
   for (int i=0; i< person_vec.size(); i++) {
   Person* temp_person = person_vec.at(i);
   Relationships* relationships = temp_person->get_relationships();
   //if (temp_person->get_count_matched_partners()>0) cout << "count_match_partner worked" << endl;
   if (temp_person->get_sex()=='M' && relationships->check_relationships_eligiblity() && relationships->get_partner_list_size()>0) {
   fprintf(report_partners_output, "%d\t" , temp_person->get_id());
   for (int j=0; j< relationships->get_partner_list_size(); j++) {
   fprintf(report_partners_output, "%d\t" , relationships->get_matched_partner_id(j));   //change
   }
   fprintf(report_partners_output, "\n");
   }
   }
   */
  
  //  *******************************************************
  //  report number of people matched by sex and age
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* temp_person = Global::Pop.get_person(p);
    Relationships* relationships = temp_person->get_relationships();
    int age_group = relationships->get_current_age_group();
    int age = temp_person->get_age();
    
    
    //mina check: add the reset for male_partner_inyear_matched array
    
    if (temp_person->get_sex()=='M' && relationships->check_relationships_eligiblity() && temp_person->is_enrolled_in_network(this) == true ) {
      
      if (relationships->get_partner_list_size()==0) male_partner_inyear_matched[age_group][0]++;
      else if (relationships->get_partner_list_size()==1) male_partner_inyear_matched[age_group][1]++;
      else if (relationships->get_partner_list_size()==2) male_partner_inyear_matched[age_group][2]++;
      else if (relationships->get_partner_list_size()>=3) {
        male_partner_inyear_matched[age_group][3]++;
        if (relationships->get_partner_list_size()>=3 && relationships->get_partner_list_size()<7)
          male_partner_inyear_matched[age_group][4]++;
        else if (relationships->get_partner_list_size()>=7 && relationships->get_partner_list_size()<15)
          male_partner_inyear_matched[age_group][5]++;
        else if (relationships->get_partner_list_size()>=15)
          male_partner_inyear_matched[age_group][6]++;
      }
    }
  }
  
  
  for (int age_group = 0; age_group < ROW_ever1; age_group++) {   //5 age groups   //mina check : report all age groups/
    for (int i = 0; i < COL_partner_categories_extended; i++) {  //6 partner categories
      male_partner_matched_total[age_group] += male_partner_inyear_matched[age_group][i];
    }
  }
  
  fprintf (report_matched, "Number of matched partners for males, based on age.\n");
  fprintf (report2, "***** Year %d\n", Global::Simulation_Day/365);
  fprintf (report_matched, "Age Group\t0\t1\t2\t3 or more\t3 to 6\t7 to 14\t15 or more");
  
  for (int age_group = 0; age_group < ROW_ever1; age_group++) {
    fprintf (report_matched, "\n%d", age_group);
    for (int i = 0; i < COL_partner_categories_extended; i++) {
      fprintf (report_matched, "\t%d", male_partner_inyear_matched[age_group][i]);
    }
  }
  
}   //end of prepare_report_matched

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::prepare_report4(int year){
  
  // partner ever report: runs on day 0 (initialization) and every year (every 365 days)
  // ROW_ever1 = 6 age categories, because I do not want to count the over 45 age gorup
  // before reseting, I wanna know how many males I have in each category:
  int partner_cat = 0;
  int age_group = 0;
  
  * xls_output << "Year" << '\t' << year << '\t' << "Day" << '\t' << Global::Simulation_Day <<'\n';
  
  for (age_group = 0; age_group < ROW_ever1; age_group++) {
    for (partner_cat = 0; partner_cat < COL_partner_categories_extended; partner_cat++) {
      * xls_output << male_partner_ever[age_group][partner_cat] << '\t';
    }
    * xls_output << male_partner_ever[age_group][partner_cat] << '\n';
  }
  * xls_output << male_partner_ever[age_group][partner_cat] << endl << endl;
  
  //*****************************
  int total_male_partner_ever[ROW_ever1] = {0};
  int total_female_partner_ever[ROW_ever1] = {0};

  for (int age_group = 0; age_group < ROW_ever1; age_group++) {
    for (int i = 0; i < COL_partner_categories; i++) {
      total_male_partner_ever[age_group] += male_partner_ever[age_group][i];
      total_female_partner_ever[age_group] += female_partner_ever[age_group][i];
    }
  }
  
  for (int age_group = 0; age_group < ROW_ever1 ; age_group++) {
    for (int i = 0; i < COL_partner_categories_extended; i++) {
      if (i < COL_partner_categories){
        partner_male_lifetime_stat[age_group][i] = 100.0*double(male_partner_ever[age_group][i])/total_male_partner_ever[age_group];
        partner_female_lifetime_stat[age_group][i] = 100.0*double(female_partner_ever[age_group][i])/total_female_partner_ever[age_group];
      }
      else {
        partner_male_lifetime_stat[age_group][i] = 100.0*double(male_partner_ever[age_group][i])/double(male_partner_ever[age_group][3]);
        partner_female_lifetime_stat[age_group][i] = 100.0*double(female_partner_ever[age_group][i])/double(female_partner_ever[age_group][3]);
      }
    }
  }
  
  //*****************************
  
  //both genders
  if(year==0){
    stringstream filename;
    //filename << "Partner_EVER_" << year << ".txt" ;
    filename << "Partner_EVER.txt" ;
    //string myString = filename.str();
    xls_output_ever = new ofstream(filename.str().c_str(), ios::out );
    * xls_output_ever << "0\t1\t2\t3+\t\t0\t1\t2\t3+\t" << endl;
  }
  * xls_output_ever  << endl;
  
  for (int i = 0; i < ROW_ever1; i++) {
    for (int j = 0; j < COL_partner_categories; j++) {
      * xls_output_ever << roundf(partner_male_lifetime_stat[i][j] * 100) / 100  << '\t' ;
    }
    * xls_output_ever << '\t';
    for (int j = 0; j < COL_partner_categories; j++) {
      * xls_output_ever << roundf(partner_female_lifetime_stat[i][j] * 100) / 100 << '\t' ;
    }
    * xls_output_ever << endl;
  }

  //*****************************
  //Males
  * xls_output_male_ever << endl;
  * xls_output_male_ever << "Year " << Global::Simulation_Day/365 << "\tDay " << Global::Simulation_Day << endl;
  
  for (int i = 0; i < ROW_ever1; i++) {
    for (int j = 0; j < COL_partner_categories; j++) {
      * xls_output_male_ever << partner_male_lifetime_stat[i][j] << '\t' ;
      //printf("%.2f \t" , partner_male_lifetime_stat[i][j]);
    }
    * xls_output_male_ever << endl;
    //cout << endl;
  }
  
  //*****************************
  //Females
  * xls_output_female_ever << endl;
  * xls_output_female_ever << "Year " << Global::Simulation_Day/365 << "\tDay " << Global::Simulation_Day << endl;
  
  for (int i = 0; i < ROW_ever1; i++) {
  //int i = 0; // age group 0 print
  for (int j = 0; j < COL_partner_categories; j++) {
      * xls_output_female_ever << partner_female_lifetime_stat[i][j] << '\t' ;
      printf("%.2f \t" , partner_female_lifetime_stat[i][j]);
    }
    * xls_output_female_ever << endl;
    cout << endl;
  }
  

  //*****************************

   // out files for calibration. for each age group, separate!  xls_output4 : out_'filename in argv'.txt
  //* txt_output4 << time/365 << '\n' ;
  
  for (int i = 0; i < ROW_ever1; i++) {
    for (int j = 0; j < COL_partner_categories; j++) {
      * txt_output4 << std::fixed << partner_male_lifetime_stat[i][j] << '\t' ;
    }
    * txt_output4 << endl;
  }
  //* txt_output4 << endl;
  
  reset_ever_counters();
  
  FRED_VERBOSE(1, "Prepare_report4() finished. \n");
  
}  //end of prepare_report4

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::report_adult_population(){
  
  //report number of adult individuals in each age group
  fprintf (report1, "Year %d\t Day %d\n", Global::Simulation_Day/365, Global::Simulation_Day );
  for (int i = 0; i < 6; i++) {
    fprintf (report1, "%d\t%d\t%d\n" , i, male[i], female[i]);
  }
  fprintf (report1, "Total\t%d\t%d\n", total_male_adult, total_female_adult);
  
  fprintf (report1, "\nMales - Age group 0:\n");
  for (int i = 0; i < 5; i++) {
    fprintf (report1, "%d\t%d\n" , i+15, male_agegroup0[i]);
  }
  fprintf (report1, "Total\t%d\n", total_male_adult_agegroup0);
  fprintf (report1, "\n\n");
  
} //end of report_adult_population()

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::remove_person(vector<Person*> &temp_vec, int i){
  //  cout << "1 temp_vec size = " << temp_vec.size() << endl;
  Person* tempB;
  Person* tempZ;
  tempZ = temp_vec.at(i);
  tempB = temp_vec.at(temp_vec.size()-1);
  temp_vec.at(i)= tempB;
  temp_vec.at(temp_vec.size()-1)=tempZ;
  //  if (tempZ->get_person_sex()=='F') {
  //    cout << "pop_back female id "<< tempZ->get_person_id() << endl;
  //  }
  temp_vec.pop_back();
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::reset_1partner_vectors(){
  //reset
  males_to_match_1partner_FH.clear();
  females_to_match_1partner_FH.clear();
  males_to_match_1partner_nonFH.clear();
  females_to_match_1partner_nonFH.clear();
  
  for (int group = 0; group < 3; group++) {
    males_to_match_1partner_byAge_FH[group].clear();
    females_to_match_1partner_byAge_FH[group].clear();
    males_to_match_1partner_byAge_nonFH[group].clear();
    females_to_match_1partner_byAge_nonFH[group].clear();
  }
}

void Sexual_Transmission_Network::reset_matching_vectors(){
  females_to_match.clear();
  males_to_match.clear();
  for (int group = 0; group < 3; group++) {
    males_to_match_byAge[group].clear();
    females_to_match_byAge[group].clear();
  }
}

void Sexual_Transmission_Network::reset_ever_counters(){
  int age_group = 0;
  int partner_cat = 0;
  
  for (age_group = 0; age_group < ROW_ever1; age_group++) {
    for (partner_cat = 0; partner_cat < COL_partner_categories_extended; partner_cat++) {
      male_partner_ever[age_group][partner_cat] = 0;
      male_partner_inyear_matched[age_group][partner_cat] = 0;
      female_partner_ever[age_group][partner_cat] = 0;
      female_partner_inyear_matched[age_group][partner_cat] = 0;

    }
  }
}

//*******************************************************
//*******************************************************

int Sexual_Transmission_Network::set_num_partner(Person* &individual, double partner_cumulative[ROW_age_group][COL_partner_categories_extended]){
  
  int partner_number = -1;    //partner_number ADDED (additional to what the person has)
  int age_group;
  int temp=0;
  double rand1 = ((double) rand() / (RAND_MAX));
  double rand2 = ((double) rand() / (RAND_MAX));
  double rand3 = ((double) rand() / (RAND_MAX));
  
  Relationships* relationships = individual->get_relationships();
  age_group = relationships->get_current_age_group();
  
  //num_partner_ever_day0 for each relationships shows the partner in lifetime assigned on day 0 for each person during initialization. After day 0, partner_in_age_group shows the count of number of partners in each age group.
  
  if (Global::Simulation_Day > 0 ){
    
    int partner_ever;
    
    if (age_group == 0 ) {
      
      //partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0);
      partner_ever = relationships->get_partner_in_age_group(0);
      
      if (partner_ever == 0) {
        
        if (rand1 <= partner_cumulative[age_group][0])
          partner_number = 0;
        
        else {
          //should not stay in ever 0, move on to another catgory, which means that partner_number should not stay 0.
          if (rand2 <= partner_cumulative[age_group][1])      // add 1 partner to move to 1 ever
            partner_number = 1;
          else {
            double rand3 = ((double) rand() / (RAND_MAX));
            if (rand3 <= partner_cumulative[age_group][2])
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1])
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2]) { // add 1 partner to go to 2 ever
            partner_number = 1;
          }
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2])
          partner_number = 0;
        else {
          partner_number = 1;
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
      
    }  //end of if (age_group == 0)
    
    else if (age_group == 1 ) {

      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1);
      
      if (partner_ever == 0) {
        if (rand1 <= partner_cumulative[age_group][0])  // stay in 0 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          //should not stay in ever 0, move on to another catgory, which means that partner_number should not stay 0.
          if (rand2 <= partner_cumulative[age_group][1])      // add 1 partner to move to 1 ever
            partner_number = 1;
          else {
            double rand3 = ((double) rand() / (RAND_MAX));
            if (rand3 <= partner_cumulative[age_group][2] )
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1] )   // stay in 1 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2])  // add 1 partner to go to 2 ever
            partner_number = 1;
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2] ) //stay in 2 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          partner_number = 1;        //add 1 partner to go to at least 3 ever
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
    }  //end of if (age_group == 1)
    
    else if (age_group == 2 ) {
      
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2);
      
      if (partner_ever == 0) {
        if (rand1 <= partner_cumulative[age_group][0])   //stay in 0 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          //should not stay in ever 0, move on to another catgory, which means that partner_number should not stay 0.
          if (rand2 <= partner_cumulative[age_group][1])       //add 1 partner to move to 1 ever
            partner_number = 1;
          else {
            if (rand3 <= partner_cumulative[age_group][2] )
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1] )    //stay in 1 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2])   //add 1 partner to go to 2 ever
            partner_number = 1;
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2] ) //stay in 2 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          partner_number = 1;        //add 1 partner to go to at least 3 ever
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
    } //end of if (age_group == 2)
    
    else if (age_group == 3 ) {
      
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3);
      
      if (partner_ever == 0) {
        if (rand1 <= partner_cumulative[age_group][0])  // stay in 0 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          //should not stay in ever 0, move on to another catgory, which means that partner_number should not stay 0.
          if (rand2 <= partner_cumulative[age_group][1])       //add 1 partner to move to 1 ever
            partner_number = 1;
          else {
            if (rand3 <= partner_cumulative[age_group][2] )
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1] )    //stay in 1 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2])  // add 1 partner to go to 2 ever
            partner_number = 1;
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2] ) //stay in 2 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          partner_number = 1;       // add 1 partner to go to at least 3 ever
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
    } //end of if (age_group == 3)
    
    else if (age_group == 4 ) {
      
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3) + relationships->get_partner_in_age_group(4);
      
      if (partner_ever == 0) {
        if (rand1 <= partner_cumulative[age_group][0])
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][1])
            partner_number = 1;
          else {
            if (rand3 <= partner_cumulative[age_group][2] )
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1] )   // stay in 1 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2])   //add 1 partner to go to 2 ever
            partner_number = 1;
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2] ) //stay in 2 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          partner_number = 1;      //  add 1 partner to go to at least 3 ever
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
    } //end of if (age_group == 4)
    
    else if (age_group == 5 ) {
      
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3) + relationships->get_partner_in_age_group(4)  + relationships->get_partner_in_age_group(5);
      
      if (partner_ever == 0) {
        if (rand1 <= partner_cumulative[age_group][0])
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][1])       //add 1 partner to move to 1 ever
            partner_number = 1;
          else {
            if (rand3 <= partner_cumulative[age_group][2] )
              partner_number = 2;
            else
              partner_number = 3;
          }
        }
      }
      else if (partner_ever == 1) {
        if (rand1 <= partner_cumulative[age_group][1] )  //  stay in 1 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          if (rand2 <= partner_cumulative[age_group][2])   //add 1 partner to go to 2 ever
            partner_number = 1;
          else
            partner_number = 2;
        }
      }
      else if (partner_ever == 2) {
        if (rand1 <= partner_cumulative[age_group][2] ) //stay in 2 ever, means num_pertners = 0;
          partner_number = 0;
        else {
          partner_number = 1;        //add 1 partner to go to at least 3 ever
        }
      }
      else if (partner_ever >= 3) {
        partner_number = 0;
      }
    } //end of if (age_group == 5)
    
    
    //**********************
    //4 or more partners, using lognormal fitted with
    //discrete lognormal distribution in R. input: cdf_lognorm[47], for 4 to 50 partners
    //double rand3 = ((double) rand() / (RAND_MAX));
    //partner_number = get_lognormal1(cdf_lognorm, rand3);
    
  }  // end of else if (time > 0 )
  
  return partner_number;
}   // end of set_num_partner

//*******************************************************
//*******************************************************

int Sexual_Transmission_Network::set_dur_utility(int age_group, bool long_term, int age_at_first_marriage_category, double dur[9][3]){
  
  double rand1 = ((double) rand() / (RAND_MAX));
  double rand3 = ((double) rand() / (RAND_MAX));
  int duration, duration_days;
  
  if (long_term == false) {
    if (rand1 <= dur[age_group][0]) duration = get_uniform(1, 12);
    else if (rand1 > dur[age_group][0]) duration = get_uniform(13, 36);
  }
  else if (long_term == true) {
    duration = get_marriage_dur(rand3, age_at_first_marriage_category);
  }
  
  duration_days = duration*30;
  return duration_days;   //return in days
  
}   //end of set_dur_utility

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::set_sexual_acts(int day){   //run this for each DAY!!!
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    if (person->get_sex() == 'M' && relationships->get_person_num_partners_label() > 0 && relationships->get_partner_list_size() > 0 && relationships->check_relationships_eligiblity() && person->is_enrolled_in_network(this) == true) {
      p_vector.push_back(person);
    }
  }
  
  for (int i = 0; i < p_vector.size(); i++) {   //for males
    Person* person = p_vector.at(i);
    Relationships* relationships = person->get_relationships();
    int num_matched_partners = relationships->get_partner_list_size();
    bool sexual_act_today = false;
    double rand_num1 = ((double) rand() / (RAND_MAX));
    int out_degree = person->get_out_degree(this);
    
    if (num_matched_partners != out_degree) {
      cout << "ERROR: set_sexual_frequency(). out_degree is not qual to num_matched_partners for male. Press key to continue.\n" << endl;
      getchar();  //for error
    }
    
    for (int j = 0; j < num_matched_partners; j++) {    //reset sexual acts: set to false
      relationships->reset_sexual_act_today(j);
    }
    
    if (person->get_age() >= 15 && person->get_age()<30 && rand_num1 <= sexual_freq_male_15_29) {
      sexual_act_today = true;
    }
    else if (person->get_age() >= 30 && person->get_age()<40 && rand_num1 <= sexual_freq_male_30_39) {
      sexual_act_today = true;
    }
    else if (person->get_age() >= 40 && person->get_age()<50 && rand_num1 <= sexual_freq_male_40_49) {
      sexual_act_today = true;
    }
    else if (person->get_age() >= 50 && person->get_age()<60 && rand_num1 <= sexual_freq_male_50_59) {
      sexual_act_today = true;
    }
    else if (person->get_age() >= 60 && person->get_age()<70 && rand_num1 <= sexual_freq_male_60_69) {
      sexual_act_today = true;
    }
    else if (person->get_age()>=70 && rand_num1 <= sexual_freq_male_over70){
      sexual_act_today = true;
    }
    
    if (sexual_act_today == true) {
      
      //randomly pick one of the partners for the sexual act today
      int rand_num2 = -1;
      rand_num2 = rand() % num_matched_partners;
      relationships->set_sexual_act_today(rand_num2);
      
      Person* partner = person->get_end_of_link(rand_num2,this);
      Relationships* partner_relationships = partner->get_relationships();
      
      int partner_num_matched_partners = partner_relationships->get_partner_list_size(); //number of female's matched partner
      int in_degree = partner->get_in_degree(this);
      
      if (partner->get_sex() != 'F') {
        cout << "ERROR: set_sexual_frequency(). matched partner of the male is not female. Press key to continue.\n" << endl;
        getchar();  //for error
      }
      else if (partner_num_matched_partners != in_degree) {
        cout << "ERROR: set_sexual_frequency(). in_degree is not qual to num_matched_partners for female. Press key to continue.\n" << endl;
        getchar();  //for error
      }
      else {
        for (int m = 0; m < partner_num_matched_partners; m++) {
          int male_partner_id = partner_relationships->get_matched_partner_id(m);
          if (male_partner_id == person->get_id())
            partner_relationships->set_sexual_act_today(m);
        } //end of for m
      }
      
      fprintf(report_sexual_acts, "Day %d\tMale ID %d\tAge %d\t num_matched_partners %d\t sxual act with partner %d\n" , day, person->get_id(), person->get_age(), num_matched_partners, partner->get_id());
    }
  }  //end of for
  
  
  p_vector.clear();
  FRED_VERBOSE(1, "set_sexual_frequency finished. \n" );
}   //end of set_sexual_acts

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::update_time_past_remove_partnerships(){
  
  FRED_VERBOSE(1, "MINA update_time_past_remove_partnerships() started.\n");
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if (Global::Simulation_Day != 0 && person->get_sex() == 'M' && relationships->get_partner_list_size() > 0 && relationships->check_relationships_eligiblity()) {   //use > 0 to exclue -1s as well
      
      //add time in relationship (increase by 1 day for each partnership)
      for (int j = 0; j < relationships->get_partner_list_size(); j++) {
        
        relationships->decrease_time_left_in_relationship(j);  //j = partner place in time_past_in_relationship vector
        
        Person* partner = relationships->get_partner_from_list(j);
        Relationships* partner_relationships = partner->get_relationships();
        int partner_partner_list_size = partner_relationships->get_partner_list_size(); //number of female's matched partner
        //partner j's partners: find the male 'person' in female's partner list
        for (int m = 0; m < partner_partner_list_size; m++) {
          int male_partner_id = partner_relationships->get_matched_partner_id(m);
          if (male_partner_id == person->get_id()) {   //person is the same male partner we have here
            partner_relationships->decrease_time_left_in_relationship(m);
          }
        }
      }
      
      //remove the partners whose relationship duration is over
      for (int j = relationships->get_partner_list_size(); j>0; j--) {
        
        if (relationships->get_time_past_in_relationship(j-1) == relationships->get_partner_dur(j-1)) {
          
          //remove from female's partner before removing from male's list:
          Person* partner = relationships->get_partner_from_list(j-1);
          Relationships* partner_relationships = partner->get_relationships();
          
          if (partner->get_id() == Global::TEST_ID) {
            cout << "DEBUG3333 " << j << "\t" << partner_relationships->get_partner_list_size() << endl;
            getchar();  //for TEST_ID
          }
          
          for (int m = partner_relationships->get_partner_list_size(); m>0; m--) {
            int male_partner_id = partner_relationships->get_matched_partner_id(m-1);
            if (male_partner_id == person->get_id()) {   //person is the same male partner we have here
              
              if (partner->get_id() == Global::TEST_ID) {
                cout << "Male ID1 " << person->get_id() << "  Female ID2 " << partner->get_id() << "  is connected to? " << person->is_connected_to(partner, this) << endl;
                cout << "Female ID2 " << partner->get_id() << "  Male ID1 " << person->get_id() << "  is connected from? " << partner->is_connected_from(person, this) << endl;
                getchar();  //for TEST_ID
              }
              
              //delete network links:
              if(person->is_connected_to(partner, this) == true && partner->is_connected_from(person, this) == true) {
                partner->destroy_network_link_from(person, this);
                person->destroy_network_link_to(partner, this);
              }
              else {
                cout << "ERROR in Sexual networks update_time_past_remove_partnerships()- Female. male person ID: " <<  person->get_id() << "  female partner ID: " << partner->get_id() << "  press key to continue." << endl;
                cout << "ERROR in Sexual networks update_time_past_remove_partnerships()- Male." << " male person ID: " << person->get_id() << "  press key to continue." << endl;
                getchar();  //for error
              }
              
              partner_relationships->remove_from_partner_list(m-1);
              partner_relationships->set_person_num_partners_label((partner_relationships->get_person_num_partners_label())-1) ;
              
              if (partner->get_id() == Global::TEST_ID) {
                cout << "DEBUG3333 " << j << "\t" << partner_relationships->get_partner_list_size() << endl;
                getchar();  //for TEST_ID
              }
              
              
              if (partner_relationships->get_partner_list_size() < 0) {
                FRED_VERBOSE(0, "ERROR in Sexual networks update_time_past_remove_partnerships()- num_matched_patner less than 0. female person ID: %d. press key to continue.\n", partner->get_id());
                getchar();  //for error
                exit(0);
              }
            }
          }
          
          relationships->remove_from_partner_list(j-1);
          relationships->set_person_num_partners_label((relationships->get_person_num_partners_label())-1) ;
          if (partner_relationships->get_partner_list_size() < 0) {
            cout << "ERROR in Sexual networks update_time_past_remove_partnerships()- num_matched_patner less than 0" << " male person ID: " << person->get_id() << "  press key to continue." << endl;
            getchar();  //for error
          }
          if (person->get_id() == Global::TEST_ID) {
            cout << "Update time past in relationship Male ID1 " << person->get_id() << "  Female ID2 " << partner->get_id() << "  is connected to? " << person->is_connected_to(partner, this) << endl;
            cout << "pdate time past in relationship Female ID2 " << partner->get_id() << "  Male ID1 " << person->get_id() << "  is connected from? " << partner->is_connected_from(person, this) << endl;
          }
        }
      } //end of for
    } //end of if (person->get_num_matched > 0 )
  } //end of for all people
  FRED_VERBOSE(1, "MINA update_time_past_remove_partnerships() finished.\n");
}

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::update_stats_ever(Person* temp_person){
  FRED_VERBOSE(1, "Mina: update_stats_ever started.\n");
  //partners EVER: report at ages 20, 25, 30, 35, 40, 45+
  //categories for #partners ever: 0,	1,	2,	3–6, 7–14,	15 or more
  //this method is executed every day for each person if the person's AGE GROUP has changed at the BEGINNING of the day. For example, it is run at the beginning day of age 20, to calculate counters for age group 15-19.
  
  Relationships* relationships = temp_person->get_relationships();
  int age_group = -1;
  
  if (relationships->check_relationships_eligiblity() && temp_person->is_enrolled_in_network(this)) {
    
    int partner_ever = 0;
    
    //15-19, age group 0
    if (temp_person->get_real_age() >= 19.0 && temp_person->get_real_age() < 20.0 ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0);
      age_group = 0;
    }
    //20-24, age group 1
    else if (temp_person->get_age() >= 24.0 && temp_person->get_real_age() < 25.0  ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) ;
      age_group = 1;
    }
    //25-29, age group 2
    else if (temp_person->get_age() >= 29.0 && temp_person->get_real_age() < 30.0 ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2);
      age_group  = 2;
    }
    //30-34, age group 3
    else if (temp_person->get_age() >= 34.0  && temp_person->get_real_age() < 35.0 ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3);
      age_group  = 3;
    }
    //35-49, age group 4
    else if (temp_person->get_age() >= 39.0 && temp_person->get_real_age() < 40.0 ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3) + relationships->get_partner_in_age_group(4);
      age_group  = 4;
    }
    //40-44, age group 5
    else if (temp_person->get_age() >= 44.0 && temp_person->get_real_age() < 45.0 ) {
      partner_ever = relationships->get_person_num_partners_ever_day0() + relationships->get_partner_in_age_group(0) + relationships->get_partner_in_age_group(1) + relationships->get_partner_in_age_group(2) + relationships->get_partner_in_age_group(3) + relationships->get_partner_in_age_group(4) + relationships->get_partner_in_age_group(5);
      age_group  = 5;
    }
    
    if(age_group != -1){
      
      if (temp_person->get_sex() == 'M') {
        if (partner_ever == 0) male_partner_ever[age_group][0]++;
        else if (partner_ever == 1) male_partner_ever[age_group][1]++;
        else if (partner_ever == 2) male_partner_ever[age_group][2]++;
        else if (partner_ever >= 3) {
          male_partner_ever[age_group][3]++;
          if (partner_ever>=3 && partner_ever<7) male_partner_ever[age_group][4]++;
          else if (partner_ever>=7 && partner_ever<15) male_partner_ever[age_group][5]++;
          else if (partner_ever>=15) male_partner_ever[age_group][6]++;
        }

      }
      else if (temp_person->get_sex() == 'F') {
        if (partner_ever == 0) female_partner_ever[age_group][0]++;
        else if (partner_ever == 1) female_partner_ever[age_group][1]++;
        else if (partner_ever == 2) female_partner_ever[age_group][2]++;
        else if (partner_ever >= 3) {
          female_partner_ever[age_group][3]++;
          if (partner_ever>=3 && partner_ever<7) female_partner_ever[age_group][4]++;
          else if (partner_ever>=7 && partner_ever<15) female_partner_ever[age_group][5]++;
          else if (partner_ever>=15) female_partner_ever[age_group][6]++;
        }
      }
      
    }

  }   //end of counting
  
  FRED_VERBOSE(1, "Mina: update_stats_ever finished.\n");
  
} //end of update_stats_ever

//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::label_females_day0(){
  
  FRED_VERBOSE(0, "MINA label_females_day0() started.\n" );
  
  //**********************************
  //female labeling: expected number of females in each age and sex category with X number of partners
  for (int age_group = 0; age_group < 6; age_group++) {
    for (int partner_cat = 0 ; partner_cat < 4; partner_cat++) {
      female_expected_partner[age_group][partner_cat] = f_partner[age_group][partner_cat] * female[age_group];
    }
  }
  
  for (int age_group = 0; age_group < 6; age_group++) {
    total_female_1partner += female_expected_partner[age_group][1] ; //counting expected monogamous
    total_female_2partner += female_expected_partner[age_group][2] ; //counting expected 2-partner
    total_female_3partner += female_expected_partner[age_group][3] ; //counting expected 3-partner
  }
  
  fprintf (report2, "\n\nExpected partners FEMALES:\n" );
  fprintf (report2, "Age Group\t0 partner\t1 partner\t2 partner\t3 partner\n" );
  for (int age_group = 0; age_group< 6; age_group++) {
    fprintf (report2, "%d", age_group);
    for (int partner_cat = 0; partner_cat < 4; partner_cat++) {
      fprintf (report2, "\t%d", female_expected_partner[age_group][partner_cat] );
      //total1 += female_expected_partner[age_group][partner_cat] ;
    }
    fprintf (report2, "\n");
  }

  //**********************************
  
  //reset counters
  female_1partner_labeled = 0;
  for (int i = 0 ; i < ROW_age_group; i++) {
    female_labeled_1partner[i] = 0;
    females_to_label_monogamous[i].clear();
    female_labeled_2partner[i] = 0;
    females_to_label_2partner[i].clear();
    female_labeled_3partner[i] = 0;
    females_to_label_3partner[i].clear();
  }
  
  //prepare a vector of females in sexual network. Females who are eligible to have relationships
  vector<Person*> p_vector;
  for (int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    
    if (person->get_sex() == 'F') relationships->set_current_age_group(relationships->calculate_current_age_group());   //mina check
    
    if (person->get_sex() == 'F' && relationships->check_relationships_eligiblity() && relationships->get_person_num_partners_label() == -1  && person->is_enrolled_in_network(this) == true ) {
      p_vector.push_back(person);
    }
    
  }
  
  //************************** 1 partner
  int partner_cat = 1;
  for (int i=0; i< p_vector.size(); i++) {
    Person* individual = p_vector.at(i);
    Relationships* relationships = individual->get_relationships();
    int age_group = relationships->get_current_age_group();
    
    double rand1 = ((double) rand() / (RAND_MAX));
    double rand2 = ((double) rand() / (RAND_MAX));
    
    //Females available to be labeled mongamous- family HH type
    if (relationships->get_htype() == 2 || relationships->get_htype() == 4 || relationships->get_htype() == 7) {
      for (int group = 0; group < ROW_age_group; group++) {
        if (age_group == group && rand1 > f_partner_cumulative[age_group][partner_cat-1] && rand1 <= f_partner_cumulative[age_group][partner_cat] && rand2 <= prob_mono_1) {
          relationships->set_person_num_partners_label(1);   //mina check: set number of partners ever for females later
          female_labeled_1partner[age_group]++;
          female_1partner_labeled++;  //count total 1-partner labeled
          
          //********************
          // females_to_label_monogamous[group].push_back(individual); //mina check
          
        }
      }
    }
  } // end of for loop
  
  int total = 0;
  //  fprintf (report2, "'labeled' 1 partner FEMALES- family HH type:\n" );
  //  fprintf (report2, "Age Group\tLabeled Monogamous\n" );
  //    for (int age_group = 0; age_group< 6; age_group++) {
  //      fprintf (report2, "%d\t%d\n", age_group, female_labeled_1partner[age_group]);
  //      total += female_labeled_1partner[age_group];
  //    }
  //    fprintf (report2, "Total\t%d\n\n", total);
  
  //Females available to be labeled mongamous- non-family HH type
  if (female_1partner_labeled < total_female_1partner-20) {  // mina check -20 . if not enough females labeled monogamous in step 1
    for (int i=0; i< p_vector.size(); i++) {
      Person* individual = p_vector.at(i);
      Relationships* relationships = individual->get_relationships();
      int age_group = relationships->get_current_age_group();
      
      if (relationships->get_person_num_partners_label() == -1 && relationships->get_htype() != 2 && relationships->get_htype() != 4 && relationships->get_htype() != 7 && relationships->get_htype()!= -1 ){
        
        for (int group = 0; group < 6; group++) {
          if (age_group == group)
            females_to_label_monogamous[group].push_back(individual);
        }
      }
    } //end of for
  }
  

  //  fprintf (report2, "Additional females available to label monogamous, non-family hh type:\n" );
  //  fprintf (report2, "Age Group\tAvailable for monogamous label\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%lu\n", age_group, females_to_label_monogamous[age_group].size());
  //    total += females_to_label_monogamous[age_group].size();
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  
  for (int age_group = 0; age_group < 6; age_group++) {
    while (female_labeled_1partner[age_group] < female_expected_partner[age_group][partner_cat] && females_to_label_monogamous[age_group].size() != 0 ){
      
      int rand_num = rand() % females_to_label_monogamous[age_group].size() ;   //randomly select person from list
      Person* temp_person = females_to_label_monogamous[age_group].at(rand_num);    //get person
      Relationships* relationships = temp_person->get_relationships();
      
      double rand1 = ((double) rand() / (RAND_MAX));
      double rand2 = ((double) rand() / (RAND_MAX));
      
      if (rand1 > f_partner_cumulative[age_group][partner_cat-1] && rand1 <= f_partner_cumulative[age_group][partner_cat] && rand2 <= prob_mono_2) {
        relationships->set_person_num_partners_label(1);
        female_labeled_1partner[age_group]++;
        remove_person(females_to_label_monogamous[age_group], rand_num);
      }
    }
  }
  
  //  fprintf (report2, "\n'labeled' 1 partner FEMALES:\n" );
  //  fprintf (report2, "Age Group\tLabeled Monogamous\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%d\n", age_group, female_labeled_1partner[age_group]);
  //    total += female_labeled_1partner[age_group];
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  //
  
  //************************** 2 partner
  partner_cat = 2;
  total=0;
  //Females available to be labeled 2-partner
  for (int i=0; i< p_vector.size(); i++) {
    Person* individual = p_vector.at(i);
    Relationships* relationships = individual->get_relationships();
    int age_group = relationships->get_current_age_group();
    
    if (relationships->get_person_num_partners_label() == -1){
      for (int group = 0; group < 6; group++) {
        if (age_group == group)
          females_to_label_2partner[age_group].push_back(individual);
      }
    }
  } //end of for
  

  //  fprintf (report2, "Females available to label 2 partner:\n" );
  //  fprintf (report2, "Age Group\tAvailable for 2-partner label\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%lu\n", age_group, females_to_label_2partner[age_group].size());
  //    total += females_to_label_2partner[age_group].size();
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  
  for (int age_group = 0; age_group < 6; age_group++) {
    while (female_labeled_2partner[age_group] < female_expected_partner[age_group][partner_cat] && females_to_label_2partner[age_group].size() != 0 ){
      int rand_num = rand() % females_to_label_2partner[age_group].size() ;   //randomly select person from list
      Person* temp_person = females_to_label_2partner[age_group].at(rand_num);    //get person
      Relationships* relationships = temp_person->get_relationships();
      
      double rand1 = ((double) rand() / (RAND_MAX));
      double rand2 = ((double) rand() / (RAND_MAX));
      
      if (rand1 > f_partner_cumulative[age_group][partner_cat-1] && rand1 <= f_partner_cumulative[age_group][partner_cat] && rand2 <= prob_2partner_1) {
        relationships->set_person_num_partners_label(2);
        female_labeled_2partner[age_group]++;
        remove_person(females_to_label_2partner[age_group], rand_num);
      }
    }
  }
  
  //  fprintf (report2, "'labeled' 2-partner FEMALES:\n" );
  //  fprintf (report2, "Age Group\tLabeled 2-partner\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%d\n", age_group, female_labeled_2partner[age_group]);
  //    total += female_labeled_2partner[age_group];
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  //  cout << "label_2partner Females Finished." << endl;
  //  cout << "\nLabeling 3-partner Females **************" << endl;
  //
  partner_cat = 3;
  total=0;
  //  fprintf (report2, "Expected 3-partner FEMALES:\n" );
  //  fprintf (report2, "Age Group\tExpected 3-partner\n" );
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%d\n", age_group, female_expected_partner[age_group][partner_cat] );
  //    total += female_expected_partner[age_group][partner_cat] ;
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  
  //Females to be labeled 3-partner
  for (int i=0; i< p_vector.size(); i++) {
    Person* individual = p_vector.at(i);
    Relationships* relationships = individual->get_relationships();
    int age_group = relationships->get_current_age_group();
    
    if (relationships->get_person_num_partners_label() == -1){
      for (int group = 0; group < 6; group++) {
        if (age_group == group)
          females_to_label_3partner[group].push_back(individual);
      }
    }
  } //end of for
  
  //  fprintf (report2, "Females available to label 3-partner:\n" );
  //  fprintf (report2, "Age Group\tAvailable for label\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%lu\n", age_group, females_to_label_3partner[age_group].size());
  //    total += females_to_label_3partner[age_group].size();
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  
  for (int age_group = 0; age_group < 6; age_group++) {
    while (female_labeled_3partner[age_group] < female_expected_partner[age_group][partner_cat] && females_to_label_3partner[age_group].size() != 0 ){
      int rand_num = rand() % females_to_label_3partner[age_group].size() ;   //randomly select person from list
      Person* temp_person = females_to_label_3partner[age_group].at(rand_num);    //get person
      Relationships* relationships = temp_person->get_relationships();
      
      double rand1 = ((double) rand() / (RAND_MAX));
      double rand2 = ((double) rand() / (RAND_MAX));
      
      if (rand1 > f_partner_cumulative[age_group][partner_cat-1] && rand1 <= f_partner_cumulative[age_group][partner_cat] && rand2 <= prob_3partner_1) {
        relationships->set_person_num_partners_label(3);
        female_labeled_3partner[age_group]++;
        remove_person(females_to_label_3partner[age_group], rand_num);
      }
    }
  }
  
  //  fprintf (report2, "'labeled' 3-partner FEMALES:\n" );
  //  fprintf (report2, "Age Group\tLabeled 3-partner\n" );
  //  total=0;
  //  for (int age_group = 0; age_group< 6; age_group++) {
  //    fprintf (report2, "%d\t%d\n", age_group, female_labeled_3partner[age_group]);
  //    total += female_labeled_3partner[age_group];
  //  }
  //  fprintf (report2, "Total\t%d\n\n", total);
  //
  //  cout << "label_3partner Finished." << endl;
  
  
  //Females to be labeled 0-partner; whoever is left.
  for (int i=0; i< p_vector.size(); i++) {
    Person* individual = p_vector.at(i);
    Relationships* relationships = individual->get_relationships();
    int age_group = relationships->get_current_age_group();
    if (relationships->get_person_num_partners_label() == -1)
      relationships->set_person_num_partners_label(0);
  } //end of for
  
  p_vector.clear();
  FRED_VERBOSE(1, "MINA label_females_day0() finished.\n" );
}  //end of label_females_day0()

//*******************************************************
//*******************************************************

void Sexual_Transmission_Network::label_females(){
  
  int temp = 0;
  // new females that are added to the network at 15 need a partner label for matching
  for(int p = 0; p < Global::Pop.get_population_size(); ++p) {
    Person* person = Global::Pop.get_person(p);
    Relationships* relationships = person->get_relationships();
    int age_group = relationships->get_current_age_group();
    
    if (Global::Simulation_Day != 0 && person->get_sex() == 'F' && person->get_real_age() >= 15 && person->get_real_age() < 16 && person->is_enrolled_in_network(this) && relationships->check_relationships_eligiblity() && relationships->get_person_num_partners_label() == -1 && age_group == 0 ){
      
      int partner_number = -1;    //partner_number ADDED (additional to what the person has)
      double rand1 = ((double) rand() / (RAND_MAX));
      
      if (rand1 <= f_partner_cumulative[age_group][0] )
        partner_number = 0;
      else if (rand1 > f_partner_cumulative[age_group][0] && rand1 <= f_partner_cumulative[age_group][1] )
        partner_number = 1;
      else if (rand1 > f_partner_cumulative[age_group][1] && rand1 <= f_partner_cumulative[age_group][2] )
        partner_number = 2;
      else if (rand1 > f_partner_cumulative[age_group][2])
        partner_number = 3;
      
      
      relationships->set_person_num_partners_label(partner_number);
      temp++;
    }
  }
  //cout << "New 15 year old females added this year = " << temp << endl;
  FRED_VERBOSE(1, "MINA label_females() finished.\n" );
}  //end of label_females()




