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
// File: HIV_Infection.h
//

#ifndef _FRED_HIV_INFECTION_H
#define _FRED_HIV_INFECTION_H

#include "HIV_Natural_History.h"  
#include "Population.h"
#include "math.h"

#define CD4_OUTPUT_EXCEL_DAY  1

#define NEVER (-1)

class Condition;
class Person;

class HIV_Infection {
  
public:
  
  HIV_Infection(int _condition_id, Person* _host);
  
  ~HIV_Infection() {}
  
  void update(int day);
  
  void setup();
  double get_infectivity(int day);
  double get_susceptibility(int day);
  double get_symptoms(int day);
  bool is_fatal(int day);
  
  /****************** MINA BEGIN ******************/
  typedef struct
  {
    int drugNum;
    int classNum;
    bool res;
    bool intol;
  }drug;
  
  drug* getDrugPtr() const;
  drug getDrugObj() const;
  
  drug drugs[MAX_ARV_TYPES][MAX_DRUGS_IN_CLASS];		//the array of all drugs
  
  //methods
  
  void setPatIndividualDrugCompliance();
  void setup1(Person* host);
  void setup2(Person* host);
  int RandomNumber(int upperRange);
  void Process_Initial_Mutations();
  void checkForCrossResAndCrossClassRes(int mutType);
  void printNumRemaining();
  
  void process_patient1(int today, int exposure_date);
  void process_patient2(int today, int exposure_date);
  void process_patient3(int today, int exposure_date);
  void process_patient4(int today, int exposure_date);
  
  void start_haart(int today);
  bool notEnoughDrugsForInitialReg();
  void Get_Num_Each_Type_In_Array(int numInArray, int drugArray[], int numEachType[]);
  void printDrugsInReg();
  void getVLdec();
  void setNextMonitorCycle(int today);
  void Change_Regimen(int today);
  int ChooseRegimen_Rich();
  int ChooseRegimen_Poor();
  int countResDrugs();
  int numNotResOrIntol(int type);
  int numNotIntol(int type);
  int numNotRes (int type);
  void putPatOnReg(int *type);
  int giveMeBestDrugInClass(int type,  int numAssignedSoFar);
  double Utility();
  void Get_Compliance();
  double addNoise (double input, int type, int today);
  int Get_Category(int type);
  void getCD4regressionideal(int today);
  double linearInterpolate(double x0, double y0, double x1, double y1, double x);
  double getVLTerm();
  double Get_HIV_Death_Rate();
  double HIV_Lookup(bool haart);
  double Get_Rate(/*double table[TABLE_SPACE][2], int entries,*/int type, double lookup);
  void Check_For_Mutations();
  void decrementForIntolerance();
  bool metTrigger(int trigger);
  double Cost(int whichCost);
  
  void Process_Death(int type, int cyclenum);
  double get_CD4_real(int k);
  double get_VL_real(int k);
  
  void getInitialAIDSRates();     //add back if we want to initialize some AIDS on day 0
  //  void graph_trajectories(int cyclenum);
  
  //  double PerlinNoise1D(double x,double alpha,double beta,int n);    //perlin
  //  double noise1(double arg, int start);   //perlin
  //  void init(void);    //perlin
  
  //string createConstantsHeader(void);
  
  double Max(double x, double y) { return x > y ? x: y; }
  double Min(double x, double y) { return x < y ? x: y; }
  
  
  bool break_process_patient2;      //mina added
  bool break_process_patient3;      //mina added
  
  double get_CD4_count(){
    return this->CD4real;
  }
  
  double get_VL_count(){
    return this->VLreal;
  }
  
  void set_survival_days_alive(){
    this->survival_days_alive = days_post_exposure;
  }
  
  int get_survival_days_alive(){
    return this->survival_days_alive;
  }

  void set_diagnosed(){
    this->diagnosed = true;
  }
  
  bool get_diagnosed(){
    return this->diagnosed;
  }
  
  double get_VL_Fred(){
    //added by Mina- March 2016: VL_fred is the viral load used in HIV transmission in FRED. The scale of VLreal in Braithwaite code is different from the actual viral load categories. Hence I convert them here: VLreal is log 10 of viral load
    this->VL_fred = pow(10, this->VLreal);
    return VL_fred;
  }
  
  void set_has_AIDS(){
    this->has_AIDS = true;
  }
  
  void set_HIV_stage(int stage){
    this->HIV_stage = stage;
  }
  
  int get_HIV_stage(int stage){
    return this->HIV_stage;
  }
  
  void print_infection();

private:
  
  Person* host;
  Condition* condition;
  int condition_id;

  double rVLreal;     //mina: moved to HIV_infection from natural history
  double rCD4within_delta_real;
  
  bool haart;					//indicates if pat is on haart or not
  bool started_haart;
  int quit_haart;
  int numreg;
  bool exhausted_clean_regs;	//indicates if patient is resistant to all possible regimens
  int counterreg;				//gives number of cycles on this reg
  bool has_AIDS;				//bool indicating whether an AIDS defining event has ever occurred
  bool AIDSeventReg;			//bool indicating whether an AIDS defining event has occured during the current regimen
  int num_regs_before_salvage;		//used for REG_INFO_FOR_SENSITIVITY
  int num_res_drugs_before_salvage;	//used for REG_INFO_FOR_SENSITIVITY
  double maxCD4onReg;						//used for cd4 reg change trigger
  int mutCount;				//The number of mutations a patient has, regardless of resistance
  bool reached2DrugsResOnSalvage;			//"  "  Says whether patient was ever stuck with two resistant drugs on salvage (no better regimen and on Plato for the long slide down)
  bool historyOfIntolerance;		//1 if patient has ever been intolerant, else 0
  int nextMonCycle;				//Number of cycles from now to check VL or CD4 again
  double cd4_treat; //= CD4_TREAT;
  
  double start_age;
  double CD4baseline;
  double HIVbaseline;
  double comp;
  //    double censorRate;			//used only for calibrating resource poor
  double pCensor;				//used only for calibrating resource poor
  double deathrateHIV;
  double deathrate_nonHIV;
  double pDieHIV;
  double pDieAge;
  double pAIDS;
  double assoc_comp;
  //    double age;
  double pNocomp;
  double pMutate;
  double adjmutrate;
  double vl_fract;
  double luck1;
  double luck2;
  double diligence;
  double individal_drug_comp;
  double pstopreg;
  double CD4Peak;				//The maximum CD4 ever reached on ART
  //    int dieHIV;					//booleans to indicate if patient died HIV or by Age (next)
  //    int dieAge;
  //    int cyclenum;				//I use this to mean number of times through "Cycle" branch, however
  //DM increases this anytime it goes through a branch such as OnePI, DR, etc.
  //    int counterreg_limited;		//for calculating the CD4within, we want to limit counterreg at 7 months
  int haart_start_time;
  int haart_exhausted_clean_regs_time;
  double vl_regimen_failure;	//the VL threshold for regimen failure
  int monitor_interval; //MONITOR_INTERVAL;
  double delta;
  double haart_tox; //HAART_TOX;		//Kim added.
  
  double CD4real;				//gives the real CD4 level that accounts for non-inst change to the cd4 ideal.
  //set up so patient's CD4 moves a fraction of the way to the CD4 ideal
  //    double CD4deltaNoHAART;
  double CD4real_before_noise;//the smooth CD4real - we add noise to make this look more natural
  double CD4offreal;
  double CD4regressionideal;	//the CD4 as calculated by Joyce's regression equation
  double CD4regressionreal;
  
  double CD4weightedForComp;
  
  double CD4Mellors;			//the CD4 off haart as calculated by the Mellors equation
  double VLreal;
  double VLreal_before_noise;
  double VLideal;
  double VLdec;
  int offset1, offset2;
  double CD4_plato;			//used in calculating CD4
  
  double VL_fred;     //added by Mina. This is the Viral load we use for transmission in FRED
  
  
  //  int total_num_regs_before_salvage, total_res_drugs_before_salvage;
  bool hasResistance;				//says if pat has developed resistance to any drugs whatsoever
  //  double start_age; //= AVG_AGE;
  double start_cd4; //= AVG_CD4;
  double start_hiv; //= AVG_HIV;
  
  
  
  //double avgAge_SD; //= AVG_AGE_SD;
  double avgHIV_SD; //= AVG_HIV_SD;
  double avgCD4_SD; //= AVG_CD4_SD;
  
  double stop_reg; // = STOP_REG;
  double mortMultHIV; // = MORTMULTHIV;
  double intolMultiplier; // = INTOL_MULTIPLIER;
  
  //   int mut_pi_singular_start = MUT_PI_SINGULAR_START;
  //   int mut_pi_boosted_start = MUT_PI_BOOSTED_START;
  //   int mut_nrti_tam_start = MUT_NRTI_TAM_START;
  //   int mut_nrti_nontam_start = MUT_NRTI_NONTAM_START;
  //   int mut_nnrti_efavirenz_start = MUT_NNRTI_EFAVIRENZ_START;
  //   int mut_nnrti_nevirapine_start = MUT_NNRTI_NEVIRAPINE_START;
  
  double start_with_aids; //START_WITH_AIDS;
  
  double AIDSrate_less200, AIDSrate_greater200;
  //int cycles_since_PLATO_start;
  double intol_rate;
  //double max_possible_CD4withinideal;
  
  int regChangeIntol[3];										//Counts the number of reg changes due to intolerance
  int regChangeFail[3];											//Coun the number of reg changes due to reg failure
  //for VL/CD4 trajectories
  bool regChangeFailFlag;
  bool regChangeIntolFlag;
  
  //bool regFailedLastCycle, patIntolLastCycle;					//Used to provide more info to the trajectory graphs
  
  //values at start of each regimen - used for CD4within
  double AgeRegBaseline;
  double CD4RegBaseline;
  double CD4atStartOfHaart;
  double VLatStartOfHaart;
  
  double qa_time;    //mina
  double qamedian;    //mina added for each infection
  double cost, total_cost, mean_cost, total_disc_cost, mean_disc_cost; //,individual_cost[PATIENTS];
  double drug_cost, total_drug_cost, mean_drug_cost, total_drug_disc_cost, mean_disc_drug_cost;
  double total_lab_cost, mean_lab_cost, total_lab_disc_cost, mean_disc_lab_cost;
  double hospital_cost, total_hospital_cost, mean_hospital_cost, total_hospital_disc_cost, mean_disc_hospital_cost;
  double care_cost, total_care_cost, mean_care_cost, total_care_disc_cost, mean_disc_care_cost;
  
  
  double pmutres_pi_singular_est; // = PMUTRES_PI_SINGULAR_EST;   //MOVE TO .CC
  double pmutres_pi_boosted_est; // = PMUTRES_PI_BOOSTED_EST;
  double pmutres_nrti_tam_est; // = PMUTRES_NRTI_TAM_EST;
  double pmutres_nrti_nontam_est; // = PMUTRES_NRTI_NONTAM_EST;
  double pmutres_nnrti_efavirenz_est; // = PMUTRES_NNRTI_EFAVIRENZ_EST;
  double pmutres_nnrti_nevirapine_est; // = PMUTRES_NNRTI_NEVIRAPINE_EST;
  
  double pcrossres_pi_singular_est ; //PCROSSRES_PI_SINGULAR_EST;
  double pcrossres_pi_boosted_est	; //PCROSSRES_PI_BOOSTED_EST;
  double pcrossres_nrti_tam_est ; //PCROSSRES_NRTI_TAM_EST;
  double pcrossres_nrti_nontam_est ; //PCROSSRES_NRTI_NONTAM_EST;
  double pcrossres_nnrti_efavirenz_est ; //PCROSSRES_NNRTI_EFAVIRENZ_EST;
  double pcrossres_nnrti_nevirapine_est ; //PCROSSRES_NNRTI_NEVIRAPINE_EST;
  
  double pcrossres_piboosted_singular ; //PCROSSRESPI_BOOSTED_SINGULAR;
  double pcrossres_pisingular_boosted ; //PCROSSRESPI_SINGULAR_BOOSTED;
  double pcrossres_tam_nontam ; //PCROSSRESNRTI_TAM_NONTAM;
  double pcrossres_nontam_tam ; //PCROSSRESNRTI_NONTAM_TAM;
  double pcrossres_efav_nevir ; //PCROSSRESNNRTI_EFAV_NEVIR;
  double pcrossres_nevir_efav ; //PCROSSRESNNRTI_NEVIR_EFAV;
  
  //  double comp; //COMP;
  double mut_rate; //MUT_RATE;
  //  double assoc_comp; //ASSOC_COMP;
  double factor; //FACTOR;
  double adjmutres; //ADJMUTRES;
  double adjcrossres; //ADJCROSSRES;
  int arv_types;					//the number of different drug classes
  int patient_starts_on_haart;// = PATIENT_STARTS_ON_HAART; // Indicates whether patient will be initialized to be on HAART when patient is created.
  int trig; // = TRIG;											//The type of regimen change trigger to use (values in hiv.h)
  int patRegCombo;	//Patient may return to a previous reg, if so, we need to know
  //if it was reg 0, reg 1, reg 2, etc. for costing purposes
  //This value refers to the reg number in the list of regs in ChooseRegimen_Poor()
  
  
  double mut_rate_pi;
  double mut_rate_nrti;
  double mut_rate_nnrti;
  
  int num_in_class[MAX_ARV_TYPES];	//a counter for the number of drugs in each drug class
  int num_res[MAX_ARV_TYPES];		//a counter for the number of drugs in each class the patient is resistant to
  int cyclesOnReg[MAX_REGS_MODEL];    // the number of cycles on a given therapy 0 = haart1, 1 = haart2, 2 = haart3, etc.
  char className[MAX_ARV_TYPES][20];
  int mut_arv[MAX_ARV_TYPES];		//gives number of mutations against each arv type
  //    int num_in_reg[MAX_ARV_TYPES];	//a counter for the number of drugs in each class
  double pmutres[MAX_ARV_TYPES];
  double pcrossres[MAX_ARV_TYPES];
  int crossClassType[MAX_ARV_TYPES];
  double pCrossClassRes[MAX_ARV_TYPES];
  double mutRateClass[MAX_ARV_TYPES];	//Given the overall mutation rate, this is the probability of a mutation occurring in this class
  
  
  //    int res_drugs_at_reg_start[DRUGS_IN_REG];//indicates if pat is resistant to drug 1, 2, 3 at the start of the reg
  
  int nc[DRUGS_IN_REG];		//indicates if pat is compliant to drug 1, 2, 3
  //    double pRes_drugs[DRUGS_IN_REG];
  int total_res_after_initial_muts[ARV_TYPES];
  int numEachTypeinReg[ARV_TYPES];
  
  //double real_CD4_year[TIME];    //new added
  double real_CD4_day[365];    //new added
  double real_VL_day[365];    //new added
  
  int days_post_exposure;
  
  bool fatal_hiv;
  //    int hivlookup;				//gives index for looking up hiv mortality rate in table
  int CD4cat;
  int VLcat;
  
  int totalresnc;				//total number of drugs being taken that virus is resistant to or pat is not compliant to.
  int totalres;				//total number of drugs in reg that virus is resistant to
  int totalintol;				//total number of drugs in reg that pat is intol to
  bool lastRegFailed;
  
  double testCostVL; //TESTCOSTVL;
  double testCostCD4; //TESTCOSTCD4;
  //   double cost_of_care_poor; //COST_OF_CARE_POOR;        //LL added
  //   double hospital_cost_poor; //HOSPITAL_COST_POOR;      //LL added
  //   double cost_reg1_poor; //COST_REG1_POOR;                  //LL added
  //   double cost_reg2_poor; //COST_REG2_POOR;                  //LL added
  //   double cost_reg3_poor; //COST_REG3_POOR;
  
  double vl_regimen_failure_1st; //VL_REGIMEN_FAILURE_1ST;					//for MONITORING_PAPER
  double vl_regimen_failure_after_1st; //VL_REGIMEN_FAILURE_AFTER_1ST;		//for MONITORING_PAPER
  
  drug *structDrugPtr;
  drug structDrugObj;
  
  drug *reg[3];					//The patient's regimen.  The pointers point to the drugs in the drug array
  int init_reg[3];				//holds the drug types of the initial regimen
  
  //****** added by Mina, these do not exist in Braithwait code
  
  int survival_days_alive;
  bool diagnosed;
  int start_haart_day;
  bool acute;
  int end_acute; //day that acute infection ends
  int HIV_stage;
};

#endif // _FRED_INFECTION_H

