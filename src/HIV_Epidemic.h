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
//  Created by Mina Kabiri on 6/1/16.
//
 

#ifndef _FRED_HIV_EPIDEMIC_H
#define _FRED_HIV_EPIDEMIC_H

#include "Epidemic.h"
#include "HIV_Natural_History.h"
#include "Sexual_Transmission_Network.h"

#define TIME 100
#define SURV_RATES 1

class Condition;

class HIV_Epidemic : public Epidemic {
  
public:
  HIV_Epidemic(Condition * condition);
  ~HIV_Epidemic() {}
  void setup();
  void update(int day);
  void report_condition_specific_stats(int day);
  void end_of_run();
  void prepare();
  void import_aux1(char sex, int age_bundle, int day, double radius, fred::geo lat, fred::geo lon);
  int import_aux2(vector<Person*> people, int imported_HIV_requested, int imported_AIDS_requested, double radius, int day, int searches_within_given_location);
  // void annual_update(Person* person, int day);  //virtual in Epidemic.h
  
  
  
  /////// STATIC MEMBERS
  
  // defining array for HIV incidence information for each year based on age group. set for 100 years for now.
  static int HIV_inc_agegroup[100][Sexual_Transmission_Network::ROW_age_group];
  static int HIV_inc_f_agegroup[100][Sexual_Transmission_Network::ROW_age_group];
  static int HIV_inc_m_agegroup[100][Sexual_Transmission_Network::ROW_age_group];

  static int HIV_stage_f[100][3];
  static int HIV_stage_m[100][3];
  

  static double HIV_screening_prob;
  
  // ***** Initial HIV Prevalence *****//
  // AGE distribution
  static double HIV_prev_agegroup0;   //13-24
  static double HIV_prev_agegroup1;   //25-34
  static double HIV_prev_agegroup2;   //35-44
  static double HIV_prev_agegroup3;   //45-54
  static double HIV_prev_agegroup4;   //55+
  
  // Disease stage CD4
  static double HIV_prev_diseaseStage_F1;
  static double HIV_prev_diseaseStage_F2;
  static double HIV_prev_diseaseStage_F3;
  
  static double HIV_prev_diseaseStage_M1;
  static double HIV_prev_diseaseStage_M2;
  static double HIV_prev_diseaseStage_M3;
  
  static bool CONDOM_USE;
  static double DIAGNOSED_RATE;
  
  static int imported_HIV_requested_male[5];
  static int imported_HIV_requested_female[5];
  
  static int imported_AIDS_male[5];
  static int imported_AIDS_female[5];
  
 
  //METHODS
  
  void increment_total_qa_surv_time(double qa_time, double increment);
  void increment_total_disc_surv_time(double increment);
  void increment_total_cost(double cost);
  void increment_total_disc_cost(double cost, double increment);
  void increment_total_drug_cost(double drug_cost);
  void increment_total_drug_disc_cost(double drug_cost, double increment);
  void increment_total_care_cost(double care_cost);
  void increment_total_care_disc_cost(double care_cost, double increment);
  void increment_total_hospital_cost(double hospital_cost);
  void increment_total_hospital_disc_cost(double hospital_cost, double increment);
  
  void increment_total_hiv_deaths();
  void increment_total_age_deaths();
  void increment_hivdeaths(int cyclenum, int cycles_per_year);
  void increment_agedeaths(int cyclenum);
  void increment_total_AIDSorDeath();
  void increment_total_hiv_surv_time(double duration);
  
  void add_element_to_hivdeaths_today();
  void increment_hivdeaths_today(int today);
  void add_element_to_agedeaths_today();
  void increment_agedeaths_today(int today);
  
  void increment_hiv_incidence(Person* infectee);
  
  int time_to_care();
  
  //**********
  
  double start_cd4;
  
private:
  
  double total_qa_surv_time;
  double total_qa_disc_surv_time;
  double total_disc_surv_time;
  double total_surv_time;
  double mean_surv_time, mean_disc_surv_time, mean_qa_surv_time, mean_qa_disc_surv_time;
  
  double total_cost;
  double total_disc_cost;
  double total_drug_cost;
  double total_drug_disc_cost;
  double total_care_cost;
  double total_care_disc_cost;
  double total_hospital_cost;
  double total_hospital_disc_cost;
  double total_lab_cost;
  double total_lab_disc_cost;
  
  double mean_cost;
  double mean_disc_cost;
  double mean_hospital_cost;
  double mean_disc_hospital_cost;
  double mean_disc_lab_cost;
  double mean_drug_cost;
  double mean_care_cost;
  double mean_disc_care_cost;
  double mean_disc_drug_cost;
  double mean_lab_cost;
  
  int	total_hiv_deaths;
  int	total_age_deaths;
  int total_censored;
  int	numDeaths;
  
  long int total_muts_all_pats, total_res_muts_all_pats;
  int TotalPatsRes1st1years, TotalPatsRes1st5years;
  int totalAIDSOrDeath;   //mina  used for DEBUG_AIDS
  
  int total_sim_years;
  
  //double qamedian[PATIENTS];  // mina check
  vector <int> hivdeaths;   //int hivdeaths[TIME];
  vector <int> agedeaths;   //int agedeaths[TIME];
  
  double start_with_aids = START_WITH_AIDS;
  double AIDSrate_less200, AIDSrate_greater200;
  //double start_cd4 = HIV_Natural_History::AVG_CD4;
  
  static FILE *hiv_file1;
  static FILE *hiv_incidence;
  static FILE *hiv_prevalence;
  static FILE *hiv_prevalence_age_group;
  
};

#endif
