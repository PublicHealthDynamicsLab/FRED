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
//

#ifndef _FRED_HIV_Natural_History_H
#define _FRED_HIV_Natural_History_H

#include "Natural_History.h"
class Infection;

//*******************************************************
//*******************************************************
//******** debug settings

#define SCREEN 1    //this is an indicator: changing HIV infection to start with undiagnosed HIV, and not tr when undiagnosed. and implement screening rate

//#define TIME 100            //Number of years of model run //MINA CHECK : check for the role of this
//#define PATIENTS 1
#define CALIBRATE 0
#define GETSTATES 0
#define KIMTEST 0
#define VLCD4GRAPH 0													//kim_test2
#define RESOURCE_RICH 1
#define ONLY_CORE_EVENTS 0
#define TABLE_SPACE 400

//*******************************************************
//*******************************************************
//******** Treatment constants
#define PATIENT_STARTS_ON_HAART 0   //If value is 1, patient is initialized to be on haart at the start of the model.
#define MAX_REGS_MODEL 50
#define MAX_ARV_TYPES 6
#define DRUGS_IN_REG 3          //3 indicates triple-drug therapy
#define MAX_DRUGS_IN_CLASS 8
#define MAX_REGS 50             //No limit on number of regimens for resource rich
#define ARV_TYPES 6             //represents the number of classes: PI, NRTI, NNRTI (x2)
#define CD4_TREAT 350.0
//All patients will begin on this regimen
#define INITIAL_REG_DRUG1 NNRTI_EFAVIRENZ		//During normal runs, we will start pats on this reg
#define INITIAL_REG_DRUG2 NRTI_NONTAM
#define INITIAL_REG_DRUG3 NRTI_TAM
#define NUM_CLASS_COMBINATIONS 6      //The number of different possible class combinations
#define VL_DEC_PI_SINGULAR 1.84       //1.78 //=(1.12 * 100/63)
#define VL_DEC_PI_BOOSTED 2.68        //2.51	//=(1.58 * 100/63)
#define VL_DEC_EFAVIRENZ 3.09         //3.29	//=(2.07 * 100/63)
#define VL_DEC_NEVIRAPINE 2.22         //2.29	//=(1.44 * 100/63)
//Switch determines whether it is possible to develop mutations against a drug in the regimen that the patient has already developed resistance to.
#define CAN_GET_MUTS_TO_RES_DRUGS 0
//the VL threshold for regimen failure before and after the first regimen failure
#if CALIBRATE
#define VL_REGIMEN_FAILURE_1ST 3.7
#else
#define VL_REGIMEN_FAILURE_1ST 2.7
#endif
#define VL_REGIMEN_FAILURE_AFTER_1ST 3.7
//Possible regimen change triggers (for comparison)
#define T_CD4		1
#define T_VL		2
#define T_NESTED	3			//If CD4 failure criteria is met, then check VL
#define T_CLINICAL	4
//ratio of probability of mutations between drug classes
#define RATIO_PI_TO_NRTI	1.0		//.2
#define RATIO_NNRTI_TO_NRTI 1.0

#define MUT_PI_SINGULAR_START 0
#define MUT_PI_BOOSTED_START 0
#define MUT_NRTI_TAM_START 0
#define MUT_NRTI_NONTAM_START 0
#define MUT_NNRTI_EFAVIRENZ_START 0
#define MUT_NNRTI_NEVIRAPINE_START 0
#define COMP .62		//Scott says this is the compliance for the VACS Cohort. Use this value for calibration.  Go back to .75 after calibration.
#define VL_ADJUST 1.5  //kn changed for v2.0.  Was 3.0 in 1.5c.
//the following is obtained so that if we approach something at the rate of (1/3.13) per month, then
//after 6 months we'll be 90% of the way there
#define CD4_ADJUST_W 3.13  //3.0
//the following is obtained so that if we approach something at the rate of (1/13.51) per month, then
//after 30 months we'll be 90% of the way there
#define TOX_MULT_1ST_3_MONTHS	1.0		//there may be increased toxicity over the first three months (.25 years)of being on haart
//Setting this param to 1.0 makes no change to the toxicity
#define INTOL_MULTIPLIER		1.0		//if a patient has previously been intolerant, they're likely to be intol again
#define	PROB_CENSOR_INSTEAD_OF_FAIL 0	//Used only for resource poor calibration
//The proportion of time we decrement for ALL of the drugs in a patient's drug regimen vs. choosing one at random to decrement.
#define INTOLERANCE_DING_FOR_ALL .25

//Probability that mutation results in resistance
#define PMUTRES_PI_SINGULAR_EST .5
#define PMUTRES_PI_BOOSTED_EST .5
#define PMUTRES_NRTI_TAM_EST .5
#define PMUTRES_NRTI_NONTAM_EST .5
#define PMUTRES_NNRTI_EFAVIRENZ_EST .9
#define PMUTRES_NNRTI_NEVIRAPINE_EST .9

//Probability that mutation results in cross-resistance (to other drugs within class)
//All PIs are boosted except for Nelfinavir
#define PCROSSRES_PI_SINGULAR_EST 1.0
#define PCROSSRES_PI_BOOSTED_EST .24
#define PCROSSRES_NRTI_TAM_EST 1.0
#define PCROSSRES_NRTI_NONTAM_EST .48
#define PCROSSRES_NNRTI_EFAVIRENZ_EST 1.0
#define PCROSSRES_NNRTI_NEVIRAPINE_EST 1.0

//Probability of cross-resistance between different classes
#define PCROSSRESPI_BOOSTED_SINGULAR .05
#define PCROSSRESPI_SINGULAR_BOOSTED .07
#define PCROSSRESNRTI_TAM_NONTAM .02
#define PCROSSRESNRTI_NONTAM_TAM .08
#define PCROSSRESNNRTI_EFAV_NEVIR 1.0
#define PCROSSRESNNRTI_NEVIR_EFAV .875

#define PI_SINGULAR 0
#define PI_BOOSTED 1
#define NRTI_TAM 2
#define NRTI_NONTAM 3
#define NNRTI_EFAVIRENZ 4
#define NNRTI_NEVIRAPINE 5

//Regimen types used in function ChooseRegimen()
#define MAINTAIN_CURRENT_REG 0
#define NEW_REG 1
//The number of drugs the patient must be resistant to on all regimens before going on salvage
#define NUM_DRUGS_RES_FOR_PLATO 2

//*******************************************************
//*******************************************************
//******** HIV Disease Constants

//#define AVG_CD4 332.31
#define AVG_ACUTE_HIV 6.03
#define AVG_ACUTE_HIV_SD 1.52

#define AVG_HIV 4.07
#define AVG_AGE_SD 9.28
#define AVG_CD4_SD 253.81
#define AVG_HIV_SD 1.11
#define MAX_CD4 1500
#define MAX_HIV 8
#define MUT_RATE 0.18 //0.18 per year equals 0.015 per month
#define ASSOC_COMP .9
#define FACTOR 3.3		//Used in determining the mutation rate based on viral load
#define START_WITH_AIDS			0		//The fraction of patients starting with AIDS
#define TRIG					T_VL
#define COINFECTION_MULT		1.0
#define MORTMULTHIV				1.0
//The probability of death from HIV is adjusted for whether the pat has AIDS
//Source:  AIDS 2007, 21:1185-1197, 2.33 (2.01-2.69) 95% confidence interval
//The NON_AIDS_ADJUST was calculated based on a .310 initial AIDS rate in the CHORUS
//cohort.  Source:  Journal of Clinical Epidemiology 57 (2004) 89-97.
#define AIDS_ADJUST				2.33
#define NON_AIDS_ADJUST			.401
//the probability of an AIDS defining event occurring is this factor times the probability of
//death from the HIV lookup table.  (Source: ARTCC - Antiretroviral Therapy Cohort Collaboration)
#define AIDS_EVENT_MULTIPLIER	3.0
#define WEIGHT_PAT_LUCK		.8
#define WEIGHT_ROUND_LUCK	.2
#define LUCK_MULTIPLIER	0.5						//to control the effect of patient luck on the CD4within
#define	VACS_HIV_NEG_MORT_RATE  0.01526	//The mortality rate of HIV negative patients in the VACS cohort
#define VACS_HIV_NEG_MORT_MULT	2.5		//We doubled the HIV negative mortality during calibration to
//bring the survivals in line

//*******************************************************
//*******************************************************
//******** HIV Demographics constants
#define AVG_AGE 50.0  //45.87      //mina: I use seeding by age, this is now only used in some output files. DOes not affect results
//#define MAX_AGE 80

//*******************************************************
//*******************************************************
//********* HIV Cost constants
//These costs are from Kara Wools-Kaloustian, per her e-mail dated 11/18/2009.
//At AMPATH the cost of a CD4 test is $11.20, and VL is $70.
#define TESTCOSTVL 70.00
#define TESTCOSTCD4 11.20

#define COST_OF_CARE_POOR 287.28			//Annual cost of care for Resource Poor
#define HOSPITAL_COST_POOR 390.27			//Annual costs for hospitalization for Resource Poor

#define COST_REG1_POOR 15.78				//Monthly costs of reg1;
#define COST_REG2_POOR 113.43				//Monthly costs of reg2;
#define COST_REG3_POOR 255.60				//Monthly costs of reg3:  for sensitivity $113.43, $255.60, $1022.41

//*******************************************************
//*******************************************************
////for sensitivity analyses
#define DVLHAART 0 //baseline: 0 //-1, 1
#define DCD4NOHAART 0 //baseline: 0 //-200, 200
//#define DCD4HAART	0 //baseline: 0 //-200, 200
#define ADJMUTRES 1 //baseline: .5 //0, 1
#define ADJCROSSRES 1 //baseline: 1 //0, 10
//#define MORTMULTNOHIV 1 //baseline: 1//.1, 10
//#define ADJMUTRES = 1.0;
#define REG_INFO_FOR_SENSITIVITY	0
#define MONITOR_INTERVAL		3		//in units of months
//#if !CALIBRATE
//#else  //settings used only for calibration		//To replicate the VACS calibration data, we will start pats on PI_S
//#define INITIAL_REG_DRUG1 PI_SINGULAR
//#define INITIAL_REG_DRUG2 NRTI_NONTAM
//#define INITIAL_REG_DRUG3 NRTI_TAM
//#define NUM_CLASS_COMBINATIONS 8		//The number of different possible class combinations
//#endif

//output settings - all to go file1

#define TIME_TO_TREATMENT_FAILURE_OF_REGIMENS_1_2_3 0
#define CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_THERAPY 0
#define CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE 0
#define ANNALS_REGS_MUTS 0
#define ACTIVE_HAART_INFO 0
#define REASONS_FOR_REG_CHANGES 0
#define MORTALITY_1_3_5_YEARS_AFTER_THERAPY 0
#define DEBUG_NUM_MUTS 0
#define DEBUG_AIDS 0
#define MONITORING_PAPER 0

//Used in putPatOnReg
#define NONE_AVAILABLE -999

#define DISC_RATE .03
#define EPSILON .00001
#define HAART_TOX 1.0  //toxicity
#define STOP_REG				.1		//TEST.06		//rate is per year	(used to be 0.15/month)

#define DELTA_UTILITY_WITH_HAART -.053
#define UTILITY_CD4_LESS_THAN_100 .81
#define UTILITY_CD4_LESS_THAN_200 .87
#define UTILITY_CD4_ABOVE_200 .94

#define VL 1
#define CD4 2
#define HIV 0
#define AGE 1
#define CENSORED 2
#define AGE_MALE 3
#define AGE_FEMALE 4

//used for breaking down costs
#define TOTAL 0
#define DRUG 1
#define LAB 2
#define CARE 3
#define HOSP 4

//Allowable behaviors (to help debug)
#define USE_PERLIN_NOISE 1
#define VRT	0
#define BATCH 0

#define DAY 1
#define MONTH 2
#define CYCLE DAY				//length of cycle can be MONTH or DAY
#define DAYS_PER_YEAR			365

#if CYCLE == DAY
#define CYCTIME				1.0/DAYS_PER_YEAR
#define CYCLES_PER_YEAR		DAYS_PER_YEAR
#define CYCTIME_IN_MONTHS	12.0/DAYS_PER_YEAR	//CYCTIME_IN_MONTHS is the number of months per cycle
//For daily, .032877 (one day = .032877 months)
#define CYCLES_PER_MONTH	DAYS_PER_YEAR / 12.0
#else
#define CYCTIME				1.0/12.0			//CYCTIME is the number of months per cycle
#define CYCLES_PER_YEAR		12.0
#define CYCTIME_IN_MONTHS	1.0					//	One cycle is equal to one month
#endif

#define MALE		.98
#define FEMALE		.02
//For VRT
//#define MAX_SEED_INIT 8
//#define MAX_SEED_CYCLE 510
//*******************************************************
//*******************************************************
//*******************************************************
//*******************************************************

class HIV_Natural_History : public Natural_History {

public:
  
  static long int seed;   // = initial_seed = 60067 in Kim's code;      //mina

  HIV_Natural_History();

  ~HIV_Natural_History();

  void setup(Condition *condition);

  void get_parameters();

  void update_infection(int day, Person* host, Infection *infection);

  double get_probability_of_symptoms(int age);

  int get_latent_period(Person* host);
  
  int get_duration_of_infectiousness(Person* host);

  int get_duration_of_immunity(Person* host);

  int get_incubation_period(Person* host);

  int get_duration_of_symptoms(Person* host);

  bool is_fatal(double real_age, double symptoms, int days_symptomatic);

  bool is_fatal(Person* per, double symptoms, int days_symptomatic);
  
  /************ BEGIN MINA  ***************/

  double Get_Median(double arr[], int);
  void QuickSort(double arr[], int, int);
  void Swap( double[], int, int );
  
  double get_HIV_table(int row, int col);
  double get_HIV_AGE_Male_table(int row, int col);
  double get_HIV_AGE_Female_table(int row, int col);
  
  int get_num_mort_entries(){
    return this->hiv_num_mort_entries;
  }
  int get_num_mort_male_entries(){
    return this->hiv_num_male_entries;
  }
  int get_num_mort_female_entries(){
    return this->hiv_num_female_entries;
  }
  
  double CD4_at_Diagnosis();
  double assign_initial_CD4();
  int update_hiv_stage(double cd4);
  
  void  hiv_salvage(double a, int b, int c);
  void  hiv_fillUnifSim();
  int   RandomNumber(int upperRange);
  
  static double AVG_CD4;

  //Number of drugs in each class (resource rich!!)
  static int NUM_PI_SINGULAR;	//Nelfinavir is the only unboosted PI
  static int NUM_PI_BOOSTED;
  static int NUM_NRTI_TAM;
  static int NUM_NRTI_NONTAM;
  static int NUM_NNRTI_EFAVIRENZ;
  static int NUM_NNRTI_NEVIRAPINE;

  /************ END MINA  **********/

private:
  
  int hiv_num_mort_entries;
  int hiv_num_male_entries;
  int hiv_num_female_entries;
  
  int num_exhausted_clean_regs;
  int numDeadYear[22];
  int numDeadYearHaart[22];
  double total_time_on_haart_before_salvage; //Making this variable a double because at 1M runs, the value it holds exceeds the space allocated for an integer
  int total_num_regs_before_salvage, total_res_drugs_before_salvage;
  
  double hiv_mort_table[400][2];    //mina
  double hiv_male_age_table[400][2];
  double hiv_female_age_table[400][2];
  
  double mean_reg_time[3], median_reg_time[3];				//used in TIME_TO_TREATMENT_FAILURE
  double total_completed_reg_cycles[3];						//used in KAPLAN_MEIER_CURVE_OF_TIME_TO_TREATMENT_FAILURE_REG_1_2_3
  int numPeopleCompletedReg[3];   //mina
  //for debugging the number of mutations with DEBUG_NUM_MUTS
  
  
  //for VRT
  //double unifInit[MAX_SEED_INIT];
  double unifSim[510];     //mina /*MAX_SEED_CYCLE = 510*/
  //  long int MAX_PAT_RNS;
  //  int pat_rns_used;
  //  int kludge_counter;   //mina
  //  int max_seed_cycle;   //mina
  
  int numStartedSalvage;						//from hiv.h				//Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE
  int numReached2DrugsResOnSalvage;			//from hiv.h				//Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE
  
  //double totalCD4=0;
  //double meanCD4;
  //double arrayCD4[PATIENTS];
  
  //used for DEBUG_AIDS
  //int totalAIDSOrDeath;
  
  //used for DEBUG_NUM_MUTS
  //int totalResDrugsAtDeath;
  
  //variable for LOOP analysis
  int removeNonHIVMort; // = 0;		//default to NOT removing it!
  
  //variables for data for MARK_ROBERTS_AHRQ
  /*
   int numVLsup;			//number of patients with viral load supressed
   double tot_CD4elev;		//total of all patients CD4 elevation from baseline
   int numAIDS;			//number of patients with AIDS
   int tot_regs;		//total regs over all pats
   int tot_treatFail;	//total patients with treatment failure of initial reg
   int tot_alive;		//total alive at certain time
   int pats_alive;		//num pats alive at time "timepoint"
   int numVLFail;		//num patients who experience VL failure
   int numVLFailWithMuts;	//the number of patients with virological failure and one or more res muts
   bool VLFail;
   int tot_intolFirstReg;	//total patients who changed first reg due to intol
   
   
   double clinDatPoorReg[3][TIME*(int)CYCLES_PER_YEAR];		//For reading in clinical data to help calibrate
   int num_entries_poor_reg[3];
   
   //for VRT
   double unifInit[MAX_SEED_INIT];
   double unifSim[MAX_SEED_CYCLE];
   long int MAX_PAT_RNS;
   int pat_rns_used;
   
   double CD4within_mult;
   int SurvivalPlot[3][TIME * (int)CYCLES_PER_YEAR];				//3D array: [0] is for hiv deaths, [1] is age deaths, [2] is censored
   double ChangeInCD4_1_3_5_YearsIntoTherapy[3][PATIENTS];
   double ChangeInVL_1_3_5_YearsIntoTherapy[3][PATIENTS];
   double CD4_1_3_5_YearsIntoSalvage[3][PATIENTS];
   double VL_1_3_5_YearsIntoSalvage[3][PATIENTS];
   double CD4atStartOfSalvage[PATIENTS];
   double VLatStartOfSalvage[PATIENTS];
   double cyclesToCompleteReg[3][PATIENTS];					//Kim added.  Used in calculating time to treatment failure for regs 1-3.
   double qalmtimes[PATIENTS];
   
   int numPeopleCompletedReg[3];								//Added by Kim
   int numDeadYear[22];
   int numDeadYearHaart[22];
   
   int num_exhausted_clean_regs;								//Used in TIME_RES_DRUGS_BEFORE_SALVAGE
   double YearlyCD4[TIME][PATIENTS];							//Used in YEARLY_CD4_AND_YEARLY_CHANGE_IN_CD4_AFTER_START_OF_THERAPY
   //double VLatYear5IntoHaart[PATIENTS];						//Used in MONITORING_PAPER
   //double VLatYear10IntoHaart[PATIENTS];						//Used in MONITORING_PAPER
   int numAliveAtYearIntoHaart[TIME];							//Used in YEARLY_CD4_AND_YEARLY_CHANGE_IN_CD4_AFTER_START_OF_THERAPY AND MONITORING_PAPER
   int year;													//Used in YEARLY_CD4_AND_YEARLY_CHANGE_IN_CD4_AFTER_START_OF_THERAPY
   int years;													//Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_THERAPY (and SALVAGE)
   int numStartedSalvage;										//Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE
   int numReached2DrugsResOnSalvage;							//Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE
   //int totalNumRegs5yearsIntoHaart_ofPatsAlive5YrsIn;							//Used in MONITORING_PAPER
   //int totalNumRegs10yearsIntoHaart_ofPatsAlive10YrsIn;						//Used in MONITORING_PAPER
   //int totalNumMuts5yearsIntoHaart_ofPatsAlive5YrsIn;							//Used in MONITORING_PAPER
   //int totalNumMuts10yearsIntoHaart_ofPatsAlive10YrsIn;							//Used in MONITORING_PAPER
   
   
   double total_time_on_haart_before_salvage;					//Making this variable a double because at 1M runs, the value it holds exceeds the space allocated for an integer
   double maxMonths;
   int vl_monitor_interval;
   int cd4_monitor_interval;
   double CD4_previous;
   int eventIndicator;
   
   
   double vacs_hiv_neg_mort[NUM_YEARS_VACS_HIV_NEG];									//This is the mortality calculated from VACS hiv-negatives
   double mean_reg_time[3], median_reg_time[3];				//used in TIME_TO_TREATMENT_FAILURE
   double total_completed_reg_cycles[3];						//used in KAPLAN_MEIER_CURVE_OF_TIME_TO_TREATMENT_FAILURE_REG_1_2_3
   
   
   int vl_thresh;															//for MONITORING_PAPER loop
   int numAvailableRegimens; //NUM_CLASS_COMBINATIONS;						//only used for resource poor for MONITORING_PAPER
   int poorCanGoBackOnPreviousRegs; //POOR_CAN_GO_BACK_ON_PREVIOUS_REGIMENS;
   
   int numAlive_1_3_5_YearsIntoHaart[3];  //Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_THERAPY
   int numAlive_1_3_5_YearsIntoSalvage[3];  //Used in CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE
   //double Util_dec;
   ofstream senseFile;
   */

};

#endif
