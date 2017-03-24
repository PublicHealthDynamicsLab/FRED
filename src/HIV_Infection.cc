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
// File HIV_Infection.cc
//

#include "HIV_Infection.h"
#include "Condition.h"
#include "Global.h"
#include "Person.h"
#include "Utils.h"
#include "HIV_Epidemic.h"
#include "perlin.h"
#include "perlin.cc"
#include "Random.h"
#include "Date.h"

// static variables
FILE * kim_test = NULL;
FILE * mina_test = NULL;
FILE * CD4_excel = NULL;
FILE * VL_excel = NULL;

HIV_Infection::HIV_Infection(int _condition_id, Person* _host) {
  
  this->host = _host;
  this->condition_id = _condition_id;
  this->condition = Global::Conditions.get_condition(this->condition_id);

  this->start_age = -1.0;
  this->start_cd4 = -1.0;
  this->start_hiv = -1.0;
  this->cd4_treat = -1.0;
  
  //this->avgAge_SD = -1.0;
  this->avgHIV_SD = -1.0;
  this->avgCD4_SD = -1.0;
  
  arv_types = 0;
  CD4baseline = -1.0;
  HIVbaseline = -1.0;
  
  num_regs_before_salvage = 0;
  num_res_drugs_before_salvage = 0;
  
  haart = 0;					//indicates if patient is on haart or not
  started_haart = false;
  quit_haart = 0;
  
  numreg = 0;
  exhausted_clean_regs = false;	//indicates if patient is resistant to all possible regimens
  
  counterreg = 0;   //number of cycles pat is on the current regimen
  has_AIDS =  false;
  AIDSeventReg = false;	//initialize for first reg
  
  
  maxCD4onReg=0;
  mutCount=0;
  reached2DrugsResOnSalvage = false;
  historyOfIntolerance = false;
  
  nextMonCycle = -1;   //Number of cycles from now to check VL or CD4 again
  
  structDrugPtr = new drug;
  this->days_post_exposure = 0;
  fatal_hiv = false;
  
  break_process_patient2 = false;
  break_process_patient3 = false;
  
  VL_fred = 0;
  diagnosed = false;
  start_haart_day = -1;
  acute = false;
  end_acute = -1;
  HIV_stage = -1; //mina check that everyone with HIV has a stage //left
}

/***************************************************/
/***************************************************/

void HIV_Infection::setup() {
  FRED_VERBOSE(0, "HIV_Infection::setup() started.\n");
  
  if (this->host->get_id() == Global::TEST_ID) {
    //open files for writing to:
    if( (kim_test = fopen( "kim_test.txt", "w" )) == NULL ){
      printf("ERROR: kim_test.txt file was not opened\n" );
      exit(0);
    }
    
    if( (mina_test = fopen( "mina_test.txt", "w" )) == NULL ){
      printf("ERROR: mina_test.txt file was not opened\n" );
      exit(0);
    }
    
    ostringstream xls_name;
    if( (CD4_excel = fopen( "FRED_CD4.xls", "w" )) == NULL ){
      printf("ERROR: FRED_CD4.xls file was not opened\n" );
      exit(0);
    }
    if( (VL_excel = fopen( "FRED_VL.xls", "w" )) == NULL ){
      printf("ERROR: FRED_VL.xls file was not opened\n" );
      exit(0);
    }
  }
  
  if(SCREEN) this->diagnosed = false;
  else this->diagnosed = true;        //if we are not modeling screening, everyone is diagnosed.

  
  // initialize HIV specific-variables here:
  this->vl_regimen_failure_1st = VL_REGIMEN_FAILURE_1ST;					//for MONITORING_PAPER
  this->vl_regimen_failure_after_1st = VL_REGIMEN_FAILURE_AFTER_1ST;		//for MONITORING_PAPER
  this->comp = COMP;
  this->mut_rate = MUT_RATE;
  this->assoc_comp = ASSOC_COMP;
  this->factor = FACTOR;
  this->adjmutres = ADJMUTRES;
  this->adjcrossres = ADJCROSSRES;
  this->arv_types = ARV_TYPES;
  this->trig = TRIG;											//The type of regimen change trigger to use (values in hiv.h)
  this->patient_starts_on_haart = PATIENT_STARTS_ON_HAART;
  this->monitor_interval = MONITOR_INTERVAL;
  this->cd4_treat = CD4_TREAT;
  this->haart_tox = HAART_TOX;
  this->testCostVL = TESTCOSTVL;
  this->testCostCD4 = TESTCOSTCD4;

  
  if (Global::Simulation_Day == 0) { //initial prevalence setup
    this->acute = false;    // Assumption for now.
    
    double rand_stage = Random::draw_random();
    
    if (host->get_sex() == 'M' && this->has_AIDS == false) {    //people needed with AIDS are taken care of in HIV_Epidemic::import_aux2: just for their disease stage label, not their CD4 and viral load. CD4 and viral load should be set here.
      //re-calculate cut offs becasue we are not using % people with AIDS here.
      double proportion_1 = HIV_Epidemic::HIV_prev_diseaseStage_M1 / (HIV_Epidemic::HIV_prev_diseaseStage_M1 + HIV_Epidemic::HIV_prev_diseaseStage_M2);
      
      if (rand_stage < proportion_1) {
        this -> set_HIV_stage(1);
      }
      else if (rand_stage >= proportion_1){
        this -> set_HIV_stage(2);
      }
    }
    else if (host->get_sex() == 'F' && this->has_AIDS == false) {
      
      double proportion_1 = HIV_Epidemic::HIV_prev_diseaseStage_F1 / (HIV_Epidemic::HIV_prev_diseaseStage_F1 + HIV_Epidemic::HIV_prev_diseaseStage_F2);
      
      if (rand_stage < proportion_1) {
        this -> set_HIV_stage(1);
      }
      else if (rand_stage >= proportion_1){
        this -> set_HIV_stage(2);
      }
    }

    //set CD4 and VL based on HIV stage:
    // Disease stage - CD4
    //Stage 1 (CD4 >500 cells/μL or >29%)
    //Stage 2 (CD4 200–499 cells/μL or 14%–28%)
    //Stage 3 (AIDS) (OI or CD4 <200 cells/μL or <14%)

    if (this->HIV_stage == 1) {
      this->start_hiv = 3.56; //Mellors et al. Page 3 of Mellors: CD4 and viral load (log 10)
      this->start_cd4 = Random::draw_random_int(500, 900);
    }
    else if (this->HIV_stage == 2) {
      this->start_hiv = AVG_HIV;
      this->start_cd4 = Random::draw_random_int(200, 499);
    }
    else if (this->HIV_stage == 3) {
      this->start_hiv = 4.3;    //Mellors et al. Page 3 of Mellors: CD4 and viral load (log 10)
      this->start_cd4 = Random::draw_random_int(20, 199);
      this->has_AIDS = true;
    }
    

    this->avgHIV_SD = AVG_HIV_SD;
    this->avgCD4_SD = AVG_CD4_SD;
    
  } else {
    this->acute = true;
    //this is for new infections: Start as acute HIV infection
    this->start_hiv = Random::draw_normal(AVG_ACUTE_HIV, AVG_ACUTE_HIV_SD) ;
    
    //****** Mina Updated: we want to assign CD4 count when person is infected with HIV
    //****** WHAT Is the decline in CD4 count after getting infected?
    //****** CD4 count when infected with HIV :  900 (750–900)  Sanders et al. 2005
    this->start_cd4 = ((HIV_Natural_History *)(this->condition->get_natural_history()))->assign_initial_CD4();
    
    this->avgHIV_SD = AVG_ACUTE_HIV_SD;
    this->avgCD4_SD = AVG_CD4_SD;
   
    this->print_infection();
    //assume acute infection last 4-6 weeks: 120 to 180 days
    int dur = (int)(Random::draw_random(120, 180));
    this->end_acute = this->host->get_exposure_date(this->condition_id) + dur;
    
    //cout << "Start HIV " << this->start_hiv << " end acute: " << this->end_acute << endl;
    //getchar();  //for debug
  
    
    this->HIV_stage = ((HIV_Natural_History *)(this->condition->get_natural_history()))->update_hiv_stage(start_cd4);
    //left
  }
  
  
  
  this->stop_reg = STOP_REG;
  this->mortMultHIV = MORTMULTHIV;
  this->intolMultiplier = INTOL_MULTIPLIER;
  this->start_with_aids = START_WITH_AIDS;
  this->arv_types = ARV_TYPES;
  this->nextMonCycle = CYCLES_PER_YEAR; //Number of cycles from now to check VL or CD4 again //Mina check
  
  //calculate the exponential rates r used in the VLreal and CD4real calculations
  this->rVLreal = -log(1-1/VL_ADJUST);
  this->rCD4within_delta_real = -log(1-1/CD4_ADJUST_W);
  
  //patient starts out not being tolerant or resistant to any drugs
  this->num_res[PI_SINGULAR] = 0;
  this->num_res[PI_BOOSTED] = 0;
  this->num_res[NRTI_TAM] = 0;
  this->num_res[NRTI_NONTAM] = 0;
  this->num_res[NNRTI_EFAVIRENZ] = 0;
  this->num_res[NNRTI_NEVIRAPINE] = 0;
  
  for (int i=0; i<ARV_TYPES; i++)
  {
    switch (i)
    {
      case 0: strcpy(this->className[i], "PI_Singular"); break;
      case 1: strcpy(this->className[i], "PI_Boosted"); break;
      case 2: strcpy(this->className[i], "TAM"); break;
      case 3: strcpy(this->className[i], "NONTAM"); break;
      case 4: strcpy(this->className[i], "Efavirenz"); break;
      case 5: strcpy(this->className[i], "Nevirapine"); break;
    }
  }
  
  this->pmutres_pi_singular_est = PMUTRES_PI_SINGULAR_EST;
  this->pmutres_pi_boosted_est = PMUTRES_PI_BOOSTED_EST;
  this->pmutres_nrti_tam_est = PMUTRES_NRTI_TAM_EST;
  this->pmutres_nrti_nontam_est = PMUTRES_NRTI_NONTAM_EST;
  this->pmutres_nnrti_efavirenz_est = PMUTRES_NNRTI_EFAVIRENZ_EST;
  this->pmutres_nnrti_nevirapine_est = PMUTRES_NNRTI_NEVIRAPINE_EST;
  
  this->pcrossres_pi_singular_est = PCROSSRES_PI_SINGULAR_EST;
  this->pcrossres_pi_boosted_est	= PCROSSRES_PI_BOOSTED_EST;
  this->pcrossres_nrti_tam_est = PCROSSRES_NRTI_TAM_EST;
  this->pcrossres_nrti_nontam_est = PCROSSRES_NRTI_NONTAM_EST;
  this->pcrossres_nnrti_efavirenz_est = PCROSSRES_NNRTI_EFAVIRENZ_EST;
  this->pcrossres_nnrti_nevirapine_est = PCROSSRES_NNRTI_NEVIRAPINE_EST;
  
  this->pcrossres_piboosted_singular = PCROSSRESPI_BOOSTED_SINGULAR;
  this->pcrossres_pisingular_boosted = PCROSSRESPI_SINGULAR_BOOSTED;
  this->pcrossres_tam_nontam = PCROSSRESNRTI_TAM_NONTAM;
  this->pcrossres_nontam_tam = PCROSSRESNRTI_NONTAM_TAM;
  this->pcrossres_efav_nevir = PCROSSRESNNRTI_EFAV_NEVIR;
  this->pcrossres_nevir_efav = PCROSSRESNNRTI_NEVIR_EFAV;
  
  //determine the probability of developing a mutation to each drug type
  //note:  mut_rate_nrti + mut_rate_nnrti + mut_rate_pi = mut_rate;
  //note:  mut_rate_nrti + RATIO_NNRTI_TO_NRTI*mut_rate_nrti + RATIO_PI_TO_NRTI*pmut_nrti = mut_rate;
  this->mut_rate_nrti = mut_rate/(1.0 + RATIO_NNRTI_TO_NRTI + RATIO_PI_TO_NRTI);
  this->mut_rate_nnrti = RATIO_NNRTI_TO_NRTI * mut_rate_nrti;
  this->mut_rate_pi = RATIO_PI_TO_NRTI * mut_rate_nrti;
  
  //Set the number of drugs in each class
  this->num_in_class[PI_SINGULAR] = HIV_Natural_History::NUM_PI_SINGULAR;
  this->num_in_class[PI_BOOSTED] = HIV_Natural_History::NUM_PI_BOOSTED;
  this->num_in_class[NRTI_TAM] = HIV_Natural_History::NUM_NRTI_TAM;
  this->num_in_class[NRTI_NONTAM] = HIV_Natural_History::NUM_NRTI_NONTAM;
  this->num_in_class[NNRTI_EFAVIRENZ] = HIV_Natural_History::NUM_NNRTI_EFAVIRENZ;
  this->num_in_class[NNRTI_NEVIRAPINE] = HIV_Natural_History::NUM_NNRTI_NEVIRAPINE;
  
  //initialize number of mutations against each drug type
  this->mut_arv[PI_SINGULAR] = MUT_PI_SINGULAR_START;
  this->mut_arv[PI_BOOSTED] = MUT_PI_BOOSTED_START;
  this->mut_arv[NRTI_TAM] = MUT_NRTI_TAM_START;
  this->mut_arv[NRTI_NONTAM] = MUT_NRTI_NONTAM_START;
  this->mut_arv[NNRTI_EFAVIRENZ] = MUT_NNRTI_EFAVIRENZ_START;
  this->mut_arv[NNRTI_NEVIRAPINE] = MUT_NNRTI_NEVIRAPINE_START;
  
  //Min statements are unnec for our current constants
  //Since the above comment says they are unncessary, I'm removing them!  KAN v2.0
  //gives prob of a mutation being resistant to each ARV type
  this->pmutres[PI_SINGULAR] = adjmutres*pmutres_pi_singular_est;  //Min(.99, adjmutres*pmutres_pi_singular_est);
  this->pmutres[PI_BOOSTED] = pmutres_pi_boosted_est;   //Min(.99, adjmutres*pmutres_pi_boosted_est);
  this->pmutres[NRTI_TAM] = pmutres_nrti_tam_est;   //Min(.99, adjmutres*pmutres_nrti_tam_est);
  this->pmutres[NRTI_NONTAM] = pmutres_nrti_nontam_est;   //Min(.99, adjmutres*pmutres_nrti_nontam_est);
  this->pmutres[NNRTI_EFAVIRENZ] = pmutres_nnrti_efavirenz_est;   //Min(.99, adjmutres*pmutres_nnrti_efavirenz_est);
  this->pmutres[NNRTI_NEVIRAPINE] = pmutres_nnrti_nevirapine_est;   //Min(.99, adjmutres*pmutres_nnrti_nevirapine_est);
  
  //gives prob of a resistant strain being cross resistant to other drugs in the same class
  this->pcrossres[PI_SINGULAR] = pcrossres_pi_singular_est;   //Min(.99, adjcrossres*pcrossres_pi_singular_est);
  this->pcrossres[PI_BOOSTED] = pcrossres_pi_boosted_est;   //Min(.99, adjcrossres*pcrossres_pi_boosted_est);
  this->pcrossres[NRTI_TAM] = pcrossres_nrti_tam_est;   //Min(.99, adjcrossres*pcrossres_nrti_tam_est);
  this->pcrossres[NRTI_NONTAM] = pcrossres_nrti_nontam_est;   //Min(.99, adjcrossres*pcrossres_nrti_nontam_est);
  this->pcrossres[NNRTI_EFAVIRENZ] = pcrossres_nnrti_efavirenz_est;   //Min(.99, adjcrossres*pcrossres_nnrti_efavirenz_est);
  this->pcrossres[NNRTI_NEVIRAPINE] = pcrossres_nnrti_nevirapine_est;   //Min(.99, adjcrossres*pcrossres_nnrti_nevirapine_est);
  
  this->crossClassType[PI_SINGULAR]=PI_BOOSTED;
  this->crossClassType[PI_BOOSTED]=PI_SINGULAR;
  this->crossClassType[NRTI_TAM]=NRTI_NONTAM;
  this->crossClassType[NRTI_NONTAM]=NRTI_TAM;
  this->crossClassType[NNRTI_EFAVIRENZ]=NNRTI_NEVIRAPINE;
  this->crossClassType[NNRTI_NEVIRAPINE]=NNRTI_EFAVIRENZ;
  
  this->pCrossClassRes[PI_SINGULAR]=pcrossres_pisingular_boosted;
  this->pCrossClassRes[PI_BOOSTED]=pcrossres_piboosted_singular;
  this->pCrossClassRes[NRTI_TAM]=pcrossres_tam_nontam;
  this->pCrossClassRes[NRTI_NONTAM]=pcrossres_nontam_tam;
  this->pCrossClassRes[NNRTI_EFAVIRENZ]=pcrossres_efav_nevir;
  this->pCrossClassRes[NNRTI_NEVIRAPINE]=pcrossres_nevir_efav;
  
  //Given the overall mutation rate, this is the probability of a mutation occurring in this class
  this->mutRateClass[PI_SINGULAR] = mut_rate_pi;
  this->mutRateClass[PI_BOOSTED] = mut_rate_pi;
  this->mutRateClass[NRTI_TAM] = mut_rate_nrti;
  this->mutRateClass[NRTI_NONTAM] = mut_rate_nrti;
  this->mutRateClass[NNRTI_EFAVIRENZ] = mut_rate_nnrti;
  this->mutRateClass[NNRTI_NEVIRAPINE] = mut_rate_nnrti;
  
  this->started_haart = false;
  this->quit_haart = 0;					//huh? (int)(MAX_MONTHS/CYCTIME)+5;
  this->haart = 0;
  this->exhausted_clean_regs = false;
  this->numreg = 1;
  this->counterreg = 0;	//number of cycles pat is on the current regimen
  this->vl_regimen_failure = vl_regimen_failure_1st;
  this->has_AIDS = false;
  this->AIDSeventReg = false;	//initialize for first reg
  
  this->num_regs_before_salvage = 0;
  this->num_res_drugs_before_salvage = 0;
  this->maxCD4onReg=0;
  this->mutCount=0;
  this->reached2DrugsResOnSalvage = false;
  this->historyOfIntolerance = false;
  
  this->qa_time=0; this->qamedian=0;
  this->cost=0; this->total_cost=0; this->mean_cost=0; this->total_disc_cost=0; this->mean_disc_cost=0; //,individual_cost[PATIENTS];
  this->drug_cost=0; this->total_drug_cost=0; this->mean_drug_cost=0; this->total_drug_disc_cost=0; this->mean_disc_drug_cost=0;
  this->total_lab_cost=0; this->mean_lab_cost=0; this->total_lab_disc_cost=0; this->mean_disc_lab_cost=0;
  this->hospital_cost=0; this->total_hospital_cost=0; this->mean_hospital_cost=0; this->total_hospital_disc_cost=0; this->mean_disc_hospital_cost=0;
  this->care_cost=0; this->total_care_cost=0; this->mean_care_cost=0; this->total_care_disc_cost=0; this->mean_disc_care_cost=0;
  
  //initialize the drugs array (note that this has to be done after the num_in_class array is set!
  for (int c=0; c<this->arv_types; c++){
    //int num_in_class = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_num_in_class(c);
    for (int d=0; d<num_in_class[c]; d++){
      drugs[c][d].drugNum=d;
      drugs[c][d].classNum=c;
      drugs[c][d].res=false;
      drugs[c][d].intol=false;
    }
  }
  
  this->init_reg[0] = INITIAL_REG_DRUG1;
  this->init_reg[1] = INITIAL_REG_DRUG2;
  this->init_reg[2] = INITIAL_REG_DRUG3;
  
  //  if (REASONS_FOR_REG_CHANGES){
  //    for (int reg=0; reg<3; reg++)
  //    {
  //      regChangeIntol[reg]=0;
  //      regChangeFail[reg]=0;
  //    }
  //  }
  
  //If we're running the calibration for RESOURCE_RICH, we want to emulate
  //the drug regimens from the VACS population used in Scott's 2007 AIDS paper
  //We set the proportion of patients on each reg according to the paper
  //Since we don't have 3NRTI Regs or "Other" in our model, we recalate the
  //proportions according to the regs we do have
  //Prop Efav = 1140/(6397-(517+500)) = .212
  //Prop Nevir = 512/(6397-(517+500)) = .0953
  //Prop Single PI = 3324 /(6397-(517+500)) = .618
  //Prop Boosted PI = 401 / (6397-(517+500)) = .0747
  
  /*   if (RESOURCE_RICH && CALIBRATE)
   {
   if (patnum+1<PATIENTS*.212) pat->init_reg[0] = NNRTI_EFAVIRENZ;
   else if (patnum+1<PATIENTS*(.212+.0953)) pat->init_reg[0] = NNRTI_NEVIRAPINE;
   else if (patnum+1<PATIENTS*(.212+.0953+.618)) pat->init_reg[0] = PI_SINGULAR;
   else if (patnum+1>=PATIENTS*(.212+.0953+.618)) pat->init_reg[0] = PI_BOOSTED;
   }
   */
  
  double normal1 = normal(0,1,&HIV_Natural_History::seed);
  double normal2 = normal(0,1,&HIV_Natural_History::seed);
  double normal3 = normal(0,1,&HIV_Natural_History::seed);
  double uniform1 = uniform(&HIV_Natural_History::seed);
  double uniform2 = uniform(&HIV_Natural_History::seed);
  
  // mina debug
  if (this->host->get_id() == Global::TEST_ID) {
    //fprintf(mina_test, "\n\nnormal1: %f \t normal2: %f\t normal3: %f", normal1, normal2, normal3);
    //fprintf(mina_test, "\nuniform1: %f \t uniform2: %f", uniform1, uniform2);
  }
  double rand_normal1 = 0; //mina: how do you suggest to use the random numbers? array of them? How to add uniform?
  double rand_normal2 = 0;
  double rand_normal3 = 0;
  double rand_normal4 = 0;
  double rand_uniform1 = 0;
  double rand_uniform2 = 0;
  
  
  
  
  
  //Mina check
  //*The function getInitialAIDSRates() is used when START_WITH_AIDS (the percentage of patients starting with AIDS)
  // getInitialAIDSRates();
  
  
  /**************************************
   //mina check
   //newly added by Mina January 2016
   double CD4_temp = Random::draw_normal(850, 350);    //use later. CD4 count range for healthy people: 500 to 1200
   this->CD4baseline = 0.75*CD4_temp;
   
   //we need an indicator to check whether this is a new infection or not. Any flag?
   **************************************/
  
  
  //fprintf (mina_test, "\nInitial Seed: %ld\n", &HIV_Natural_History::seed);
  //pat->start_age = Max(Min(MAX_AGE, start_age + avgAge_SD*(!VRT ? normal(0,1,&seed) : unifInit[0])), 0);
  //mina: I want to fake using one of the seeds for normal distribution to be the same as Scott's model here based on the line above:
  int fake_seed = 0*(!VRT ? normal1 : rand_normal2);
  //  if (KIMTEST) fprintf (kim_test, "\n\n MINAAA seed = %.4ld \t normal(0,1,&HIV_Natural_History::seed)= %.4f.  \n\n ", HIV_Natural_History::seed, normal(0,1,&HIV_Natural_History::seed));
  
  if (SCREEN) {
    if (this->diagnosed == false) {
      this->CD4baseline = this->start_cd4;
      this->HIVbaseline = this->start_hiv;
    }
    else if (this->diagnosed == true) {
      this->CD4baseline = Max(Min(MAX_CD4, HIV_Natural_History::AVG_CD4 + AVG_CD4_SD*(!VRT ? normal2 : rand_normal3)), 0);
      this->HIVbaseline = Max(Min(MAX_HIV, AVG_HIV + AVG_HIV_SD*(!VRT ? normal3 : rand_normal4)), 0);
    }
    
    if (this->host->get_id() == Global::TEST_ID) {
      cout << "CD4baseline  " << CD4baseline << "  diagnosed " << this->diagnosed << endl;
    }
  }
  else {
    this->CD4baseline = Max(Min(MAX_CD4, HIV_Natural_History::AVG_CD4 + AVG_CD4_SD*(!VRT ? normal2 : rand_normal3)), 0);
    this->HIVbaseline = Max(Min(MAX_HIV, AVG_HIV + AVG_HIV_SD*(!VRT ? normal3 : rand_normal4)), 0);
  }
  
  
  this->start_age = this->host->get_age();
  
  if (KIMTEST && this->has_AIDS) fprintf (kim_test, "Pat started with AIDS!\n");
  
  //initialize compliance to drug1, drug2, drug3
  for (int i = 0; i < DRUGS_IN_REG; i++){
    this->nc[i] = 0;
  }
  
  this->VLreal_before_noise = this->VLideal = this->VLreal = this->HIVbaseline;
  this->CD4offreal = this->CD4Peak = this->CD4real = this->CD4real_before_noise = this->CD4baseline;
  
  //the idea behind luck1 and luck2 is that luck1 represents an individual's tendency to have good/bad CD4 reactions
  //(this is random from patient to patient). luck2 represents regimen-specific randomness to good/bad CD4 changes
  //diligence allows a random variation around the average cohort compliance from patient to patient
  
  double normal4 = normal(0,1,&HIV_Natural_History::seed);
  double normal5 = normal(0,1,&HIV_Natural_History::seed);
  double uniform3 = uniform(&HIV_Natural_History::seed);
  
  //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\nnormal4: %f \t normal5: %f\t uniform3: %f \n", normal4, normal5, uniform3);
  
  double rand_normal5 = 0;
  double rand_normal6 = 0;
  double rand_uniform3 = 0;

  //VRT
  this->luck1 = (!VRT ? normal4 : rand_normal5 /*unifInit[5]*/); //Min(2.0, Max(-2.0, normal(0, 1, &seed)));
  this->luck2 = (!VRT ? normal5 : rand_normal6 /*unifInit[6]*/);
  this->diligence = (!VRT ? uniform3 : rand_uniform3 /*unifInit[7]*/);

  setPatIndividualDrugCompliance();

  for (int i=0; i<MAX_REGS; i++){
    this->cyclesOnReg[i]=0;
  }
  
  //The noise pattern for each patient is looking the same.
  //Add an offset to change the start of the sin wave so pat to pat is out of phase
  offset1 = RandomNumber(999);
  offset2 = RandomNumber(999);
  
  //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\noffset1: %d \t offset2: %d", offset1, offset2);      //mina debug
  
  //	if (VLCD4GRAPH){
  //		regChangeIntolFlag = false;
  //		regChangeFailFlag = false;
  //	}
  //	if (DEBUG_NUM_MUTS) pat->hasResistance=false;

  //if there are mutations at the start, we want to see if any of the accumulated mutations are resistant ones
  Process_Initial_Mutations();
  
  //Used for generating the transmission model lookup tables with GETSTATES
  //totalCostAtStartOfYearThisPat = total_cost;   //mina change
  
  //Set to nonsense number that will never be correct
  //if (DO_CLINICAL_VISIT_EVERY_SIX_MONTHS_FOR_WHO) sixMonthsSinceLastVisit = -999;   //mina - not needed in FRED for now
  
  if (this->host->get_id() == Global::TEST_ID){
    fprintf(mina_test, "HIV infection initialization\n");
    fprintf(mina_test, "HIV Patient. ID %d\tDiagnosed %d\tCD4baseline %.1f\tHIVbaseline %.1f\t  HAART start day %d\n" , this->host->get_id(), this->diagnosed, this->CD4baseline, this->HIVbaseline, this->start_haart_day);
  }

  if (KIMTEST) fprintf (kim_test, "PATIENT INITIALIZED\nBaseline Age: %.2f, CD4: %.2f, VL: %.2f, luck1: %.2f\n", this->start_age, this->CD4baseline, this->HIVbaseline, this->luck1);
  
  FRED_VERBOSE(0, "HIV_Infection::setup() finished.\n"); fflush(stdout);
  
} //end of setup()

/***************************************************/
/***************************************************/

double HIV_Infection::get_infectivity(int day) {
  
  //Kim'a email about HIV code:
  // The numbers are the log of the VL.  So a VL of less than 3.5 refers to a VL of 3,162 copies/mL. In that code, there are four categories for log viral load.  <3.5, 3.5-4.5, 4.5-5.5, > 5.5.
  
  VL_fred = get_VL_Fred();
  
  double infectivity_prob = 0;
  if (this->VL_fred <=500)
    infectivity_prob = 0.0001;
  else if (this->VL_fred>500 && this->VL_fred<=3000)
    infectivity_prob = 0.0012;
  else if (this->VL_fred>3000 && this->VL_fred<=10000)
    infectivity_prob = 0.0012;
  else if (this->VL_fred>10000 && this->VL_fred<=30000)
    infectivity_prob = 0.0014;
  else if (this->VL_fred>30000)
    infectivity_prob = 0.0023;
  else
    infectivity_prob = 0;
  
  if (HIV_Epidemic::CONDOM_USE)
    return infectivity_prob; // *0.20*0.8 ;    //mina REMOVED
  else return infectivity_prob;
}

bool HIV_Infection::is_fatal(int day) {   //mina check
  if (fatal_hiv) return true;
  else return false;
}

/***************************************************/
/***************************************************/

void HIV_Infection::update(int day) {
  
  FRED_VERBOSE(1, "update HIV INFECTION on day %d for host %d", day, this->host->get_id());
  
  // put daily update here
  int disease_id = condition->get_id();
  int exposure_date = this->host->get_exposure_date(disease_id);
  
  int today = day ; //-1 is used so that we have the correct days post exposure based on Scott's model. Update: -1 is removed because FRED starts from day 0 now
  this->days_post_exposure = today - exposure_date;
  if (days_post_exposure == -1) {
    printf("ERROR MINA: days post exposure is negative! Press key to continue.");
    getchar();   //for error
  }
  
  //******* ACUTE phase ending ************
  //***************************************
  if (this->acute && today == this->end_acute) {
    this->acute = false;
    
    this->start_hiv = AVG_HIV;
    this->start_cd4 = HIV_Natural_History::AVG_CD4;
    this->avgHIV_SD = AVG_HIV_SD;
    this->avgCD4_SD = AVG_CD4_SD;
    
    if (SCREEN) {
      if (this->diagnosed == false) {
        this->CD4baseline = this->start_cd4;
        this->HIVbaseline = this->start_hiv;
      }
      else if (this->diagnosed == true) {
        this->CD4baseline = Max(Min(MAX_CD4, HIV_Natural_History::AVG_CD4 + AVG_CD4_SD*(Random::draw_normal(0,1))), 0);
        this->HIVbaseline = Max(Min(MAX_HIV, AVG_HIV + AVG_HIV_SD*(Random::draw_normal(0,1))), 0);
      }
      
      if (this->host->get_id() == Global::TEST_ID) {
        cout << "CD4baseline  " << CD4baseline << "  diagnosed " << this->diagnosed << endl;
      }
    }
    else {
      this->CD4baseline = Max(Min(MAX_CD4, HIV_Natural_History::AVG_CD4 + AVG_CD4_SD*(Random::draw_normal(0,1))), 0);
      this->HIVbaseline = Max(Min(MAX_HIV, AVG_HIV + AVG_HIV_SD*(Random::draw_normal(0,1))), 0);
    }
  }
    
    
    
    
    
  
  //************* SCREENING ***************
  //***************************************
  //screen once a year : on birthday
  int birthday = Date::get_day_of_year((this->host->get_demographics())->get_birthday_sim_day());

  if (birthday == day && SCREEN && this->diagnosed == false) {
    double today_screening_prob = 0.0;
    today_screening_prob = ((double) rand() / (RAND_MAX));
    
    if (today_screening_prob <= HIV_Epidemic::HIV_screening_prob) {
      this->diagnosed = true;
      //assign treatment initiation date
      this->start_haart_day = day + ((HIV_Epidemic *)(this->condition->get_epidemic()))->time_to_care();
      
      FRED_VERBOSE(0, "HIV infection is diagnosed. Host ID = %d\n", this->host->get_id() )
      
      if (this->host->get_id() == Global::TEST_ID){
        cout << "birthday sim day  = " << birthday << endl;
       // getchar();
      }
    }
  }
  
  if ((SCREEN && this->diagnosed == true) || !SCREEN){   //if SCREEN = 0, everyone is diagnosed.
    if (this->CD4real <= this->cd4_treat) start_haart_day = day;    //start today. check condition for diagnosed cases
  }

  //***************************************
  //***************************************
  
  if (this->host->get_id() == Global::TEST_ID){
    fprintf(mina_test, "\n\n");
    fprintf(mina_test, "HIV Patient. ID %d\tDiagnosed %d\t HAART start day %d\n" , this->host->get_id(), this->diagnosed, this->start_haart_day);
  }

  //Kim's comment:
  //run through Markov cycle until stopping criteria are met. In general, there will be many recursive calls within Process_Patient, ending when a patient either dies HIV, dies Age, or some other stopping criteria are met.
  //Mina: I have eliminated recursive calls (i.e. goto commands) by defining new variables such as break_process_patient and also splitting process_patient method into multiple sections
  //reset break_process_patient values before updating infection
  this->break_process_patient2 = false;
  this->break_process_patient3 = false;
  
  process_patient1(today, exposure_date);
  
  // if this HIV is fatal today: set_fatal_infection and abort updating health in process_patient2.
  if (fatal_hiv){
    this->host->set_case_fatality(this->condition_id);
    if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n\nHIV Patient DIED.");
    printf("\nHIV Patient DIED. \n");
    //getchar();
    return;
  }
  
  //if patient does not die of HIV, we now call process_patient2
  process_patient2(day, exposure_date);
  
  if (!(this->break_process_patient2)) {
    process_patient3(day, exposure_date);
    if (!(this->break_process_patient3))    // break_process_patient3 should be false to continue to process 4
      process_patient4(day, exposure_date);
  }
  
  this->HIV_stage = ((HIV_Natural_History *)(this->condition->get_natural_history()))->update_hiv_stage(CD4real); //upate HIV stage based on cd4
  
  
  int full_year;
  double temp;
  
  /*
  ostringstream xls_name;
    if (CD4_OUTPUT_EXCEL_DAY) {
      fprintf(Global::mina_test, "Time to fill CD4 real array for patient. Day %d.\t", day);
      fprintf(Global::CD4_excel, "%.2f\t", real_CD4_day[day]);
      fprintf(Global::VL_excel, "%.2f\t", real_VL_day[day]);
  
      if ((day % (CYCLES_PER_YEAR-1) == 0.00) && day!=0){
        fprintf(Global::CD4_excel, "\n");
        fprintf(Global::VL_excel, "\n");
      }
    }
  
    else{
      if (day % CYCLES_PER_YEAR == 0.00){
          full_year = (int)(day/CYCLES_PER_YEAR);
          fprintf(Global::mina_test, "Time to fill CD4 real array for patient. Year %d.\t", full_year);
          temp= get_CD4_real(full_year-1);   //-1 to start array from 0
          fprintf(Global::mina_test, "temp CD_real = %.2f\n\n", temp);
          fprintf(Global::CD4_excel, "%.2f\t", get_CD4_real(full_year-1));
  
  
          if ((int)(day/CYCLES_PER_YEAR) == TIME+1)
            fprintf(Global::CD4_excel, "\n\t");
        }
    }
  */
  FRED_VERBOSE(1, "HIV infection Update complete, Day: %d \n", today);
  
  
}   //end of update

/***************************************************/
/***************************************************/

void HIV_Infection::process_patient1(int today, int exposure_date){
  
  FRED_VERBOSE(1, "HIV_Infection::process_patient1 started. Day: %d \n", today);
  
  if (patient_starts_on_haart && start_haart_day == today) start_haart(today);
  //Dafault is that person is not initialized with haart : assigned day to start HAART
  
  /***************************************************************************************************
   At beginning of the month, we determine if the patient will be on haart for this next month.  When the CD4real falls below the treatment threshold, we put the patient on haart (as long has the patient hasn't exhausted all regimens, or if he has but we "stay on haart" which indicates we will continue to put the patient on haart because living with resistant strains is better than living with the wild type.  Has to do with viral fitness concept).
   Note that once patient falls below the CD4_TREAT, we keep the patient on haart, even if the CD4real 	subsequently rises above the treatment threshold.  We have the variable pat->started_haart because we consider the VL/CD4 dynamics to still be affected by a haart regimen after the patient stops that regimen (through the lag effect).
   *****************************************************************************************************/
  
  if (!this->started_haart && this->CD4real <= cd4_treat){

    if (SCREEN ) {
      if (this->diagnosed == true && this->start_haart_day == today) {
        start_haart(today);
      }
      else
        ;//no treatment date assigned
    }
    else
      start_haart(today);
  }
  
  //Increment time on current regimen. The ( pat->numreg < MAX_REGS_MODEL ) is to make sure we don't exceed the size of the storage array
  if ( ( this->numreg <= MAX_REGS_MODEL ) && this->haart ){
    this->cyclesOnReg[this->numreg-1]++;
    this->counterreg++;
  }
  
  if (this->CD4Peak < this->CD4real) this->CD4Peak = this->CD4real;
  
  
  
  //Quality adjust and discount.
  //mina change : we should find a way to set an array for patients in HIV_epidemic to calculate qa_time median for all patients
  //qamedian +=qa_time;
  
  //gives value between 0 and 1, according to patient's CD4,that represents 1 utility-weighted month
  double increment;
  qa_time = Utility();
  increment = ((double)this->days_post_exposure/(double)CYCLES_PER_YEAR); //mina check : days post exposure or today? I think days post exposure
  
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_qa_surv_time(qa_time, increment);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_disc_surv_time(increment);
  
  //mina change cost
  cost = Cost(TOTAL);
  
  //individual_cost[patnum]+=cost;							//Used for Retention in Care    // mina: not needed
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_cost(cost);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_disc_cost(cost, increment);
  
  //breakdown costs into Drug, Lab, Care, and Hospitalization
  drug_cost = Cost(DRUG);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_drug_cost(drug_cost);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_drug_disc_cost(drug_cost, increment);
  
  care_cost = Cost(CARE);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_care_cost(care_cost);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_care_disc_cost(care_cost, increment);
  
  hospital_cost = Cost(HOSP);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_hospital_cost(hospital_cost);
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_hospital_disc_cost(hospital_cost, increment);
  
  if (this->haart){
    //obtain the compliance pattern for this next month
    //For each drug in the regimen, will the patient be compliant this month? Yes/No
    Get_Compliance();
    
    //get number of drugs virus is *not* susceptible to. If virus is resistant to a drug or the patient is not compliant to that drug, then it is ineffective at fighting HIV, which we make use of below
    this->totalresnc = 0;
    this->totalres = 0;
    int i=0;
    for (i = 0; i < DRUGS_IN_REG; i++){  //mina check ..what is this?
      this->totalresnc += (int) Max(this->reg[i]->res, this->nc[i]);
      this->totalres += this->reg[i]->res;
    }
    
    //the following (fract) is a measure of the effectiveness of this regimen. It gives the fraction of drugs the virus is susceptible to.
    this->vl_fract = (double)(DRUGS_IN_REG-this->totalresnc)/DRUGS_IN_REG;
    if (KIMTEST) fprintf(kim_test, "\t DRUGS_IN_REG = %d      totalres = %d\n", DRUGS_IN_REG, this->totalres);   //mina debug
  }
    
  /*******************************************************************************************
   UPDATING PATIENT VIRAL LOAD
   -------------------------------------------------------------------------------------------
   Each patient starts with a baseline viral load.  We have this notion of an "ideal" and a "real" level of VL that has to do with a lag effect of the real level approaching the ideal level.  See description below in CD4 change section and constants.h  If the patient is not on haart, the VL ideal stays at the baseline level. When the patient starts haart, the ideal drops by a VLdec amount (defined in Initialize()) subject to an adjustment based on the effectiveness of the regimen (which is based on a combination of adherence and resistance
   *******************************************************************************************/
  
  //debug
  if (KIMTEST) fprintf (kim_test, "\tMINA DEBUG Cycle %d:  HIVbaseline = %f    haart = %d     vl_fract = %f     VLdec = %f  \n", today, this->HIVbaseline, this->haart, this->vl_fract, this->VLdec);
  
  this->VLideal = this->HIVbaseline - (this->haart * this->vl_fract * this->VLdec);
  
  this->VLreal_before_noise = (this->VLreal_before_noise - this->VLideal) * exp(-rVLreal*CYCTIME_IN_MONTHS) + this->VLideal;
  
  //Mina Debug
  if (this->host->get_id() == Global::TEST_ID){
    //fprintf(mina_test, "VLreal_before_noise= %f \t VLideal= %f \t rVLreal= %f \t\n", this->VLreal_before_noise,  this->VLideal, this->rVLreal);
    //  if (KIMTEST) fprintf (kim_test, "rVLreal = %.2f \n",  rVLreal);
    //  if (KIMTEST) fprintf (kim_test, "VLreal_before_noise = %.2f \n",  this->VLreal_before_noise);
    //  if (KIMTEST) fprintf (kim_test, "VLideal = %.2f \n",  this->VLideal);
  }
  
  /*The VL will be constant when the patient is not on haart or when the pat is resistant to all drugs in the reg.
   We add noise to the VL trajectory to be more realistic.*/
  if ((!this->haart || (this->reg[0]->res && this->reg[1]->res && this->reg[2]->res)) && USE_PERLIN_NOISE){
    this->VLreal =  addNoise(this->VLreal_before_noise, VL, this->days_post_exposure);
  }   //mina note: we want this->days_post_exposure-1, -1 is important, mina2: removed -1 because it's accounted for in today1-1 = today, in calculation of days_post_exposure
  else this->VLreal = this->VLreal_before_noise;
  
  this->VLcat = Get_Category(VL);
  
  
  //Mina Debug
  //  if (this->host->get_id() == Global::TEST_ID){
  //    fprintf (mina_test, "VLideal = %f    VLreal_after_noise = %f    VLcat = %d    ", this->VLideal, this->VLreal, this->VLcat);
  //  }
  
  if (KIMTEST && !ONLY_CORE_EVENTS) fprintf (kim_test, "\n\tCycle %d:  VLideal = %f    VLreal_after_noise = %f    VLcat = %d    ", today, this->VLideal, this->VLreal, this->VLcat);
  
  /*******************************************************************************************
   UPDATING PATIENT CD4 COUNT
   -------------------------------------------------------------------------------------------
   We consider CD4 count to be an aggregate of three types of CD4 dynamics: 1) CD4 changes when off of haart, 2) CD4 changes between regimens, and 3) CD4 changes within regimens.  For 2 and 3, we consider an "ideal" level and a "real" level. The ideal represents the level that the real level is approaching asymptotically (or with a lag effect).  This is why we do the real = real + (ideal-real)/adjust.  The denominator (adjust) is based on the real being X% of the ideal after Y months if everything else stayed the same.  See "constants.h" for further explanation. Note in the following that the offhaart level decreases when a patient is off haart, and freezes while a patient is on haart; the aggregated "between" effects are active from the time a patient starts haart until they die, regardless if they stop haart sometime in between; the "within effect" are also always active, but when a patient is off of haart, it is only an effect to the extent that the within effect approaches 0 asymptotically.  Note that the first change to the between level really takes effect after a patient changes regimen in Change_Regimen
   *******************************************************************************************/
  /*To clarify the following if statements, I will reference the following states:
   1)  Haart Naive (or 0% compliant)
   2)  On Haart - minimal resistance
   3)  On Haart - significant resistance (Plato equation)
   */
  
  //initialize CD4_plato to an impossibly high number (for use below where we take the min)
  CD4_plato = 9999;
  double asr_male_rate, asr_female_rate, temp;
  
  // (1: Haart Naive or 0% compliant)  Decrement based on the Mellors paper:
  temp = CYCTIME_IN_MONTHS*Max(0, 1.78 + 2.8*(this->VLreal - 3)) + DCD4NOHAART;
  
  this->CD4Mellors = this->CD4real_before_noise - temp;
  this->CD4Mellors = Max(0,this->CD4Mellors);
  
  // (2:  On Haart, minimal resistance)
  if (this->haart){
    //sets the variable pat->CD4regressionideal
    getCD4regressionideal(today);    //we need 'today' instead of 'days past exposure'
    
//    if (this->host->get_id() == Global::TEST_ID){
//      fprintf (mina_test, "\t CD4regressionideal = %.2f", this->CD4regressionideal);
//      fprintf (mina_test, "\n MINAAA \t  CD4regressionideal = %.2f  \n", this->CD4regressionideal);
//      fprintf (mina_test, "\n MINAAA \t  CD4real_before_noise = %.2f  \n", this->CD4real_before_noise);
//    }
    
    
    if (this->CD4real_before_noise < this->CD4regressionideal) this->CD4regressionreal = (this->CD4real_before_noise - this->CD4regressionideal) * exp(-rCD4within_delta_real*CYCTIME_IN_MONTHS)+ this->CD4regressionideal;
    
    if (this->CD4real_before_noise >= this->CD4regressionideal){
      delta = (this->CD4real_before_noise - this->CD4regressionideal) / CYCLES_PER_YEAR;
      //     if (KIMTEST) fprintf (mina_test, "\tdelta = %.2f\n", delta);
      this->CD4regressionreal = this->CD4real_before_noise - delta;
      //    if (KIMTEST && !ONLY_CORE_EVENTS) fprintf (kim_test, "\n MINAAA \t  CD4regressionreal = %.2f  \n", this->CD4regressionreal);
    }
  }
  
  // (3:  Pat is resistant to >= NUM_DRUGS_RES_FOR_PLATO drugs and their VL is below 4.0)
  // VL condition added on 4/20/09.  If the VL is above 4.0, the decline in CD4 is governed by PLATO.
  // But PLATO does not impose any decline if the VL is below 4.0.
  if (this->haart && (this->totalres >= NUM_DRUGS_RES_FOR_PLATO && this->VLreal >= 4.0)){
    //determine the number of cycles PLATO has been applied consecutively so far
    
    //mina change/check with Kim's code
    
    //if (pat->exhausted_clean_regs) cycles_since_PLATO_start = pat->cyclenum - pat->haart_exhausted_clean_regs_time;
    //else cycles_since_PLATO_start = pat->cyclenum - pat->cycleMetResistanceLimit;
    
    //decrement based on the PLATO study:
    //On the following line, that used to be an && instead of an ||,
    //but we don't want patients CD4s to increase if the VL is less than 4.0
    //so I'm changing it to an ||.
    //Up for debate whether or not to use the two-year limit (cycles_since_PLATO_start >=CYCLES_PER_YEAR*2 && pat->VLreal < 4.0 )
    
    if (this->VLreal >= 4.0) temp = ((double)-50 * this->VLreal + 200)/CYCLES_PER_YEAR;
    else temp = 0;
    
    CD4_plato = this->CD4real_before_noise + temp;
    CD4_plato = Max(0, CD4_plato);
  }
  
  //if the patient isn't on haart, the Mellors equation applies
  if (!this->haart) this->CD4real_before_noise = this->CD4Mellors;
  
  if (this->haart){
    //If the patient has less than 50% compliance, the patient's CD4 is calculated using a combination
    //of CD4 on haart (the regression equation) and CD4 not on haart (Mellors).  At 0%, the patient
    //should be entirely using Mellors, at 50%, they should be entirely using the Regression.
    if (this->comp < .5) this->CD4weightedForComp = linearInterpolate(0, this->CD4Mellors, 0.5, this->CD4regressionreal, this->comp);
    else this->CD4weightedForComp = this->CD4regressionreal;
  }
  
  //If the patient is on haart, we want the CD4real to be the lesser value of the CD4
  //calculated calculated from the PLATO equation and the CD4 from the regression equation
  //(and then waited for compliance) which may produce a lower value especially when the VL is close to 4.0.
  if (this->haart) this->CD4real_before_noise = Min (this->CD4weightedForComp, CD4_plato);
  
  //Add noise debug by Mina
  //  double mina1 = addNoise(this->CD4real_before_noise, CD4, this->days_post_exposure);
  //  if (KIMTEST) fprintf (kim_test, "add_noise output = %.2f , cyclenum= %d \t",  mina1, days_post_exposure);
  //  int mina2 = (int)uniform_a_b(0, 32767, &Disease::seed);
  //  fprintf (mina_test, "\nuniform_RandomNumber = %.2d ", mina2);
  //  double mina3 = PerlinNoise1D((days_post_exposure+offset1)*.002*5,1.5,5,2);
  //  fprintf (kim_test, "PerlinNoise1D = %.2f \n", mina3);
  
  
  if (USE_PERLIN_NOISE) this->CD4real = Max(0, addNoise(this->CD4real_before_noise, CD4, this->days_post_exposure));
  else this->CD4real = Max(0, this->CD4real_before_noise);
  
  this->CD4cat = Get_Category(CD4);
  
  //Some output to aid in debugging.  Used for graphing CD4 parameters over time.
  //if (VLCD4GRAPH) graph_trajectories();   //mina : not needed, used in retention in care
  //Mina Debug
  if (this->host->get_id() == Global::TEST_ID){
    //fprintf (mina_test, "CD4offreal: %.1f    CD4regressionideal: %.1f    CD4real_before_noise: %.1f    CD4real: %.1f\n", this->CD4offreal, this->CD4regressionideal, this->CD4real_before_noise, this->CD4real);
    fprintf (mina_test, "VLreal:%.1f \t CD4real: %.1f\n", this->VLreal, this->CD4real);
    if (this->haart) fprintf (mina_test, "\t Resistances:  %d  %d  %d\n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
  }
  
  if (KIMTEST && !ONLY_CORE_EVENTS) fprintf (kim_test, "CD4offreal: %.1f    CD4regressionideal: %.1f    CD4real_before_noise: %.1f    CD4real: %.1f\n", this->CD4offreal, this->CD4regressionideal, this->CD4real_before_noise, this->CD4real);
  if (KIMTEST && !ONLY_CORE_EVENTS && this->haart) fprintf (kim_test, "\t Resistances:  %d  %d  %d\n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
  
  
  /***********************************************************************************************
   Now that we have updated the VL and CD4 levels we check if the patient dies of hiv or non-hiv-related causes by the end of this month.  Then if we determine the patient is still alive, we continue on to check to see if resistant mutations developed.
   ***********************************************************************************************/
  
  this->deathrateHIV = Get_HIV_Death_Rate();
  
  //Kim: This was just a test.   We decided not to go with this code.  Leaving in for now, commented out
  //getSmoothedHIVDeathRate(pat);
  //#if RETENTION_IN_CARE
  //        check_for_AIDS_defining_events_RIC (pat);
  //#else

  //if the patient does not yet have AIDS, check for AIDS
  if (!this->has_AIDS || !this->AIDSeventReg){
    //The rate of an AIDS-defining event is AIDS_RATE_MULTIPLIER * pat->deathrateHIV
    this->pAIDS =  1 - exp(-(AIDS_EVENT_MULTIPLIER * this->deathrateHIV) * CYCTIME);
    
    double tempseed1=uniform(&HIV_Natural_History::seed);
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, " 1  Uniform_seed= %f \t", tempseed1 );
    
    if ((!VRT ? tempseed1 : 0 /*unifSim[0]*/) <= this->pAIDS){
      if (!this->has_AIDS)
      {
        this->has_AIDS = true;
        if (DEBUG_AIDS && (int)((days_post_exposure)/CYCLES_PER_YEAR) <= 5)
          //mina check : for today/cycles_per_year. what is this? if it's the time (in years) of being infected, it has to change...
          ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_AIDSorDeath();     //incerement counter in HIV_Natural_History.h and .cc for totalAIDSOrDeath++;
        //	if (WHO_MONITORING_STRATEGY_ANALYSIS) incrementWHOArray(pat, numEverDevelopedAIDS);
      }
      
      //Patient has had an AIDS-defining event during the current regimen
      this->AIDSeventReg = true;
      if (KIMTEST) fprintf (kim_test, "\nPatient had AIDS-defining event at cycle: %d\n\n", today);
    }
  } //end of if (!this->has_AIDS || !this->AIDSeventReg)
  //#endif
  
  //Adjust the deathrateHIV from the table for AIDS or non-AIDS
  if (this->has_AIDS) this->deathrateHIV = MORTMULTHIV * AIDS_ADJUST * COINFECTION_MULT * this->deathrateHIV;
  
  else this->deathrateHIV = MORTMULTHIV * NON_AIDS_ADJUST * COINFECTION_MULT * this->deathrateHIV;
  
  this->pDieHIV = 1 - exp(-this->deathrateHIV*CYCTIME);
  
  //MINA DEBUG
  /*if (pat->cyclenum == DIE_AT_DAY) pat->pDieHIV = 1.0;
   else pat->pDieHIV = 0;
   */
  
  double tempseed2 = uniform(&HIV_Natural_History::seed);
  //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, " 2  Uniform_seed= %f \t", tempseed2 );
  if (KIMTEST) fprintf(kim_test, "\n\tpDieHIV= %f \n", this->pDieHIV );
  
  if ((!VRT ? tempseed2 : 0 ) <= this->pDieHIV){
    if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "Patient %d dies of HIV on day %d.\n", this->host->get_id(), today);
    printf ("\nPatient dies of HIV in cycle %d.", today);
    //getchar();
    Process_Death(HIV, today);		//patient dies of HIV
    //goto end;     //MINA REMOVED!!!!
  }
  
   /*else{  REMOVED: //determine if patient will die from non-hiv-related causes
    
    asr_male_rate = Get_Rate(AGE_MALE, host->get_age());
    asr_female_rate = Get_Rate(AGE_FEMALE, host->get_age());
    
    //We don't specify sex of patient, so adjust deathrate for proportion male/female in the cohort
    this->deathrate_nonHIV = MALE * asr_male_rate + FEMALE * asr_female_rate;
    
    //  #ifdef MONITORING_PAPER_SENSITIVITY
    //  		if (floor((double)runnum/2) == 0) pat->deathrate_nonHIV *= sens[0][runnum % 2];
    //  #endif
    //If calibrating resource_rich, we want to use the VACS hiv-negative mortality value of 0.01526
    //(veterans have higher mortality than population average)
    //  if (RESOURCE_RICH && CALIBRATE)   //MINA CHECK: do I need this? not for now.
    //  {
    //     use the VACS hiv-neg mortality rate because it's higher
    //    this->deathrate_nonHIV = max (VACS_HIV_NEG_MORT_RATE, this->deathrate_nonHIV);
    //    this->deathrate_nonHIV *= VACS_HIV_NEG_MORT_MULT;
    //  }
    //Turn up the HIV-neg mortality for resource poor
    //  if (!RESOURCE_RICH && CALIBRATE) {
    //    this->deathrate_nonHIV *= RES_POOR_HIV_NEG_MORT_MULT;
    //  }
    
    //modify deathrate for haart toxicity
    if (this->haart) this->deathrate_nonHIV *= haart_tox;
    
    //there may be increased toxicity over the first three months (.25 years)of being on haart
    //This is currently set to 10.0 for resource poor, per Constantin, and 1.0 fo resource rich (no effect)
    if (this->haart && this->days_post_exposure <= .25 * (double)CYCLES_PER_YEAR) this->deathrate_nonHIV *= TOX_MULT_1ST_3_MONTHS;
    
    //TEST NEW
    //pat->pDieAge = 0;
    
    this->pDieAge = 1 - exp(-this->deathrate_nonHIV * CYCTIME);
    //fprintf(mina_test, "\n deathrate_nonHIV: %f\t pDieAge: %f", this->deathrate_nonHIV, this->pDieAge);
    //used for LOOP analysis for Scott
    //Turns off non-hiv mortality but caps life expectancy at 100 years
    //  if (removeNonHIVMort)   //mina set to 0 by default in Kim's code
    //  {
    //    if (this->days_post_exposure == 100 * CYCLES_PER_YEAR) this->pDieAge = 1.0;
    //    else this->pDieAge = 0;
    //  }
    
    double tempseed3 = uniform(&HIV_Natural_History::seed);
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, " 3  Uniform_seed= %f \t", tempseed3 );
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "Age = %d \t deathrate_nonHIV = %f \t pDieAge = %.8f \n", host->get_age(), deathrate_nonHIV, pDieAge);
    
    if ((!VRT ? tempseed3 : 0 ) <= this->pDieAge){
      //MINA CHANGE to use FRED mortality for age-related/all-cause deaths
      if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "Patient %d dies of AGE on day %d.\n", this->host->get_id(), today);
      printf ("\nPatient %d dies of AGE in cycle %d.", host->get_id(), today);
      //getchar();
      Process_Death(AGE, today);
      //goto end;     //MINA REMOVED!!!!
    }
    else {
      this->set_survival_days_alive();    //updated survival_days until this day, to accout for survival time, before the patient dies. used for debugging ONLY
    }
    
  } */
  
  //used for resource poor calibration
  /*
   if (CALIBRATE)
   {
   //When calibrating resource poor, determine if patient was censored
   if (pat->CD4real < 300) pat->censorRate = CENSOR_RATE;
   else pat->censorRate = 0;
   
   pat->pCensor = 1 - exp(-pat->censorRate * CYCTIME);
   
   if ((!VRT ? uniform(&seed) : unifSim[13]) <= pat->pCensor)
   {
   Process_Death(CENSORED, pat);		//patient is censored
   goto end;
   }
   }
   */
  
  FRED_VERBOSE(1, "HIV Process_patient1 Finished. Day: %d \n", today);
  
} // end of process_patient1(int today, int exposure_date)

/***************************************************/
/***************************************************/

void HIV_Infection::process_patient2(int today, int exposure_date)
{
  /***** patient may stop the regimen for his own reasons...***/
  
  if (this->haart){
    intol_rate = stop_reg;
    
    //if they've previously been intolerant, they are more likely to be intolerant again
    if (this->historyOfIntolerance) intol_rate = intol_rate * intolMultiplier;
    
    this->pstopreg = 1 - exp (-intol_rate * CYCTIME);
    
    //if patient stops regimen, then we send him to Change_Regimen to change the HIV treatment
    //New - we only change the regimen if pat has not exhausted all regimens.
    
    double tempseed4 = uniform(&HIV_Natural_History::seed);
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, " 4  Uniform_seed= %f \t", tempseed4 );
    
    if ((!VRT ? tempseed4 : 0) <= this->pstopreg){
      //getchar();
      //if (KIMTEST) fprintf (kim_test, "\n\n MINAAA seed = %.4ld \t uniform(&Disease::seed)= %.4f.  \n\n ", Disease::seed, uniform(&Disease::seed));
      //If the patient has not already exhausted all regimens, change to a new reg
      if (!this->exhausted_clean_regs){
        if (REASONS_FOR_REG_CHANGES){
          for (int reg=0; reg<3; reg++) if (this->numreg-1==reg) regChangeIntol[reg]++;
        }
        
        if (VLCD4GRAPH)	regChangeIntolFlag = true;
        
        //if (MARK_ROBERTS_AHRQ && pat->numreg==1 && pat->cyclenum <= TIMEPOINT * CYCLES_PER_YEAR)
        //tot_intolFirstReg++;
        
        if (!this->historyOfIntolerance) this->historyOfIntolerance = true;
        
        if (KIMTEST) fprintf (kim_test, "\nChanging Regimen due to intolerance at cycle %d.  \n", today);
        
        decrementForIntolerance();
        
        //mark that patient changed reg due to intol (didn't fail)
        lastRegFailed = false;
        
        Change_Regimen(today);
        //goto beginNewCycle;     //MINA : if patient changes regimen, we do not need to follow through the rest!!
        this->break_process_patient2= true;
      }
    }
  }
  FRED_VERBOSE(1, "HIV Process_patient2 Finished. Day: %d \n", today);
  
} //end of process_patient2


/*******************************************************************************************************
 *****************************************************************************************************/

void HIV_Infection::process_patient3(int today, int exposure_date){
  
  /*************************************************************************************
   kn new v2.0
   If the patient's VL has risen above vl_regimen_failure or the CD4 meets WHO criteria for reg failure,
   the regimen has failed and a new regimen must be chosen.
   Note:  The VL threshold for regimen failure is subject to change after the 1st time the patient has had regimen failure
   **************************************************************************************/
  

  
  if (this->haart && today == this->nextMonCycle /* || (DO_CLINICAL_VISIT_EVERY_SIX_MONTHS_FOR_WHO && pat->cyclenum == sixMonthsSinceLastVisit)*/ )
  {
    //maxCD4onReg is used to trigger a regimen change if running resource-poor  //mina check this: because we dont have resource-poor
    if (this->haart && today == this->nextMonCycle && this->CD4real > this->maxCD4onReg) this->maxCD4onReg = this->CD4real;
    
    //if (DO_CLINICAL_VISIT_EVERY_SIX_MONTHS_FOR_WHO) //mina we dont want this. set to 0 in constant.h
    /* {
     incrementWHOArray(pat, numPatVisits);
     if (KIMTEST) fprintf (kim_test, "Clinic visit at cycle: %d\n", pat->cyclenum);
     sixMonthsSinceLastVisit += CYCLES_PER_YEAR/2;
     }
     */
    
    //If patient has been on regimen for at least six months (per WHO Guidelines) and we meet the reg change trigger
    //Note:  Don't check the trigger unless it's been six months because the VL threshold changes after the first failure
    
    if (this->counterreg >= .5 * CYCLES_PER_YEAR){
      
      //Note:  We use (sixMonthsSinceLastVisit - CYCLES_PER_YEAR/2.0) because we already incremented sixMonthsSinceLastVisit above.  Now we need the original value again.
      if (today == this->nextMonCycle && metTrigger(trig)) {
     
        //censor patients who would have failed - used for resource poor (REMOVED)
        /*if (CALIBRATE){
          this->pCensor = PROB_CENSOR_INSTEAD_OF_FAIL;
          
          if ((!VRT ? uniform(&HIV_Natural_History::seed) : 0 ) <= this->pCensor)
          {
            //MINA check: CALIBRARE is off for now
            //Process_Death(CENSORED, pat);		//patient is censored
            //goto end;     //MINA !!!!
          }
        }
        */
      
        if (REASONS_FOR_REG_CHANGES){
          for (int reg=0; reg<3; reg++) if (this->numreg-1==reg) regChangeFail[reg]++;
        }
        
        if (VLCD4GRAPH) regChangeFailFlag = true;
        
        //If pat wasn't resistant, they must have been intolerant
        //If they aren't already intolerant to any drugs in the reg, decrement for intolerance
        this->totalintol=0;
        for (int i=0; i< DRUGS_IN_REG; i++) if (this->reg[i]->intol) this->totalintol++;
        if (this->totalres == 0 && this->totalintol == 0) decrementForIntolerance();
        
        //mark that patient failed reg (used in Change_Regimen)
        lastRegFailed = true;
        
        Change_Regimen(today);
        //goto beginNewCycle;   //MINA : if patient changes regimen, we do not need to follow through thr rest!! CHANGED
        this->break_process_patient3= true;
      }
    }
    
    if (!break_process_patient3) setNextMonitorCycle(today);    //if break_process_patient3 == true we skip this
    
  }//end section on regimen failure
  
  FRED_VERBOSE(1, "HIV Process_patient3 Finished. Day: %d \n", today);
}//    end process patient 3

/*******************************************************************************************************
 *****************************************************************************************************/

void HIV_Infection::process_patient4(int today, int exposure_date)
{
  //if patient is on haart, we'll have to check for mutations and then possibly resistance
  //if not on haart, we'll just send them right to the next cycle
  if (this->haart){
    Check_For_Mutations();
  }
  
 // if (this->host->get_id() == Global::TEST_ID) fprintf (mina_test, "Process_patient_4 called \n");
  
  //this->real_CD4_year[(int)(today/CYCLES_PER_YEAR)] = this->CD4real;
  //if (today <365) this->real_CD4_day[today] = this->CD4real;
  //if (today <365) this->real_VL_day[today] = this->VLreal;
  
  //if (this->host->get_id() == Global::TEST_ID) fprintf (mina_test, "real_CD4_day %d = %f  \n", today, this->real_CD4_day[today]);
  
  FRED_VERBOSE(1, "HIV Process_patient4 Finished. Day: %d \n", today);
  
} //end for Process_Patient4

/*******************************************************************************************************
 *****************************************************************************************************/
/*Each cycle, we determine whether the patient is compliant to each drug in the regimen.  Each drug is evaluated individually.  Then, if the patient is found to be non-compliant to any drug in the reg, their probability of being non-compliant to the other drugs is increased to ASSOC_COMP.
 
 When we set the compliance, we want to be able to specify the overall resulting compliance value.  This function calculates the compliance probability to use in the first statement above, the individual compliance probability.*/

void HIV_Infection::setPatIndividualDrugCompliance(){
  double C;
  double x;
  double y;
  double a,b,c,F,G,Q,R,S;
  
  //If the probability of non-compliance (1-pat->comp) is set to a value greater than the assoc_comp, then the assoc_comp doesn't need to be used.  Remember, if the patient is found to be non-compliant to any drug, then there is an increased likelihood (equal to assoc_comp) of the patient being non-compliant to the other drugs in the reg.  If assoc_comp is less than the probability of non-compliance, we just use the regular compliance value as the individual drug compliance.
  
  if ((1-this->comp) >= assoc_comp)
  {
    this->individal_drug_comp = this->comp;
  }
  else
  {
    C = 1 - this->comp;
    y = assoc_comp;
    
    a = -2 - y;
    b = 1 + 2*y;
    c = -1 * C;
    
    Q = (pow(a,2) - 3*b) / 9;
    
    R = (2*pow(a,3) - 9*a*b + 27*c) / 54;
    
    S = pow(Q,3) - pow(R,2);
    
    if (S>0)
    {
      printf ("Unanticipated math issue in setPatIndividualDrugCompliance() function.  Call Kim!\n");
      exit(1);
    }
    
    F = -1 * R/fabs(R) * pow((fabs(R) + sqrt(-1*S)),1.0/3.0);
    
    G = Q / F;
    
    x = F + G - a/3;
    
    this->individal_drug_comp = 1 - x;
  }
  if (KIMTEST) fprintf(kim_test, "\n comp = %f     assoc_comp = %f      individal_drug_comp: %f\n ", this->comp, assoc_comp, this->individal_drug_comp);
  
  
}   // end of setPatIndividualDrugCompliance

/*******************************************************************************************************
 *****************************************************************************************************/
//Example:  RandomNumber(8) will return a random number between 0 and 7.
int HIV_Infection::RandomNumber(int upperRange){
  
  double uniform_RandomNumber = uniform_a_b(0, upperRange-.0000000001, &HIV_Natural_History::seed);
  // fprintf(mina_test, "\nuniform_RandomNumber: %f", uniform_RandomNumber);      //mina
  //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n 9  Uniform_seed= %f \t", uniform_RandomNumber );
  
  //this line has been replaced by the following:  return (rand() % upperRange);
  return (int)floor(uniform_RandomNumber);
}

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::Process_Initial_Mutations(){
  int mutType, numMuts, drugNum;
  double pmutres;
  
  //int arv_types = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_arv_types();
  
  for (mutType=0; mutType<this->arv_types; mutType++)
  {
    numMuts = this->mut_arv[mutType];
    
    //Find the probability that at least one drug in the class is resistant
    pmutres = 1 - pow((1-this->pmutres[mutType]), numMuts);
    
    if ((!VRT ? uniform(&HIV_Natural_History::seed) : 0 /*unifSim[kludge_counter++]*/) <= pmutres)
    {
      if (KIMTEST) fprintf (kim_test, "Initial mutation to class %s is resistant.  ", this->className[mutType]);
      if (this->host->get_id() == Global::TEST_ID) fprintf (mina_test, "Initial mutation to class %s is resistant.  ", this->className[mutType]);
      //Make sure there are drugs in the class, if so, pick one at random to mark as resistant
      if (this->num_in_class[mutType] >0)
      {
        drugNum = RandomNumber(this->num_in_class[mutType]);
        if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "  used RandomNumber in Process_Initial_Mutations \t");
        
        drugs[mutType][drugNum].res = true;
        
        if (KIMTEST) fprintf (kim_test, "Choosing drug in class at random to mark as resistant:  %s.%d\n", this->className[mutType], drugNum );
      }
      
      //Check for cross-resistance and cross-class resistance.
      checkForCrossResAndCrossClassRes(mutType);
    }
  }
  
  if (REG_INFO_FOR_SENSITIVITY)
  {
    for (int drugclass=0; drugclass<ARV_TYPES; drugclass++)
    {
      total_res_after_initial_muts[drugclass] += this->num_res[drugclass];
    }
  }
}   // end of Process_Initial_Mutations

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::checkForCrossResAndCrossClassRes(int mutType){
  for (int c=0; c<this->arv_types; c++)
  {
    for (int d=0; d<this->num_in_class[c]; d++)
    {
      //			//Check for cross-resistance
      //#ifdef MONITORING_PAPER_SENSITIVITY
      //			if(floor((double)runnum/2) == 4) pat->pcrossres[mutType] *= sens[4][runnum % 2];
      //#endif
      if (c==mutType && !drugs[c][d].res && (!VRT ? uniform(&HIV_Natural_History::seed) :
                                             0 /*unifSim[kludge_counter++]*/) <= this->pcrossres[mutType])
      {
        //				//assign resistance to found drug
        drugs[c][d].res = true;
        //
        if (KIMTEST)
        {
          fprintf (kim_test, "Developed cross resistance to %s.%d\n", this->className[mutType], d);
          if (this->haart) fprintf (kim_test, "Res profile: (%d %d %d)  \n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
        }
      }
      
      //Check for cross class resistance
      if (c == this->crossClassType[mutType] &&
          !drugs[c][d].res &&
          (!VRT ? uniform(&HIV_Natural_History::seed) : 0 /*unifSim[kludge_counter++]*/) <= this->pCrossClassRes[mutType] )
      {
        //
        //				//assign resistance to found drug
        drugs[c][d].res = true;
        //
        if (KIMTEST)
        {
          fprintf (kim_test, "Developed (cross-class) resistance to %s.%d\n", this->className[this->crossClassType[mutType]], d);
          if (this->haart) fprintf (kim_test, "Res profile: (%d %d %d)  \n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
        }
      }
    }
  }
  
  if (KIMTEST) printNumRemaining();
}   //end of checkForCrossResAndCrossClassRes

/*******************************************************************************************************
 *****************************************************************************************************/

void HIV_Infection::printNumRemaining(){
  if (KIMTEST) fprintf (kim_test,"\nRes\tIntol\tClass Name\n");
  
  for (int c=0; c<this->arv_types; c++)
  {
    for (int d=0; d<this->num_in_class[c]; d++)
    {
      if (KIMTEST) fprintf (kim_test, "%d\t%d\t%s.%d\n", drugs[c][d].res, drugs[c][d].intol, this->className[c], d );
    }
    if (KIMTEST) {
      if (this->num_in_class[c] !=0) fprintf (kim_test, "\n");		//if there were no drugs in the class, prevent two endlines
      
    }
  }
}   //end of printNumRemaining


/*******************************************************************************************************
 *****************************************************************************************************/
/*Puts a patient on haart and initializes all relevent parameters.
 This code used to be in Process_Patient but was put into a function so that we could easily call it if
 we want to start patients in the model already on treatment.  We need to do this now when we generate
 rate files for the transmission model.
 */
void HIV_Infection::start_haart(int today){
  
  if (SCREEN && this->diagnosed == false) {
    FRED_VERBOSE(0, "ERROR in start haart : Patient is not diagnosed().")
    exit(0);
  }
  
  //    ofstream test1;
  //    test1.open ("test1_start_haart.txt");
  //    test1 << "test11" << today <<  "\n";
  //    test1.close();
  
  this->started_haart = true;
  this->haart = true;
  
  //Assign patient to initial regimen.  If the user has specified a situation where there aren't
  //enough drugs available to make up the initial regimen, print a message and exit.
  if (notEnoughDrugsForInitialReg())
  {
  		cout<<"User specified impossible initial regimen.  Exiting."<<endl;
  		if (KIMTEST) fprintf (kim_test, "User specified impossible initial regimen.  Exiting.\n");
  		exit(0);
  }
  else{
  		this->reg[0]=&drugs[this->init_reg[0]][0];
  		this->reg[1]=&drugs[this->init_reg[1]][0];
  		this->reg[2]=&drugs[this->init_reg[2]][0];
    
  		patRegCombo=0;			//see explanation of bestReg in hiv.h
  }
  
  //set start-of-reg values for CD4within
  this->AgeRegBaseline = host->get_age();
  this->CD4RegBaseline = this->CD4real;
  this->VLatStartOfHaart = this->VLreal;
  this->CD4atStartOfHaart = this->CD4real;
  
  if (KIMTEST) fprintf (kim_test, "Start the patient on haart!  Cycle: %d, luck2: %.2f, Regimen:  ", today, this->luck2);
  printDrugsInReg();
  
  // set VL decrement based on initial regimen
  getVLdec();
  if (KIMTEST) fprintf (kim_test, "VLdec is now %.1f\n", this->VLdec);
  
  if (KIMTEST) fprintf(kim_test, "Res profile: (%d %d %d)  \n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
  
  this->haart_start_time = today;
  //this->cyclenum; //mina check : should this be days past exposure instead of 'today'?
  //mina : based on the usage of 'haart_start_time in the code, I think 'today' is correct
  
  setNextMonitorCycle(today);					//figure out when to monitor CD4 or VL.
  
  /*	if (DO_CLINICAL_VISIT_EVERY_SIX_MONTHS_FOR_WHO)
   {
   incrementWHOArray(pat, numPatVisits);
   if (KIMTEST) fprintf (kim_test, "Clinic visit at cycle: %d\n", pat->cyclenum);
   sixMonthsSinceLastVisit = pat->haart_start_time + CYCLES_PER_YEAR/2;
   }
   */
  
  //When starting with initial muts, it's possible that the patient was initially resistant to drugs in the initial reg.
  //If this is the case, immediately change the regimen.  Only do this for research rich, because
  //with resource poor, we don't have the resistance testing to know.
  if (RESOURCE_RICH && (this->reg[0]->res || this->reg[1]->res|| this->reg[2]->res))
  {
  		if (KIMTEST) fprintf (kim_test, "Patient resistant to drug in initial reg.  Changing reg.\n");
    
  		Change_Regimen(today);
  }
  
} //end of start_haart

/*******************************************************************************************************
 *****************************************************************************************************/
//Sometimes with testing I want to set the number of drugs in a certain drug class to 0.
//This function checks to see if there are enough drugs to make the initial regimen.
bool HIV_Infection::notEnoughDrugsForInitialReg(){
  bool changeFirstReg=false;
  
  Get_Num_Each_Type_In_Array(DRUGS_IN_REG, this->init_reg, numEachTypeinReg);
  
 	for (int i = 0; i < DRUGS_IN_REG; i++)
  {
    if (numEachTypeinReg[this->init_reg[i]] > this->num_in_class[this->init_reg[i]]) changeFirstReg = true;
  }
  
  return changeFirstReg;
}   //enf of notEnoughDrugsForInitialReg

/*****************************************************************************************
 This module takes in a thisient who needs to change drug regimens.
 ********************************************************************************************/
void HIV_Infection::Change_Regimen(int today){
  int reg=999;	//initialize to nonsense number
  
  reg = (RESOURCE_RICH ? ChooseRegimen_Rich() : ChooseRegimen_Poor());
  
  //set next cycle to monitor CD4 or VL
  setNextMonitorCycle(today);
  
  //if the thisient just ran out of regimens
  if (!this->exhausted_clean_regs && reg == MAINTAIN_CURRENT_REG )
  {
    this->exhausted_clean_regs = true;
    this->haart_exhausted_clean_regs_time = today;    //mina check
    
    if (KIMTEST) fprintf(kim_test, "\n\n***Cycle: %d:   this PATIENT EXHAUSTED ALL REGIMENS!***\n\n", today);
    
    if (REG_INFO_FOR_SENSITIVITY || ACTIVE_HAART_INFO)      // Mina: REG_INFO_FOR_SENSITIVITY is used for one-way sensitivity analyses
    {
      this->num_regs_before_salvage = this->numreg;
      this->num_res_drugs_before_salvage = countResDrugs();
    }
    
    //		if (CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_SALVAGE)
    //		{
    //			CD4atStartOfSalvage[numStartedSalvage] = this->CD4real;
    //			VLatStartOfSalvage[numStartedSalvage] = this->VLreal;
    //			numStartedSalvage++;
    //		}
    
    //If the resource rich thisient just ran out of regs, but they are resistant to 2 or more drugs, call this function again
    if (RESOURCE_RICH && this->totalres>=2) reg = ChooseRegimen_Rich();
    
    //If the resource poor thisient just ran out of clean regs, but they are resistant to 1 or more drugs, call this function again
    if (!RESOURCE_RICH && this->totalres>=1) reg = ChooseRegimen_Poor();
  }
  
  if (this->exhausted_clean_regs && reg == MAINTAIN_CURRENT_REG)
  {
    if (KIMTEST)
    {
      fprintf(kim_test, "*Cycle: %d:   Patient remaining on current regimen.  ", today);
      printDrugsInReg();
      fprintf(kim_test, "Res profile: (%d %d %d)\n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
    }
  }
  
  if (reg==NEW_REG)
  {
    //We increase the regimen number
    //Note, in the case that we are changing the regimen due to initial mutatiions, we don't
    //want to increase the regimen number (since the thisient never really went on that reg)
    if (!((today==0 && this->counterreg==0) || notEnoughDrugsForInitialReg())) this->numreg++;			//increase reg number
    
    this->counterreg = 0;	//reset number of cycles on current reg
    this->cyclesOnReg[this->numreg-1] = 0;	//-1 since we already incremented numreg
    
    //Get the new VLdec according to the drugs in the new regimen
    getVLdec();
    
    //set start-of-reg values for CD4within
    this->AgeRegBaseline = host->get_age();
    
    //Only reset the CD4 baseline if the last regimen had failed (not for intol)
    if (lastRegFailed) this->CD4RegBaseline = this->CD4real;
    
    //luck2 represents regimen-specific randomness.  Since we are changing the reg, change luck2.
    this->luck2 = (!VRT ? normal(0,1,&HIV_Natural_History::seed) : 0 /*unifSim[4]*/);
    
    //with a new regimen, we consider the possibility that this thisient's compliance thistern will differ for this next regimen
    double tempseed7=uniform(&HIV_Natural_History::seed);
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n 7  Uniform_seed= %f \t", tempseed7 );
    
    this->diligence = (!VRT ? tempseed7 : 0 /*unifSim[5]*/);
    
    //initialize the max CD4 for this new regimen to the current CD4 count
    this->maxCD4onReg = this->CD4real;
    
    // now reset comp vars
    if (KIMTEST) fprintf(kim_test, "\n diligence = %f    comp before = %f \t", this->diligence, this->comp );
    if (comp >= .5)
      this->comp = COMP + (1-COMP)*(this->diligence - .5);
    else
      this->comp = COMP + (0-COMP)*(this->diligence - 0.5);
    if (KIMTEST) fprintf(kim_test, "comp after = %f \n", this->comp);
    
    
    
    //recalculate the this->individualDrugComp based on the new this->comp
    setPatIndividualDrugCompliance();
    
    //for the clinical trigger, reset whether the thisient has had an AIDS-defining
    //even yet during this regimen
    this->AIDSeventReg = false;
    
    if (KIMTEST)
    {
      fprintf (kim_test, "Changing to Reg %d at Cycle %d.\n", this->numreg, today);
      fprintf (kim_test, "New Reg:  ");
      printDrugsInReg();
      fprintf (kim_test, "Res profile: (%d %d %d)  \n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
      fprintf (kim_test, "Round diligence = %.2f:  Round compliance = %.2f\n", this->diligence, this->comp);
      fprintf (kim_test, "Assigned luck2: %.2f\n", this->luck2);
      fprintf (kim_test, "VLdec is now %.1f\n", this->VLdec);
    }
    
    //    if ( ANNALS_REGS_MUTS )     //mina: I think I dont need this...  ???
    //    {
    //      if (today <= CYCLES_PER_YEAR * 5)	totalNumRegs1st5years++;
    //      if (today <= CYCLES_PER_YEAR * 10)  totalNumRegs1st10years++;
    //    }
  }
  
}//end Change_Regimen


/*******************************************************************************************************
 *****************************************************************************************************/

void HIV_Infection::printDrugsInReg(){
  for (int i=0; i<DRUGS_IN_REG; i++)
  {
    if (KIMTEST) fprintf (kim_test, "%s.%d  ", this->className[this->reg[i]->classNum], this->reg[i]->drugNum);
  }
  if (KIMTEST) fprintf (kim_test, "\n");
}

/*******************************************************************************************************
 *****************************************************************************************************/
//The values for the VL decrements come from previous research.  Those values are true for an hiv_baseline of 4.5.  Thus, we scale the VL dec according to the patient's HIVbaseline.
void HIV_Infection::getVLdec(){
  switch (this->reg[0]->classNum)
  {
    case (PI_SINGULAR):
      this->VLdec = VL_DEC_PI_SINGULAR * this->HIVbaseline / 4.5 + DVLHAART;
      break;
    case (PI_BOOSTED):
      this->VLdec = VL_DEC_PI_BOOSTED * this->HIVbaseline / 4.5 + DVLHAART;
      break;
    case (NNRTI_EFAVIRENZ):
      this->VLdec = VL_DEC_EFAVIRENZ * this->HIVbaseline / 4.5 + DVLHAART;
      break;
    case (NNRTI_NEVIRAPINE):
      this->VLdec = VL_DEC_NEVIRAPINE * this->HIVbaseline / 4.5 + DVLHAART;
      break;
  }
  
  //	if (MARK_ROBERTS_AHRQ)
  //	{
  //		pat->VLdec *= MARK_VL_DEC_MULTIPLIER;
  //	}
  //#ifdef MONITORING_PAPER_SENSITIVITY
  //	if(floor((double)runnum/2) == 2) pat->VLdec *= sens[2][runnum % 2];
  //#endif
}

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::setNextMonitorCycle(int today)
{
  int cyclesInInterval;
  int rand;
  
  //interval is in months and CYCTIME is cycles/month, result of division is cycles
  cyclesInInterval = int(monitor_interval/12.0*CYCLES_PER_YEAR);
  rand = (!VRT ? RandomNumber(cyclesInInterval): 0 /*(int)unifSim[9]*/);
  
  if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "  used RandomNumber in setNextMonitorCycle \n");
  if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "  monitor_interval=%d \t cyclesInInterval= %d \t", monitor_interval, cyclesInInterval);
  this->nextMonCycle = int(cyclesInInterval/2) + rand + today;    //MINA CHECK today or days_post_exposure
  
}  //end of set NextMonitor Cycle

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::Get_Num_Each_Type_In_Array(int numInArray, int drugArray[], int numEachType[]){
  int i, j;
  
  //initialize
  for (i = 0; i < ARV_TYPES; i++)
    numEachType[i] = 0;
  
  for (i = 0; i < numInArray; i++)
  {
    //for each drug, we find the arv type of this drug and increment the
    //appropriate counter
    for (j = 0; j < ARV_TYPES; j++)
    {
      if (drugArray[i] == j)
        numEachType[j]++;
    }
  }
}

/*******************************************************************************************************
 *****************************************************************************************************/
int HIV_Infection::ChooseRegimen_Poor(){
  return 0;
}

/*******************************************************************************************************
 *****************************************************************************************************/
/*Possible regimens
 
 NNRTI_EFAVIRENZ			NNRTI_NEVIRAPINE
 TAM + NONTAM			TAM + NONTAM
 2 NONTAM				2 NONTAM
 
 PI_BOOSTED				PI_SINGULAR
 TAM + NONTAM			TAM + NONTAM
 2 NONTAM				2 NONTAM
 */

/*
 A thisient starts out on their initial regimen.  When they become intolerant or meet the trigger to change the reg, we
 choose a new regimen.  Initially, we choose a new regimen with no resistant or intolerant drugs.  If a new "clean" reg
 (one that contains no resistant or intolerant drugs) cannot be built, the thisient stays on their last regimen.
 They can change off of that regimen only if they have developed resistance to two or more drugs in the regimen.
 If this happens, we put them on a reg with the least number of resistant drugs regardless of intolerance.
 */

int HIV_Infection::ChooseRegimen_Rich(){
  int tReg[3]={0,0,0};					//tReg stands for test regimen!
  int remReg[NUM_CLASS_COMBINATIONS][3];	//remReg stands for "remember reg"!
  int regimenIndex;
  int numGoodRegs = 0;
  int pickRegOutOfHat;
  int numEachTypeinReg[ARV_TYPES];
  int numRes = 3;
  int numIntol = 3;
  int leastNumRes;
  int leastNumIntol = 9999;			//initialize to very high num
  int greatestVLDec = -1;				//initialize to very low num
  int typeofleastnumres;
  bool retVal;
  int n1, n2, n3;
  bool addRegToList;
  
  //initialize
  leastNumRes = this->totalres;
  
  //Note that the regimen list is in order of most effective VLdec to least effective.
  for (regimenIndex = 0; regimenIndex < NUM_CLASS_COMBINATIONS; regimenIndex++)
  {
    switch (regimenIndex)
    {
      case (0): tReg[0]= NNRTI_EFAVIRENZ; tReg[1]= NRTI_TAM; tReg[2]= NRTI_NONTAM;  break;
      case (1): tReg[0]= NNRTI_EFAVIRENZ; tReg[1]= NRTI_NONTAM; tReg[2]= NRTI_NONTAM;  break;
      case (2): tReg[0]= PI_BOOSTED; tReg[1]= NRTI_TAM; tReg[2]= NRTI_NONTAM;  break;
      case (3): tReg[0]= PI_BOOSTED; tReg[1]= NRTI_NONTAM; tReg[2]= NRTI_NONTAM;  break;
      case (4): tReg[0]= NNRTI_NEVIRAPINE; tReg[1]= NRTI_TAM; tReg[2]= NRTI_NONTAM;  break;
      case (5): tReg[0]= NNRTI_NEVIRAPINE; tReg[1]= NRTI_NONTAM; tReg[2]= NRTI_NONTAM;  break;
#if CALIBRATE
      case (6): tReg[0]= PI_SINGULAR; tReg[1]= NRTI_TAM; tReg[2]= NRTI_NONTAM;  break;
      case (7): tReg[0]= PI_SINGULAR; tReg[1]= NRTI_NONTAM; tReg[2]= NRTI_NONTAM;  break;
#endif
    }
    
    //Get the number of drugs of each class in the regimen being considered (This is set in array numEachTypeinReg[])
    Get_Num_Each_Type_In_Array(DRUGS_IN_REG, tReg, numEachTypeinReg);
    
    //initialize
    addRegToList = false;
    
    //first scenario:  this is still on active haart
    if (!this->exhausted_clean_regs)
    {
      //We make sure there are enough drugs of each type that the this is not res or intolerant to
      n1 = numNotResOrIntol(tReg[0]);
      n2 = numNotResOrIntol(tReg[1]);
      n3 = numNotResOrIntol(tReg[2]);
      
      if((n1 >= numEachTypeinReg[tReg[0]] &&
          n2 >= numEachTypeinReg[tReg[1]] &&
          n3 >= numEachTypeinReg[tReg[2]] )
         &&
         
         //Save the first eligible reg (because it will have the highest VLdec)
         //And also save any reg with the same first drug
         
         //**Just added the "OR CALIBRATE" because during calibration we will have a different first regimen
         //and subsequent regimens will be randomly chosen from all those not resistant or intolerant
         
         (numGoodRegs==0 ||
          (numGoodRegs > 0 && !CALIBRATE && tReg[0] == remReg[0][0])
          || CALIBRATE))
      {
        numGoodRegs++;
        addRegToList = true;
      }
    }
    
    //this has exhausted all regs
    else
    {
      
      //We make sure there are enough drugs of each type that the this is not intolerant to
      //We ignore intolerance if the thisient is resistant to two or more drugs
      if(	(this->totalres<2 &&
           numNotIntol(tReg[0]) >= numEachTypeinReg[tReg[0]] &&
           numNotIntol(tReg[1]) >= numEachTypeinReg[tReg[1]] &&
           numNotIntol(tReg[2]) >= numEachTypeinReg[tReg[2]] )
         ||
         this->totalres>=2)
      {
        numRes =
        max(0, (numEachTypeinReg[tReg[0]] - numNotRes(tReg[0]))) +
        max(0, (numEachTypeinReg[tReg[1]] - numNotRes(tReg[1]))*(tReg[0]!=tReg[1])) +
        max(0, (numEachTypeinReg[tReg[2]] - numNotRes(tReg[2]))*(tReg[2]!=tReg[1] && tReg[2]!=tReg[0]));
        
        if (numRes < leastNumRes)
        {
          numGoodRegs=1;				//this will be the first good reg - it gets array index 0
          leastNumRes = numRes;
          typeofleastnumres = tReg[0];
          addRegToList = true;
        }
        
        else if (numRes == leastNumRes && numRes < this->totalres &&
                 (CALIBRATE || (numRes == leastNumRes && tReg[0] == typeofleastnumres)))
        {
          numGoodRegs++;
          addRegToList = true;
        }
      }
    }   //end of else
    
    if (addRegToList)
    {
      remReg[numGoodRegs-1][0] = tReg[0];
      remReg[numGoodRegs-1][1] = tReg[1];
      remReg[numGoodRegs-1][2] = tReg[2];
    }
  }
  
  //This chunk randomly chooses a regimen out of all the regimens saved.
  if (numGoodRegs > 0)
  {
    retVal = NEW_REG;				//a new regimen was available
    pickRegOutOfHat = (!VRT ? RandomNumber(numGoodRegs) : 0 /*(int)unifSim[6]*/);
    
    //put thisient on that specific regimen
    putPatOnReg(remReg[pickRegOutOfHat]);
  }
  
  else retVal = MAINTAIN_CURRENT_REG;		//no reg available
  
  return retVal;
} //end of ChooseRegimen_Rich

/*******************************************************************************************************
 *****************************************************************************************************/
int HIV_Infection::countResDrugs()
{
  int numResDrugs = 0;
  
  for (int c = 0; c < ARV_TYPES; c++)
  {
    for (int d=0; d<this->num_in_class[c]; d++)
    {
      numResDrugs += drugs[c][d].res;
    }
  }
  
  return numResDrugs;
}

/*******************************************************************************************************
 *****************************************************************************************************/
int HIV_Infection::numNotResOrIntol(int type)
{
  int numNotResOrIntol=0;
  
  for (int i=0; i<this->num_in_class[type]; i++)
  {
    if (!drugs[type][i].intol && !drugs[type][i].res) numNotResOrIntol++;
  }
  return numNotResOrIntol;
}

/*******************************************************************************************************
 *****************************************************************************************************/
int HIV_Infection::numNotIntol(int type)
{
  int numNotIntol=0;
  
  for (int i=0; i<this->num_in_class[type]; i++)
  {
    if (!drugs[type][i].intol) numNotIntol++;
  }
  return numNotIntol;
}

/*******************************************************************************************************
 *****************************************************************************************************/
int HIV_Infection::numNotRes (int type)
{
  int numNotRes=0;
  
  for (int i=0; i<this->num_in_class[type]; i++)
  {
    if (!drugs[type][i].res) numNotRes++;
  }
  return numNotRes;
}

/*******************************************************************************************************
 *****************************************************************************************************/
//put patient on that specific regimen
void HIV_Infection::putPatOnReg(int *type)
{
  int drugNum;
  
  for (int i=0; i<DRUGS_IN_REG; i++)
  {
    drugNum = giveMeBestDrugInClass( *(type+i),  i);
    this->reg[i]=&drugs[type[i]][drugNum];
  }
}

/*******************************************************************************************************
 *****************************************************************************************************/
/*For a given drug class, this function returns the next available "best" drug in the class.
 The order of preference is:
 1)  Neither res nor intol
 2)  Intol but not res
 3)  Res but not intol
 4)  Both res and intol
 */
int HIV_Infection::giveMeBestDrugInClass(int type,  int numAssignedSoFar){
  int drugFound=NONE_AVAILABLE;
  
  for (int i=0; i<this->num_in_class[type] && drugFound==NONE_AVAILABLE; i++)
  {
    if (!drugs[type][i].res && !drugs[type][i].intol)
    {
      drugFound=i;
      
      for (int j=0; j<numAssignedSoFar; j++)
      {
        if (type==this->reg[j]->classNum && i==this->reg[j]->drugNum) drugFound=NONE_AVAILABLE;
      }
    }
    
  }
  
  if (drugFound==NONE_AVAILABLE)
  {
    for (int i=0; i<this->num_in_class[type] && drugFound==NONE_AVAILABLE; i++)
    {
      if (!drugs[type][i].res)
      {
        drugFound=i;
        
        for (int j=0; j<numAssignedSoFar; j++)
        {
          if (type==this->reg[j]->classNum && i==this->reg[j]->drugNum) drugFound=NONE_AVAILABLE;
        }
      }
    }
  }
  
  if (drugFound==NONE_AVAILABLE)
  {
    for (int i=0; i<this->num_in_class[type] && drugFound==NONE_AVAILABLE; i++)
    {
      if (!drugs[type][i].intol)
      {
        drugFound=i;
        
        for (int j=0; j<numAssignedSoFar; j++)
        {
          if (type==this->reg[j]->classNum && i==this->reg[j]->drugNum) drugFound=NONE_AVAILABLE;
        }
      }
    }
  }
  
  if (drugFound==NONE_AVAILABLE) drugFound=0;
  
  return drugFound;
}   //end of HIV_Infection::giveMeBestDrugInClass

/*******************************************************************************************************
 Returns a cost for the month.  Based on Sanders et al (NEJM 2005).  There they estimated the following annual
 costs (which I convert to monthly values in the Cost() function):
 HIV infection, CD4 > 500: $2,978
 HIV infection, CD4 between 200 and 500: $5,096
 HIV infection, CD4 < 200: $7,596
 Cost of AIDS (irrespective of CD4 count): $10,998 (we are not implementing this yet)
 3-drug therapy: $13,752
 ********************************************************************************************************/

double HIV_Infection::Cost(int whichCost){
  double retval = 0;
  
  if (RESOURCE_RICH){
    
    if (this->CD4real < 200)
      retval += 7596.0/(double)CYCLES_PER_YEAR;
    else if (this->CD4real < 500)
      retval += 5096.0/(double)CYCLES_PER_YEAR;
    else
      retval += 2978.0/(double)CYCLES_PER_YEAR;
    
    if (this->haart)
      retval += 13752.0/(double)CYCLES_PER_YEAR;
  }
  return retval;
}   //end of Cost()

/*******************************************************************************************************
 Returns a value in [0,1] inidicating a utility weight that is based on various patient parameters.
 Based on values from a Freedberg paper.
 ********************************************************************************************************/
double HIV_Infection::Utility()
{
  double retval = 0;
  
  if (this->CD4real < 100)
    retval += UTILITY_CD4_LESS_THAN_100;
  else if (this->CD4real < 200)
    retval += UTILITY_CD4_LESS_THAN_200;
  else
    retval += UTILITY_CD4_ABOVE_200;
  
  if (this->haart) retval += DELTA_UTILITY_WITH_HAART;
  
  return retval;
}

/*******************************************************************************************************
 This procedure steps through each drug the patient is taking and determines if the patient is compliant
 to that drug.  Once noncompliance is observed for any drug, there is an increased chance the patient is
 noncompliant to the other drugs (this accounts for temporal clustering of compliance).  We do not currently
 account for compliance as a function of side effects, pill burden, etc.
 *******************************************************************************************************/
void HIV_Infection::Get_Compliance(){
  int i, found_nc;
  double rand[DRUGS_IN_REG];
  
  //Note that pat->comp now refers to the individual drug compliance that gives an overall
  //compliance of COMP.  This is calculated in
  
  this->pNocomp = 1 - this->individal_drug_comp;
  if (KIMTEST) fprintf(kim_test, "pNocomp= %f", this->pNocomp);   //mina debug
  
  found_nc = 0;  //indicates if non-compliance is observed which triggers the use of the assoc comp
  
  for (i = 0; i < DRUGS_IN_REG; i++)
  {
    double tempseed8=uniform(&HIV_Natural_History::seed);
    this->nc[i] = 0;			//initializing all to false for next loop
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n 8  Uniform_seed= %f \t", tempseed8 );
    rand[i] = (!VRT ? tempseed8 : 0 /*unifSim[10+i]*/);
    
    if (rand[i] <= this->pNocomp)
    {
      this->nc[i] = 1;
      found_nc= 1;
      if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n Found_nc in Get_Compliance. \n");
    }
  }
  
  if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n" );
  if (found_nc)
  {
    for (i=0; i < DRUGS_IN_REG; i++)
    {
      if (rand[i] <= assoc_comp) this->nc[i] = 1;
    }
  }
  
  if (KIMTEST && found_nc && !ONLY_CORE_EVENTS)
  {
    fprintf (kim_test, "\nPatient not compliant to: ");
    for (i = 0; i < DRUGS_IN_REG; i++)
    {
      if (this->nc[i]) fprintf (kim_test, "%s ", this->className[this->reg[i]->classNum]);
    }
    fprintf (kim_test, "\n");
  }
} //end of Get_Compliance()

/*******************************************************************************************************
 *****************************************************************************************************/
double HIV_Infection::addNoise (double input, int type, int cyclenum)      //today or days post exposure? I think Kim means the number of days past infection (cyclenum means number of days that person was added in SCott's model)
{
  
  //These values are much the result of trial and error and make the CD4 look realistic.
  if (type==CD4)
  {
    
    //if (CYCLE==DAY) return (input + 100 * PerlinNoise1D(cyclenum*.002*12,1.5,5,2));	//12 is the value that looks good and looks like the VL
    double temp1 = PerlinNoise1D((cyclenum +offset1)*.002*5,1.5,5,2);
    //    if (KIMTEST) fprintf (mina_test, "\ncycle= %d \t noise = %f", cyclenum, temp1 );
    if (CYCLE==DAY) return (input + 100 * temp1);		//this line give smoother noise than the line above
    //    else if (CYCLE==MONTH) return (input + 200 * PerlinNoise1D(cyclenum*.002,1.5,5,2));
    
  }
  
  else if (type==VL)
  {
    double temp2 = PerlinNoise1D((cyclenum +offset2)*.002*12,1.5,5,2);
    //    if (KIMTEST) fprintf (mina_test, "\ncycle= %d \t noise = %f", cyclenum, temp2 );
    if (CYCLE==DAY) return (input + 1 * temp2);  //the 333 is an offset so the noise for the CD4 doesn't look the same as the VL
    //    else if (CYCLE==MONTH) return (input + .25* PerlinNoise1D(cyclenum*.002,1.5,5,2));
  }
  
  else return 0;
  
}

/*******************************************************************************************************
 Obtains either the viral load category or the CD4 category  according to the "type" input parameter
 *******************************************************************************************************/
int HIV_Infection::Get_Category(int type){
  if (type == VL)
  {
    if (this->VLreal < 3.5)
      return 1;
    else if (this->VLreal < 4.5)
      return 2;
    else if (this->VLreal < 5.5)
      return 3;
    else
      return 4;
  }
  else //type == CD4
  {
    if (this->CD4real < 50)
      return 1;
    else if (this->CD4real < 200)
      return 2;
    else if (this->CD4real < 350)
      return 3;
    else if (this->CD4real < 500)
      return 4;
    else
      return 5;
  }
}

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::getCD4regressionideal(int days_post_exposure)
//if we need today instead of days past exposure: I think we "today" becasue days_post_exposure - this->haart_start_time gives us the duration that person has been on haart regimen
{
  static double total_ended_round1;
  static double total_begin_round2;
  static int num_ended_round1;
  static int num_begin_round2;
  
  if (this->host->get_id() == Global::TEST_ID)
    fprintf(mina_test, "\ncyclenum= %d \t CD4atStartOfHaart= %f \t haart_start_time= %d \t AgeRegBaseline= %f \t CD4RegBaseline= %f \t comp= %f", days_post_exposure, this->CD4atStartOfHaart, this->haart_start_time, this->AgeRegBaseline, this->CD4RegBaseline, this->comp);
  
  //ONE REG
  this->CD4regressionideal =
  
  this->CD4atStartOfHaart +
  
  LUCK_MULTIPLIER * (WEIGHT_PAT_LUCK*this->luck1 + WEIGHT_ROUND_LUCK*this->luck2) * AVG_CD4_SD
  +
  (this->numreg>=1)*(
                     +
                     (37-35)*(((days_post_exposure - this->haart_start_time)/CYCLES_PER_YEAR)>=0)
                     +
                     (33-10)*(((days_post_exposure - this->haart_start_time)/CYCLES_PER_YEAR)>=1)
                     +
                     (20-10)*(((days_post_exposure - this->haart_start_time)/CYCLES_PER_YEAR)>=2)
                     +
                     14*(((days_post_exposure - this->haart_start_time)/CYCLES_PER_YEAR)>=3)
                     +
                     10*(((days_post_exposure - this->haart_start_time)/CYCLES_PER_YEAR)>=4)
                     +
                     0*(this->AgeRegBaseline<30)
                     +
                     26*(this->AgeRegBaseline>=30 && this->AgeRegBaseline<40)
                     +
                     18*(this->AgeRegBaseline>=40 && this->AgeRegBaseline<50)
                     +
                     16*(this->AgeRegBaseline>=50 && this->AgeRegBaseline<60)
                     +
                     6*(this->AgeRegBaseline>=60 && this->AgeRegBaseline<70)
                     +
                     -15*(this->AgeRegBaseline>=70)
                     +
                     1*(this->CD4RegBaseline<=200)
                     +
                     0*(this->CD4RegBaseline>200 && this->CD4RegBaseline<=350)
                     +
                     -6*(this->CD4RegBaseline>350 && this->CD4RegBaseline<=500)
                     +
                     -48*(this->CD4RegBaseline>500)
                     +
                     3.3		//average of med class coefficients
                     +
                     0*(this->comp>0.8)
                     +
                     0*(this->comp>0.6 && this->comp<=0.8)
                     +
                     -6*(this->comp>0.4 && this->comp<=0.6)
                     +
                     -31*(this->comp>0.2 && this->comp<=0.4)
                     +
                     -51*(this->comp<=0.2)
                     +
                     //We decided not to use this line because it benefited patients who started with higher Viral Loads.  (Patients who started with high VLs experience more of a drop in VL, so got more of a benefit in cd4 bump)
                     //-43*(this->VLreal - this->VLatStartOfHaart)
                     //The getVLTerm function scales the size of the VL term for the possible size of VL increase
                     43*getVLTerm()
                     );
  
  //#ifdef MONITORING_PAPER_SENSITIVITY
  //	if(floor((double)runnum/2) == 3) this->CD4regressionideal += sens[3][runnum % 2];
  //#endif
  this->CD4regressionideal = Max (0, this->CD4regressionideal);		//Don't let it fall below zero
}

/*******************************************************************************************************
 *****************************************************************************************************/
//from (y-y0)/(x-x0) = (y1-y0)/(x1-x0) we get
//y = y0 + (x-x0)(y1-y0)/(x1-x0)
double HIV_Infection::linearInterpolate(double x0, double y0, double x1, double y1, double x)
{
  double y;		//the return value
  
  y = y0 + (x-x0)*(y1-y0)/(x1-x0);
  
  return y;
}


/*******************************************************************************************************
 *****************************************************************************************************/
double HIV_Infection::getVLTerm()
{
  const double meanVLdecreaseVACS = 1.348;		//from 2007 AIDS paper by R. Braithwaite
  const double meanBaselineVLVACS = 4.07;			//mean VL baseline from Joyce's VACS data
  double VLdecreasePat, maxVLdecreaseVACS, maxVLdecreasePat, propOfMaxVACS, propOfMaxPat;
  double dec;
  
  //The maximum possible decrease in Viral Load would occur at a vl_fract of 1.0
  //(indicating perfect adherence, no resistance) and on Efavirenz (which has the
  //highest VLdec.  The 4.5 comes from the equation used in the getVLdec function.
  maxVLdecreaseVACS = 1.0 * VL_DEC_EFAVIRENZ * meanBaselineVLVACS / 4.5;
  maxVLdecreasePat  = 1.0 * VL_DEC_EFAVIRENZ * this->HIVbaseline   / 4.5;
  
  //Calculate the drop in VL from start of haart (VLatStartOfHaart should be close
  //to pat->HIVbaseline but could vary somewhat because of imposed noise
  VLdecreasePat = this->VLatStartOfHaart - this->VLreal;
  
  //Calculate the decrease in VL as a proportion of the maximum possible decrease
  propOfMaxVACS = meanVLdecreaseVACS / maxVLdecreaseVACS;
  propOfMaxPat  = VLdecreasePat      / maxVLdecreasePat;
  
  //We take the average decrement seen by the VACS patients and scale up or down
  //according to the ratio of the proportion of max decrease seen by the current patient
  //to the average proportion of max decrease seen by the current patient.
  //dec is the value that should replace (pat->VLatStartOfHaart - pat->VLreal)
  //in the CD4 regression equation
  dec = meanVLdecreaseVACS / propOfMaxVACS * propOfMaxPat;
  
  if (this->host->get_id() == Global::TEST_ID)
    fprintf(mina_test, "\n VLatStartOfHaart= %f \t VLreal= %f \t VLdecreasePat= %f \t HIVbaseline= %f ", this->VLatStartOfHaart, this->VLreal, VLdecreasePat, this->HIVbaseline);
  
  return dec;
} //end of getVLTerm()

/*******************************************************************************************************
 *****************************************************************************************************/
//A single call to HIV_Lookup and then Get_Rate() has been replaced by the following function.
//For patients with low compliance (< 0.5), we combined the off-haart mortality with on-haart
//mortality.  At a compliance of 0, the patient's mortality should be the off-haart mortality and
//at 50% comp, the patient should have the full on-haart mortality.
double HIV_Infection::Get_HIV_Death_Rate()
{
  double deathRateHaart, deathRateOffHaart, deathrateHIV, hiv_index;
  
  if (!this->haart)
  {
    hiv_index = HIV_Lookup(false);
    deathrateHIV = Get_Rate(HIV, hiv_index);
  }
  
  else
  {
    hiv_index = HIV_Lookup(true);
    deathRateHaart = Get_Rate(HIV, hiv_index);
    if (this->comp >= .5) deathrateHIV = deathRateHaart;
    else
    {
      hiv_index = HIV_Lookup(false);
      deathRateOffHaart = Get_Rate(HIV, hiv_index);
      deathrateHIV = linearInterpolate(0, deathRateOffHaart, 0.5, deathRateHaart, this->comp);
    }
  }
  
  if (KIMTEST) fprintf (kim_test, "\thiv_index: %f \t  deathrateHIV: %f.\n", hiv_index, deathrateHIV); //mina test
  return deathrateHIV;
  
}   //end of Get_HIV_Death_Rate()


/*************************************************************************************
 * This obtains the index to use to look up the HIV death rate in the hiv mortality table.
 * The factors that affect this index are whether the patient is on HAART or not, her age
 * category, her length of time since starting haart, her CD4 cat, and her VL cat.
 
 2/24/2010 This function has been modified to received two variables, pat and haart.  When patients have a
 low compliance (< 0.5), we want to find their mortality rate both on and off therapy and then interpolate
 so that the patient gets the full benefit of haart starting at the
 *************************************************************************************/
double HIV_Infection::HIV_Lookup(bool haart)
{
  /*********************************************************************************************************
   * We are changing to the following form which gives everyone the mortality benefit of being on haart
   * even before starting haart.  As Scott mentioned in an e-mail, "this is a conservative assumption that
   * biases the analyses in favor of starting later."  This has to do with "how we really don't know the
   * non-haart mortality rates for those with favorable CD4 and viral loads is."
   * CHANGE: We are no longer doing this.  That was just for a particular set of analyses.  Now, if a pat
   * is not on haart, they get no mortality benefit (first condition).  If they are on haart, then it depends
   * on whether or not there is resistance to all drugs and whether or not we are condisering viral fitness.
   * If there is not complete resistance to all regimens, then the benefit remains.  If there is res to all, and
   * we are considering reduced virulence of resistant strains, then the benefit remains. Otherwise, if there
   * is res to all and we do not consider a weaker strain, we remove the benefit of being on haart at that point.
   
   1st digit:                     4th digit:
   1 Patient is on Haart          1 CD4 < 50
   0 Patient is not on Haart			2 50 <= CD4 < 200
   3 200 <= CD4 < 350
   2nd digit:                     4 350 <= CD4 < 500
   1 age < 40                     5 CD4 >=500
   2 40 < age < 50
   3 age >= 50                     5th digit:
   1 VL < 3.5
   3rd digit:                       2 3.5 <= VL < 4.5
   1 1st year                       3 4.5 <= VL < 5.5
   2 2nd year                       4 VL > 5.5
   3 3rd year
   **************************************************************************/
  int total = 0;
  
  if (haart) total = 10000;	//In it's original form, this line was as follows, but was changed since we no longer use stay_on_haart:  total = 10000 - 10000*(pat->exhausted_clean_regs==1)*(pat->stay_on_haart==0);
  else total=0;
  
  if ( host->get_age() < 39.99 )
    total += 1000;
  else if ( host->get_age() < 49.99)
    total += 2000;
  else
    total += 3000;
  
  /*knv1
   if (pat->cyclenum < CYCLES_PER_YEAR)
   total += 100;
   else if (pat->cyclenum < 2 * CYCLES_PER_YEAR)
   total += 200;
   else
   */
  total += 300;
  
  total += 10*this->CD4cat;
  total += this->VLcat;
  return (double)total;
}

/********************************************************************************************************
 This uses a binary search to find the index such that table[index][0] = lookup and table[index][1] = the rate
 we want.  lookup for the hiv mort table is really an int and for the male and female age tables it's a double,
 but the code covers both types.  In the case that this is an age mort table lookup, the code ends up doing a
 linear interpolation between the two table values.
 ***********************************************************************************************************/
double HIV_Infection::Get_Rate(/*double table[TABLE_SPACE][2], int entries, */int type, double lookup)
{
  int entries;
  //double ** table;
  int first = 0, middle, found = 0, index;
  int last = 0; //entries-1;
  double temp1, temp2, temp3, temp4, temp5, slope;
  
  if (type == HIV) {
    
    //table = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_HIV_table(400, 2);
    entries = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_num_mort_entries();
    last = entries-1;
    
    temp1 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(first, 0); //table[first][0];
    temp2 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(last, 0); //table[last][0];
    //    temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(middle, 0); //table[middle][0];
    temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(first, 1); //table[first][1];
    temp5 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(last, 1); //table[last][1];
    
    //    fprintf (kim_test,"\ntemp1 = %f\n", temp1);
    //    fprintf (kim_test,"temp2 = %f\n", temp2);
    //    fprintf (kim_test,"temp3 = %f\n", temp3);
    //    fprintf (kim_test,"temp4 = %f\n", temp4);
    //    fprintf (kim_test,"temp5 = %f\n", temp5);
    //
    
    while (first < last)
    {
      middle = (int) ceil((first + last)/2.0);
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(middle, 0);
      
      if (fabs(temp1 - lookup) < EPSILON){
        index = first;
        found = 1;
        break;
      }
      if (fabs(temp2 - lookup) < EPSILON){
        index = last;
        found = 1;
        break;
      }
      if (fabs(temp3 - lookup) < EPSILON){
        index = middle;
        found = 1;
        break;
      }
      
      //if we are in here for the male or female age lookup, then we
      //need to do linear interpolation
      if (first == last - 1)  //only can get here in the case of age lookup...do linear interp
      {
        temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(first, 1); //table[first][1];
        slope = (temp5 - temp4)/(temp2 - temp1);
        return (temp4 + slope*(lookup - temp1));
      }
      
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(middle, 0);
      if (lookup < temp3)
        last = middle;
      else
        first = middle;
    }
    
    if (found == 0){
      printf("ERROR: entry not found in table1\n");
      exit(0);
    }
    else{
      return ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_table(index, 1); //table[index][1];
    }
    
  }
  else if (type == AGE_MALE){
    //table = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(400, 2);
    entries = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_num_mort_male_entries();
    last = entries-1;
    
    temp1 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(first, 0); //table[first][0];
    temp2 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(last, 0); //table[last][0];
    temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(middle, 0); //table[middle][0];
    temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(first, 1); //table[first][1];
    temp5 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(last, 1); //table[last][1];
    while (first < last)
    {
      middle = (int) ceil((first + last)/2.0);
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(middle, 0);
      
      if (fabs(temp1 - lookup) < EPSILON){
        index = first;
        found = 1;
        break;
      }
      if (fabs(temp2 - lookup) < EPSILON){
        index = last;
        found = 1;
        break;
      }
      if (fabs(temp3 - lookup) < EPSILON){
        index = middle;
        found = 1;
        break;
      }
      
      //if we are in here for the male or female age lookup, then we
      //need to do linear interpolation
      if (first == last - 1)  //only can get here in the case of age lookup...do linear interp
      {
        temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(first, 1); //table[first][1];
        slope = (temp5 - temp4)/(temp2 - temp1);
        return (temp4 + slope*(lookup - temp1));
      }
      
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(middle, 0);
      if (lookup < temp3)
        last = middle;
      else
        first = middle;
    }
    
    if (found == 0){
      printf("ERROR: entry not found in table2\n");
      exit(0);
    }
    else{
      return ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Male_table(index, 1); //table[index][1];
    }
    
  }
  else if (type == AGE_FEMALE){
    //table = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(400, 2);
    entries = ((HIV_Natural_History *)(this->condition->get_natural_history()))->get_num_mort_female_entries();
    last = entries-1;
    
    temp1 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(first, 0); //table[first][0];
    temp2 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(last, 0); //table[last][0];
    temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(middle, 0); //table[middle][0];
    temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(first, 1); //table[first][1];
    temp5 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(last, 1); //table[last][1];
    while (first < last)
    {
      middle = (int) ceil((first + last)/2.0);
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(middle, 0);
      
      if (fabs(temp1 - lookup) < EPSILON){
        index = first;
        found = 1;
        break;
      }
      if (fabs(temp2 - lookup) < EPSILON){
        index = last;
        found = 1;
        break;
      }
      if (fabs(temp3 - lookup) < EPSILON){
        index = middle;
        found = 1;
        break;
      }
      
      //if we are in here for the male or female age lookup, then we
      //need to do linear interpolation
      if (first == last - 1)  //only can get here in the case of age lookup...do linear interp
      {
        temp4 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(first, 1); //table[first][1];
        slope = (temp5 - temp4)/(temp2 - temp1);
        return (temp4 + slope*(lookup - temp1));
      }
      
      temp3 = ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(middle, 0);
      
      if (lookup < temp3)
        last = middle;
      else
        first = middle;
    }
    
    if (found == 0){
      printf("ERROR: entry not found in table3\n");
      exit(0);
    }
    else{
      return ((HIV_Natural_History*)(this->condition->get_natural_history()))->get_HIV_AGE_Female_table(index, 1); //table[index][1];
    }
    
  }
  else{
    printf("ERROR: TYPE not defined. input char: \n");
    getchar();
    return 0.0;
  }
  //mina debug
  //  printf("\n entries: %d\t ", entries);
  //  printf ("\n\n in Get_Rate: TYPE = %d \t hiv_mort_table: %.4f \n", type, table[1][1]); //mina test
  
}   //end of get_rate

/*******************************************************************************************************
 *****************************************************************************************************/
void HIV_Infection::decrementForIntolerance()
{
  int j, k;
  
  //Before we put the patient on a new regimen, remove the drugs the patient is intolerant to from the drugs remaining in each class.  Part of the time, we will decrement for all, otherwise, we choose a drug at random to decrement for.
  
  //First case - we decrement for all drugs in the regimen
  if ((!VRT ? uniform(&HIV_Natural_History::seed) : 0 /*unifSim[7]*/) <= INTOLERANCE_DING_FOR_ALL)
  {
    for (j = 0; j < DRUGS_IN_REG; j++)
    {
      this->reg[j]->intol=true;
    }
    
    if (KIMTEST)
    {
      fprintf (kim_test,"Patient was intolerant to ALL drugs.  Decrementing for all.\n ");
      printDrugsInReg();
      printNumRemaining();
    }
  }
  
  else	//only decrementing for one drug in the regimen.  choose it at random
  {
    k = (!VRT ? RandomNumber(3) : 0 /*(int)unifSim[8]*/);
    
    if (!this->reg[k]->intol)
    {
      this->reg[k]->intol= true;
      
      if (KIMTEST)
      {
        fprintf (kim_test,"Patient was intolerant.  Choosing one drug at random to decrement: %s.%d\n", this->className[this->reg[k]->classNum], this->reg[k]->drugNum);
        printNumRemaining();
      }
    }
    else if (KIMTEST) fprintf (kim_test,"Chose one drug at random to decrement but pat was already intol: %s.%d\n", this->className[this->reg[k]->classNum], this->reg[k]->drugNum);
  }
}

/**************************************************************************************************
 **************************************************************************************************/
void HIV_Infection::Check_For_Mutations()
{
  int j, mutType;
  double mutMultES4 = 1;		//An increase in the mutation rate for patients in state ES4.  By default, this value will be 1 to have no effect
  
  //Check each drug in the patient's regimen for mutations.
  //For purposes of the model, patient can only develop mutations to drugs in their regimen.
  
  for (j = 0; j < DRUGS_IN_REG; j++)
  {
    //for simplicity below
    mutType = this->reg[j]->classNum;
    
    /*#if RETENTION_IN_CARE
     if (pat->num_cycles_ES4 > CYCLES_PER_MONTH)  // apply multiplier on mutation if in ES4>1 month and restart ART.
     {
     mutMultES4 *= mutation_mult_es4;
     }
     #endif
     */
    
    //VL is a surrogate for replication.  Thus, a higher viral load results in a higher prob of mutation
    this->adjmutrate = this->mutRateClass[mutType]*pow(factor, this->VLideal - 2.31) * mutMultES4;
    
    //now we can get prob of mutation in a month/day (depending on the cycle time)
    this->pMutate = 1 - exp(-this->adjmutrate*CYCTIME);
    
    //determine if there is a virus mutation, if not, we just go back to cycle
    //Note:
    //1) patient can only develop a mutation to a drug if they are compliant to that drug
    //2) if the switch is set to not allow the patient to develops muts to resistant drugs
    //	 we need to make sure the patient is not resistant to that drug.
    
    double tempseed5=uniform(&HIV_Natural_History::seed);
    //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n 5  Uniform_seed= %f \t", tempseed5 );
    
    
    if (!this->nc[j] &&
        (!VRT ? tempseed5 : 0 /*unifSim[kludge_counter++]*/) <= this->pMutate && //virus mutates	//VRT
        ((!CAN_GET_MUTS_TO_RES_DRUGS && !this->reg[j]->res) || CAN_GET_MUTS_TO_RES_DRUGS))
    {
      //Virus mutated!
      if (KIMTEST)
      {
        fprintf (kim_test, "\nMutation at cycle %d against drug %s.%d.    ", this->days_post_exposure, this->className[mutType], this->reg[j]->drugNum);
      }
      
      //      if (DEBUG_NUM_MUTS && (this->days_post_exposure - this->haart_start_time) <= CYCLES_PER_YEAR) total_muts_all_pats++;    //mina change
      
      this->mutCount++;  // used in MARK_ROBERTS_AHRQ and MONITORING_PAPER
      
      if (ANNALS_REGS_MUTS)
      {
        //if ((this->cyclenum - this->haart_start_time) <=CYCLES_PER_YEAR*5 ) totalNumMuts1st5years++;
        //if ((this->cyclenum - this->haart_start_time) <=CYCLES_PER_YEAR*10) totalNumMuts1st10years++;
      }
      
      //increment number of mutations conferring possible resistance to this arv type
      this->mut_arv[mutType]++;
      
      //#ifdef MONITORING_PAPER_SENSITIVITY
      //			if(floor((double)runnum/2) == 1) pat->pmutres[mutType] *= sens[1][runnum % 2];
      //#endif
      
      double tempseed6=uniform(&HIV_Natural_History::seed);
      //if (this->host->get_id() == Global::TEST_ID) fprintf(mina_test, "\n 6  Uniform_seed= %f \t", tempseed6 );
      
      if ((!VRT ? tempseed6 : 0 /*unifSim[kludge_counter++]*/) <= this->pmutres[mutType])		//mut res to >= 1 drugs
      {
        //assign resistance to found drug in regimen
        this->reg[j]->res = true;
        
        if (KIMTEST)
        {
          fprintf (kim_test, "Mutation is resistant.\n");
          fprintf (kim_test, "Res profile: (%d %d %d)  \n", this->reg[0]->res, this->reg[1]->res, this->reg[2]->res);
        }
        
        //        if (DEBUG_NUM_MUTS)   //mina change
        //        {
        //          if ((this->days_post_exposure - this->haart_start_time) <= CYCLES_PER_YEAR) total_res_muts_all_pats++;
        //          if (!this->hasResistance)
        //          {
        //            if ((this->days_post_exposure - this->haart_start_time) <=CYCLES_PER_YEAR*1 ) TotalPatsRes1st1years++;
        //            if ((this->days_post_exposure - this->haart_start_time) <=CYCLES_PER_YEAR*5 ) TotalPatsRes1st5years++;
        //            this->hasResistance = true;
        //          }
        //        }
        
        //Now check for cross-resistance and cross-class resistance to other drugs
        checkForCrossResAndCrossClassRes(mutType);
        
      }//end if mutation is resistant
      
      else if (KIMTEST) fprintf (kim_test, "Mutation not resistant.\n");
      
    }//end of if mutation
  }//end of for loop
}   //enf of Check_For_Mutations

/*******************************************************************************************************
 *****************************************************************************************************/
//The costs in the function metTrigger are From Kara Wools-Kaloustian, per her e-mail dated 11/18/2009.
//At AMPATH the cost of a CD4 test is $11.20, and VL is $70.
bool HIV_Infection::metTrigger(int trigger)
{
  bool retVal = false;				//indicates whether trigger to change regimen has been met
  const double CD4differential = 50.0;
  double testcost = 0;					//iniitalize the cost of any tests
  
  switch (trigger)
  {
      
    case T_VL:
      
      //Set the cost of the test
      testcost = testCostVL;
      
      if (this->VLreal>=this->vl_regimen_failure)
      {
        if (KIMTEST) fprintf (kim_test, "\nRegimen failed at cycle %d.  VL above threshold of %.1f.  VL=%.1f\n", this->days_post_exposure, this->vl_regimen_failure, this->VLreal);
        
        //if this is the first time (vl_regimen_failure is still set to 1st),
        //reset the vl_regimen_failure to after_1st
        if (this->vl_regimen_failure == vl_regimen_failure_1st)
          this->vl_regimen_failure = vl_regimen_failure_after_1st;
        
        retVal = true;
      }
      
      /*if (WHO_MONITORING_STRATEGY_ANALYSIS)
       {
       incrementWHOArray(pat, numVLTests);
       
       if (trig != T_NESTED)
       {
       //Note - Scott says to implement "confirmatory retesting" where any positive VL test is followed up by another VL test
       //We ONLY do this if the regimen trigger is VL, NOT if it's NESTED.
       if (retVal == true) incrementWHOArray(pat, numVLTests);
       }
       }
       */
      break;
      
      
    case T_CD4:
      
      //Set the cost of the test
      testcost = testCostCD4;
      
      if ((this->CD4real < .5 * this->maxCD4onReg ||
           this->CD4real <= (this->CD4RegBaseline - CD4differential) ||
           this->CD4real < 100) &&
          !(CALIBRATE && this->numreg == MAX_REGS))
      {
        if (KIMTEST) fprintf (kim_test, "\nRegimen failed at cycle %d.  CD4real: %.1f, peak CD4 on round: %.1f, round baseline cd4: %.1f\n",  this->days_post_exposure, this->CD4real, this->maxCD4onReg, this->CD4RegBaseline);
        
        retVal = true;
      }
      
      //    if (WHO_MONITORING_STRATEGY_ANALYSIS) incrementWHOArray(pat, numCD4Tests);
      
      break;
      
      
    case T_NESTED:
      
      //If CD4 failure criteria is met, then check VL
      if (trigger==T_NESTED && metTrigger(T_CD4))
      {
        if (metTrigger(T_VL))
        {
          if (KIMTEST) fprintf (kim_test, "\nRegimen failed at cycle %d with nested trigger\n",  this->days_post_exposure);
          
          //Note, I don't add in the costs here because the costs are added when I check each individual trigger.
          
          retVal = true;
        }
      }
      
      break;
      
      
    case T_CLINICAL:
      
      if (this->AIDSeventReg)
      {
        if (KIMTEST) fprintf (kim_test, "\nRegimen failed at cycle %d with clinical trigger\n",  this->days_post_exposure);
        
        retVal = true;
      }
      
      break;
  }
  
  //add the costs of any tests to the total test costs    //mina move to HIV epidemic
  total_cost += testcost;
  total_disc_cost += testcost / pow( (1+DISC_RATE), ((double)this->days_post_exposure/(double)CYCLES_PER_YEAR));
  total_lab_cost += testcost;
  total_lab_disc_cost += testcost / pow( (1+DISC_RATE), ((double)this->days_post_exposure/(double)CYCLES_PER_YEAR));
  
  return retVal;
} //end of metTrigger(int trigger)

//*******************************************************
//*******************************************************

void HIV_Infection::Process_Death(int type, int cyclenum) {
  double dur, durHaart = 0;
  int year, cycles_of_outreach_year_of_death;
  int cycle_per_year = CYCLES_PER_YEAR;
  
  if (KIMTEST && type == HIV) fprintf (kim_test, "PATIENT DIED from HIV at cycle %d.\n\n", cyclenum);
  if (KIMTEST && type == AGE) fprintf (kim_test, "PATIENT DIED from Age at cycle %d.\n\n", cyclenum);
  if (KIMTEST && type == CENSORED) fprintf (kim_test, "PATIENT was censored at cycle %d.\n\n", cyclenum);
  
  if (type == HIV){
    ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_hiv_deaths();
    ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_hivdeaths(cyclenum, cycle_per_year);
    this->fatal_hiv=true;

  }
  //else if (type == AGE) {  //REMOVED: Fred handles this
  /*((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_age_deaths();
    ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_agedeaths(cyclenum);
    this->fatal_hiv=true;
    //agedeaths[int(cyclenum/CYCLES_PER_YEAR)]++;
    ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_agedeaths(cyclenum);
  }
    else if (type == CENSORED){
      total_censored++;
    }
  */
  else {
    printf("type in Process_Death not valid\n");
    exit(0);
  }
  
  /*
   #if RETENTION_IN_CARE
   if (outreach_interv_enabled && pat->outreach)  //if this pat died during the outreach process
   {
   pat->outreach = false;
   pat->deathUnkown = true;
   total_num_stopoutreach++; //outreaching is terminated
   
   addOutreachCostsToTotalCostAtDeath(pat);
   }
   #endif
   
    if (DEBUG_NUM_MUTS) totalResDrugsAtDeath += countResDrugs(pat);
  
  if the patient has AIDS, they were already counted
    if (DEBUG_AIDS && !this->has_AIDS && int(pat->cyclenum/CYCLES_PER_YEAR) <= 5)
      totalAIDSOrDeath++;
  
    if ((KAPLAN_MEIER_SURVIVAL_CURVE) && pat->cyclenum < (TIME*CYCLES_PER_YEAR) )   //you can put the survivalPlot back in if you want to use it later
    {
  //    if (type == HIV)				SurvivalPlot[0][pat->cyclenum]++;
  //    else if (type == AGE)			SurvivalPlot[1][pat->cyclenum]++;
  //    else if (type == CENSORED)		SurvivalPlot[2][pat->cyclenum]++;
    }
  */
  
  dur = (double) days_post_exposure;
  if ( this->started_haart )
    durHaart = cyclenum - this->haart_start_time;
  
  if (this->host->get_id() == Global::TEST_ID) fprintf (mina_test, "dur: %f.\n\n", dur);
  
  /*mina: following function replaced these lines: total_surv_time += dur;  //gives cumulative total years since patient entered model
   timeTillDeath[numDeaths] = (double) dur;
   numDeaths++;
   */
  ((HIV_Epidemic *)(this->condition->get_epidemic()))->increment_total_hiv_surv_time(dur);
  
  if (KIMTEST){
    fprintf (kim_test, "\n\nReg\tCycle Reg Started\tCycles on Reg\n");
    int regStart=this->haart_start_time;
    for (int i=0; this->cyclesOnReg[i]!=0 && i<MAX_REGS; i++){
      fprintf (kim_test, "%d\t%d\t\t%d\n", i, regStart, this->cyclesOnReg[i]);
      regStart += this->cyclesOnReg[i];
    }
    fprintf (kim_test, "\n\n\n");
  }
  
  if (VLCD4GRAPH && (CYCLES_PER_YEAR == 12 || cyclenum %1 ==0 )){
    //fprintf (HIV_Infection::kim_test2, "\n\nPat num: %d\n\n", patnum);
  }
  
  if (ACTIVE_HAART_INFO || REG_INFO_FOR_SENSITIVITY){
    if (this->exhausted_clean_regs){
      ((HIV_Natural_History *)(this->condition->get_natural_history()))->hiv_salvage((this->haart_exhausted_clean_regs_time - this->haart_start_time),this->num_res_drugs_before_salvage,this->num_regs_before_salvage);
    }
  }
  
  //	if (VRT){
  //		while (pat_rns_used < MAX_PAT_RNS)
  //			fillUnifSim();
  //	}
  
  if (GETSTATES && type == HIV){
    
    /*Record the cost.
     
     On a normal cycle, we just add the daily cost to
     the running total for the year.  However, we are trying to find
     the annual cost.  Since the patient has died, we want to extrapolate
     out the costs so far this year to cover the whole year.
     
     Note:  If the patient is killed on cycle 0 or is LTFU and we haven't yet calculated a cost, do so now to avoid
     a divide-by-zero error below.  And since we can't do "==0" because of floating point error, just check whether the cost
     is a very small number.  That one-day cost then has to be extrapolated out for the year, so multiply by cycles per year.
     */
    
    /*NEW Record the cost.
     
     Normally, on the last day of the year, we record the costs accumulated for that year.
     When the patient dies, we extrapolate their costs for the year so far out for the rest of the year
     to get a cost that is representative of an entire year.
     A problem occurs if the patient dies on day 365 because the denominator in that extrapolation
     will be 0 (365 % CYCLES_PER_YEAR will be 0).  In that case, we still want to make an entry
     because the patient dies, but we will take the cost accumulated that day and extrapolate them out for the year.
     
     Also note that we use (pat->cyclenum % CYCLES_PER_YEAR) + 1.0 because if we're on cycle num one, we have
     actually accumulated costs for cycle 0 and cycle 1.  (Two days.)
     */
    
    //MINA CHECK : put these back in for cost
    //if (cyclenum % CYCLES_PER_YEAR == 0) costThisYearThisPat = (total_cost - totalCostAtStartOfYearThisPat) * CYCLES_PER_YEAR;
    //else costThisYearThisPat = (total_cost - totalCostAtStartOfYearThisPat) * (double) CYCLES_PER_YEAR / (double(cyclenum % CYCLES_PER_YEAR) + 1.0);
    
  }
  
} // end of process_death

//*******************************************************
//*******************************************************

//double HIV_Infection::get_CD4_real(int k){
//  if (CD4_OUTPUT_EXCEL_DAY) return real_CD4_day[k];
//  //else return real_CD4_day[k];
//}
//
//double HIV_Infection::get_VL_real(int k){
//  if (CD4_OUTPUT_EXCEL_DAY) return real_VL_day[k];
//  //else return real_VL_day[k];
//}

//*******************************************************
//*******************************************************

/*The function getInitialAIDSRates() is used when START_WITH_AIDS (the percentage of patients starting with AIDS)
 is set to a non-zero value.  If some portion of patients are starting the model with AIDS, we want to distribute
 those patients across CD4 strata according to the hiv mortality table.  We are using "middle of the road" data for
 the lookup table:  Patient not on haart, 40<age<50, 2nd year, 4.5<=VL<5.5.  Using these values, gives us two strata for CD4.
 
 2213 (CD4 < 50)		0.2241
 2223 (50<=CD4<=200) 0.2241
 2233 (200<=CD4<=350)0.027
 2243 (350<=CD4<=500)0.027
 2253 (CD4>=500)		0.027
 
 Thus, we have two strata:  CD4<=200 and CD4>200.  This explains the significance of the 200 in the function below.
 If this table changes, the function should be updated accordingly.
 
 NumPats CD4<=200 * AIDSrate<=200 + NumPats CD4>200 * AIDSrate>200 = START_WITH_AIDS * Total NumPats
 
 This is best illustated with an example.
 
 Ex:
 
 AVG_CD4 = 250
 AVG_CD4_SD = 50
 START_WITH_AIDS = .2
 
 From normal distribution,	P(<200) = .1587
 P(>200) = .8413
 
 The ratio of AIDS rates (<200 : >200) = 0.2241:0.027
 
 The percentage of patients with CD4 <= 200 starting with AIDS must be
 .2241*AIDS_EVENT_MULTIPLIER*x
 The percentage of patients with CD4 >200 starting with AIDS must be
 .027*AIDS_EVENT_MULTIPLIER*x
 
 (Note that the AIDS_EVENT_MULTIPLIER isn't really necessary here since the ratios would be the same without it.)
 
 .1587N((.2241*AIDS_EVENT_MULTIPLIER)*x) + .8413N((.027*AIDS_EVENT_MULTIPLIER)*x) = .2N
 
 The Ns cancel leaving,
 
 .1587((.2241*AIDS_EVENT_MULTIPLIER)*x) + .8413((.027*AIDS_EVENT_MULTIPLIER)*x) = .2
 */
void HIV_Infection::getInitialAIDSRates(){
  
  double x;
  double prob_less_200;			//Given uniform distribution
  double prob_greater_200;		//Given uniform distribution
  
  double p_aids_less_200 = .2241 * AIDS_EVENT_MULTIPLIER;
  double p_aids_greater_200 = .027 * AIDS_EVENT_MULTIPLIER;
  
  if (avgCD4_SD != 0)
  {
    if (start_cd4 > 200) prob_less_200 = 1-cdf(-(200-start_cd4)/avgCD4_SD);
    else prob_less_200 = cdf((200-start_cd4)/avgCD4_SD);
    
    prob_greater_200 = 1-prob_less_200;
    
    x = start_with_aids / (prob_less_200*p_aids_less_200 +
                           prob_greater_200 * p_aids_greater_200);
    
    AIDSrate_less200 =  p_aids_less_200 * x;
    AIDSrate_greater200 = p_aids_greater_200 * x;
    
    if (AIDSrate_less200 > 1)
    {
      AIDSrate_less200 =  1;
      AIDSrate_greater200 = (start_with_aids - prob_less_200) / prob_greater_200;
    }
  }
  
  else AIDSrate_less200 = AIDSrate_greater200 = start_with_aids;
} // end of getInitialAIDSRates()

//*******************************************************
//*******************************************************


void HIV_Infection::print_infection(){
 // cout << "Person ID " << this->host->get_id() << "\tsex " << this->host->get_sex() << "\tacute " << this->acute << "\tdiagnosed " << this->diagnosed << "\tVL " << this->VLreal << "\tCD4 " << this->CD4real << endl;
  
  printf("Person ID %d\tsex %c\tacute %d\tdiagnosed %d\tVL %.2f\tCD4 %.2f\n", this->host->get_id(), this->host->get_sex(), this->acute, this->diagnosed, this->VLreal, this->CD4real );
  
}


//void HIV_Infection::graph_trajectories(int cyclenum) {}   //mina check  use this function and check what it does for graphing
 //mina check    cyclenum should be today or days past exposure
