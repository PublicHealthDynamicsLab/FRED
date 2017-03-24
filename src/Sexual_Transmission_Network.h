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
// File: Sexual_Transmission_Network.h
//

#ifndef _FRED_SEXUAL_TRANSMISSION_NETWORK_H
#define _FRED_SEXUAL_TRANSMISSION_NETWORK_H

#include "Network.h"
#include <algorithm>
using namespace std;
 
class Condition;

#define LABEL_FEMALES 0

class Sexual_Transmission_Network : public Network {
public:
  
  //********** Calibration static parameters
  static bool BASH_SCRIPT;
  static const int ARGC_expected = 12;
  static int calib_age_group;
  static int calib_column;
  static double calib_col0;
  static double calib_col1;
  static double calib_col2;
  static double calib_col3;
  
  static char* calib_filename;
  
  //********** run time parameters
  //static const int TEST_ID = -1;    //moved to Global.h
  static const int LAST_ID = 1218694;
  //last ID read in synthetic population. will get segmentation faults
  //is working on people with IDs higeher than this, since they are
  //added AFTER initialization of network
  
  //********** constant number of rows/columns
  //Age groups:
  //  15_19;  0
  //  20_24;  1
  //  25_29;  2
  //  30_34;  3
  //  35_39;  4
  //  40_44;  5
  //  45_49;  6
  //  50_54;  7
  //  55over (<75) ; 8
  // 75+  not defined
  
  static const int ROW_ever = 30;       //for input file
  static const int ROW_age_group = 9;   //number of age groups
  static const int ROW_ever1 = 6;       //number of age groups
  //partner categories: 0, 1, 2, 3 or more, 3-6, 7-14, 15 and over
  static const int COL_partner_categories_extended = 7;
  //partner categories: 0, 1, 2, 3 or more
  static const int COL_partner_categories = 4;
  
  //*******************************************************
  //*******************************************************
  //Static data and counters
  
  static ofstream* xls_output;
  
  static ofstream* xls_output_male_ever;
  static ofstream* xls_output_female_ever;
  static ofstream* xls_output_ever;
  
  static ofstream* xls_output3;
  static ofstream* txt_output4;
  static ifstream* inFile3;
  
  //output order: integer age, partner ever, temp partner in year, duration for each current partner
  static struct people_struct {
    int age;
    int partner_ever;
    int temp_partner_in_year;
    vector<int> dur;
    vector<int> dur_past;
  } people_15[10000], people_16[10000], people_17[10000], people_18[10000], people_19[10000], people_20[10000], people_21[10000], people_22[10000], people_23[10000], people_24[10000], people_25[10000], people_26[10000], people_27[10000], people_28[10000], people_29[10000], people_30[10000], people_31[10000], people_32[10000], people_33[10000], people_34[10000], people_35[10000], people_36[10000], people_37[10000], people_38[10000], people_39[10000], people_40[10000], people_41[10000], people_42[10000], people_43[10000], people_44[10000];
  
  static people_struct* people_str;
  static vector<people_struct> people_str_vec;
  
  static vector<Person*> females_to_match_1partner_FH;
  static vector<Person*> males_to_match_1partner_FH;
  static vector<Person*> females_to_match_1partner_nonFH;
  static vector<Person*> males_to_match_1partner_nonFH;

  static vector<Person*> females_to_match;
  static vector<Person*> males_to_match;
  

  static vector<Person*> females_to_match_1partner_byAge_FH[3];
  static vector<Person*> males_to_match_1partner_byAge_FH[3];
  static vector<Person*> females_to_match_1partner_byAge_nonFH[3];
  static vector<Person*> males_to_match_1partner_byAge_nonFH[3];

  static vector<Person*> females_to_match_byAge[3];
  static vector<Person*> males_to_match_byAge[3];
  
  static vector<Person*> females_to_label_monogamous[ROW_age_group];
  static vector<Person*> females_to_label_2partner[ROW_age_group];
  static vector<Person*> females_to_label_3partner[ROW_age_group];
  
  static double partner_male_lifetime_stat[ROW_ever][COL_partner_categories_extended];
  static double partner_female_lifetime_stat[ROW_ever][COL_partner_categories_extended];

  static double f_partner[ROW_age_group][COL_partner_categories];
  static double m_partner[ROW_age_group][COL_partner_categories];


  static double f_partner_cumulative[ROW_age_group][COL_partner_categories_extended];
  static double m_partner_cumulative[ROW_age_group][COL_partner_categories_extended];
  static double f_mixing[3][3];
  static double m_mixing[3][3];
  static double f_partner_3ormore[6][5];
  static double m_partner_3ormore[6][5];
  static double f_dur[9][3];
  static double m_dur[9][3];
  
  static double m_partner_ever_day0[ROW_ever][COL_partner_categories];
  static double f_partner_ever_day0[ROW_ever][COL_partner_categories];

  static double cdf_lognorm[47];
  
  //******************************
  static int female[ROW_age_group];
  static int male[ROW_age_group];
  static int male_agegroup0[5];
  
  static int total_adult;
  static int total_female_adult;
  static int total_male_adult;
  static int total_male_adult_agegroup0;
  
  static int total_female_1partner;
  static int total_female_2partner;
  static int total_female_3partner;
  
  static int female_1partner_labeled;   //total female 1partner
  static int female_2partner_labeled;
  static int female_3partner_labeled;
  
  static int female_labeled_1partner[ROW_age_group];  //female 1partner in each age group.
  static int female_labeled_2partner[ROW_age_group];
  static int female_labeled_3partner[ROW_age_group];
  
  // domensions: age group * partner ever category
  static int female_expected_partner[ROW_age_group][COL_partner_categories];
  static int male_expected_partner[ROW_age_group][COL_partner_categories];
  
  static int female_labeled_partner[ROW_age_group][COL_partner_categories];
  static int male_labeled_partner[ROW_age_group][COL_partner_categories];
  
  static int female_partner_inyear[ROW_age_group][COL_partner_categories_extended];
  static int male_partner_inyear[ROW_age_group][COL_partner_categories_extended];
  
  static int female_partner_ever[ROW_age_group][COL_partner_categories_extended];
  static int male_partner_ever[ROW_age_group][COL_partner_categories_extended];
  
  static int male_partner_inyear_matched[ROW_ever1][COL_partner_categories_extended];
  static int female_partner_inyear_matched[ROW_ever1][COL_partner_categories_extended];
  
  static int male_partner_matched_total[ROW_age_group];
  static int female_partner_matched_total[ROW_age_group];
  
  //*******************************************************
  //******************   METHODS  *************************
  
  //initialization methods
  Sexual_Transmission_Network(const char* lab);
  ~Sexual_Transmission_Network() {}
  void read_sexual_partner_file(char* sexual_partner_file);
  void get_parameters();
  void setup(int argc, char* argv[]);
  void initialize_static_variables();
  void open_input_files();
  void open_output_files();
  void count_adult_females();
  void count_adult_males();
  
  void label_females();
  void label_females_day0();

  
  //Update Methods
  void add_new_people_to_network();
  bool age_mixing(int i, int j, double m_mixing[3][3]);
  void adjust_concurrency_day0();
  void adjust_concurrency(int today);
  void add_person_to_sexual_partner_network(Person* person);
  void calculate_dur_partnership_day0(Person* &individual, double dur[9][3]);
  void calculate_dur_partnership(Person* &individual, double dur[9][3]);
  bool check_partner_duplicate(Person* person1, Person* person2);
  int get_marriage_dur(double random_number, int age_at_first_marriage_category);
  double get_prob_first_marriage(double age);
  void matching_process();
  void match_monogamous();
  void match_males(vector<Person*> &male_vector, vector<Person*> (&females_vec_byAge)[3], double m_mixing[3][3]);
  void prepare_matching_vectors(int type, vector<Person*> &p_vector);
  int set_num_partner_day0(Person* &individual, double partner_ever_day0[Sexual_Transmission_Network::ROW_ever][Sexual_Transmission_Network::COL_partner_categories]);
  int set_num_partner(Person* &individual, double partner_cumulative[ROW_age_group][COL_partner_categories_extended]);
  void set_sexual_frequency();
  void set_sexual_acts(int day);
  int set_dur_utility(int age_group, bool long_term, int age_at_first_marriage_category, double dur[9][3]);
  void update(int day);
  void update_time_past_remove_partnerships();
  //void update_stats_ever_day0(Person* person);
  void update_stats_ever(Person* person);
  
  //Utility methods
  void reset_1partner_vectors();
  void reset_matching_vectors();
  void reset_ever_counters();
  void remove_person(vector<Person*> &temp_vec, int i);

  //report methods
  void report_adult_population();
  void prepare_report_labeled();
  void prepare_report_matched();
  void prepare_report4(int year);   //partner ever

  //parametric distribution methods
  int get_uniform(int i, int j);
  double get_normal(double mean, double sd);
  double get_lognormal2();
  int get_lognormal1( double cdf[46], double rand3);

  
  double get_sexual_contacts_per_day() {
    return this->sexual_contacts_per_day;
  }
  double get_sexual_transmission_per_contact() {
    return this->probability_of_transmission_per_contact;
  }
  
private:
  
  double sexual_contacts_per_day;
  double probability_of_transmission_per_contact;
};

#endif // _FRED_SEXUAL_TRANSMISSION_NETWORK_H

