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

#include "HIV_Natural_History.h"
#include "Condition.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "Utils.h"

#include "distributions.h"
#include "distributions.cc"
#include <fstream>

long int HIV_Natural_History::seed = 60076;   //mina : kim's code: #define INITIAL_SEED 60076

double HIV_Natural_History::AVG_CD4 = 332.31;

//Number of drugs in each class (resource rich!!)
int HIV_Natural_History::NUM_PI_SINGULAR = 1;	//Nelfinavir is the only unboosted PI
int HIV_Natural_History::NUM_PI_BOOSTED = 7;
int HIV_Natural_History::NUM_NRTI_TAM = 2;
int HIV_Natural_History::NUM_NRTI_NONTAM = 5;
int HIV_Natural_History::NUM_NNRTI_EFAVIRENZ = 1;
int HIV_Natural_History::NUM_NNRTI_NEVIRAPINE = 1;

HIV_Natural_History::HIV_Natural_History() {
  this->hiv_num_mort_entries = -1;    //mina
  this->hiv_num_male_entries = -1;    //mina
  this->hiv_num_female_entries = -1;    //mina
  this->num_exhausted_clean_regs = -1;  //mina
  
  this->num_exhausted_clean_regs = -1;
    
  for (int j = 0; j < 22; j++ ){
    numDeadYear[j] = -1;
    numDeadYearHaart[j] = -1;
  }
  
  this->total_time_on_haart_before_salvage= -1.0;
  this->total_num_regs_before_salvage= -1;
  this->total_res_drugs_before_salvage=-1;
  
  for(int i=0; i<400; i++){
    for(int j = 0; j < 2; j++ ){
      hiv_mort_table[i][j] = -1;    //mina
      hiv_male_age_table[i][j] = -1;
      hiv_female_age_table[i][j] = -1;
    }
  }
  
  //  for (int i=0; i<3; i++){
  //    this->mean_reg_time[i]= -1.0;
  //    this->median_reg_time[i]= -1.0;
  //    this->total_completed_reg_cycles[i]= -1.0;
  //    this->numPeopleCompletedReg[i] = -1;
  //  }
  
  this->numStartedSalvage = -1;
  this->numReached2DrugsResOnSalvage = -1;
  
  this->removeNonHIVMort = -1; // = 0;		//default to NOT removing it!
  
  this->transmissibility = 1.0;
}

HIV_Natural_History::~HIV_Natural_History() {
}

void HIV_Natural_History::setup(Condition * _condition) {
  Natural_History::setup(_condition);
  
  FRED_VERBOSE(0, "HIV_Natural_History::setup\n");
  
  init_genrand(seed); //sets the initial the seed. initial_seed=60076. it is set above.
  
  //  int mina_test = init_genrand(seed); //sets the initial the seed
  //  fprintf(Global::mina_test, "MINAAA: mina_test_initial HIV SEED = %d ", mina_test);
  //int vl_thresh;	//for MONITORING_PAPER loop
  //this->numAvailableRegimens = NUM_CLASS_COMBINATIONS;						//only used for resource poor for MONITORING_PAPER
  //this->poorCanGoBackOnPreviousRegs = POOR_CAN_GO_BACK_ON_PREVIOUS_REGIMENS;
  
  //variable for LOOP analysis
  this->removeNonHIVMort = 0;		//default to NOT removing it!
  
  //mina: void Initialize_Vars() in NYC code starts here!
  
  this->num_exhausted_clean_regs = 0;     //maybe move to infection
  
  for (int j = 0; j < 22; j++ ){
    numDeadYear[j] = 0;
    numDeadYearHaart[j] = 0;
  }
  
  //some code omitted here... from hiv toxicity
  numStartedSalvage = 0;      //maybe move to epidemic? //mina check
  numReached2DrugsResOnSalvage = 0;   //maybe move to epidemic? //mina check
  
  /*************************************/
  /*************************************/
  /*************************************/
  
  //mina: these are not checked yet!!!!
  
  //  num_exhausted_clean_regs= 0;
  //  numReached2DrugsResOnSalvage =0;
  //  totalAIDSOrDeath =0;
  //  kludge_counter =0;
  //  max_seed_cycle =510;
  
  //  numPeopleCompletedReg[0] = numPeopleCompletedReg[1] = numPeopleCompletedReg[2] = 0;  //Kim added
  //  total_completed_reg_cycles[0] = total_completed_reg_cycles[1] = total_completed_reg_cycles[2] = 0;	//Kim
  
  //  if (TIME_TO_TREATMENT_FAILURE_OF_REGIMENS_1_2_3 || REG_INFO_FOR_SENSITIVITY){
  //    for (i=0; i<3; i++){
  //      //initialize
  //      mean_reg_time[i]=0;
  //      median_reg_time[i]=0;
  //
  //      //        if (numPeopleCompletedReg[i])
  //      //        {
  //      //          mean_reg_time[i] = (total_completed_reg_cycles[i]/(double)CYCLES_PER_YEAR)/numPeopleCompletedReg[i];
  //      //          median_reg_time[i] = Get_Median(cyclesToCompleteReg[i], numPeopleCompletedReg[i])/(double)CYCLES_PER_YEAR;
  //      //        }
  //    } 	//output below
  //  }
  
  //    if (DEBUG_NUM_MUTS){
  //      total_muts_all_pats=0, total_res_muts_all_pats=0;
  //      TotalPatsRes1st1years = 0;
  //      TotalPatsRes1st5years = 0;
  //      totalResDrugsAtDeath = 0;
  //    }
  
  FRED_VERBOSE(0, "HIV_Natural_History::setup finished\n");

}  //end of setup()

void HIV_Natural_History::get_parameters() {

  FRED_VERBOSE(0, "HIV_Natural_History get_parameters started.\n");

  Natural_History::get_parameters();

  // Read in any file having to do with HIV natural history, such as
  // mortality rates

  // If you read in a value X here that should be accessible to
  // HIV_Infection, you should define a method in HIV_Natural_History.h
  // called get_X() that returns the value of X.  Then HIV_Infection can
  // access the value of X as follows:

  // int my_x = this->condition->get_natural_history()->get_X();
  
  int i=0; //, ival;
  int table_space = 400;
  
  //read in HIV mortality table
  ifstream  inFile;
  inFile.open("hiv_mort_table.txt", ios::in);
  while (1)
  {
    if (i == table_space)
    {
      printf("WARNING: Out of table space for HIV mortality\n");
      exit(0);
    }
    inFile >> this->hiv_mort_table[i][0] >> this->hiv_mort_table[i][1] ;
    //    printf("\n hiv mortality %d \t first col: %f \t second column: %.6f", i, this->hiv_mort_table[i][0], this->hiv_mort_table[i][1]);
    i++;
    if (inFile.eof()){
      //fprintf(Global::Statusfp, "\n ENDED HIV MORT READING\n");
      break;
    }
  }
  this->hiv_num_mort_entries = i;
  //fprintf(Global::Statusfp, "\n hiv_mort: %.4f \n\n\n\n", this->hiv_mort_table[1][1]);
  fflush(Global::Statusfp);
  inFile.close();
  
  //read in Resource Rich female non-hiv age-related death table
  inFile.open("male_age_table.txt", ios::in);
  i=0;
  while (1)
  {
    if (i == table_space)
    {
      printf("WARNING: Out of table space for HIV male age mortality\n");
      exit(0);
    }
    inFile >> this->hiv_male_age_table[i][0] >> this->hiv_male_age_table[i][1] ;
    //    printf("\n hiv mortality %d \t first col: %f \t second column: %.6f", i, this->hiv_male_age_table[i][0], this->hiv_male_age_table[i][1]);
    i++;
    if (inFile.eof()){
     // fprintf(Global::Statusfp, "\n ENDED MALE AGE MORT READING\n");
      break;
    }
  }
  this->hiv_num_male_entries = i;
  //fprintf(Global::Statusfp, "\n hiv_mort: %.4f \n\n\n\n", this->hiv_male_age_table[1][1]);
  fflush(Global::Statusfp);
  inFile.close();
  
  //read in Resource Rich female non-hiv age-related death table
  inFile.open("female_age_table.txt", ios::in);
  i=0;
  while (1)
  {
    if (i == table_space)
    {
      printf("WARNING: Out of table space for HIV female age mortality\n");
      exit(0);
    }
    inFile >> this->hiv_female_age_table[i][0] >> this->hiv_female_age_table[i][1] ;
    //    printf("\n hiv mortality %d \t first col: %f \t second column: %.6f", i, this->hiv_female_age_table[i][0], this->hiv_female_age_table[i][1]);
    i++;
    if (inFile.eof()){
      //fprintf(Global::Statusfp, "\n ENDED FEMALE AGE MORT READING\n");
      break;
    }
  }
  this->hiv_num_female_entries = i;
  //fprintf(Global::Statusfp, "\n hiv_mort: %.4f \n\n\n\n", this->hiv_female_age_table[1][1]);
  inFile.close();
  
  //fprintf(Global::Statusfp, "\n hiv_mort_entries: %d \t hiv_num_male_entries: %d \t hiv_num_female_entries: %d", this->hiv_num_mort_entries, this->hiv_num_male_entries, this->hiv_num_female_entries);
  
  fflush(Global::Statusfp);
  FRED_VERBOSE(0, "HIV_Natural_History get_parameters finished.\n");
}  //end of get_parameters()

//******************************************************
//******************************************************

double HIV_Natural_History::get_probability_of_symptoms(int age) {
  return 0.0;
}
int HIV_Natural_History::get_latent_period(Person* host) {
  return 1; //should be at least 1 //mina change
}
int HIV_Natural_History::get_duration_of_infectiousness(Person* host) {
  // infectious forever
  return -1;
}
int HIV_Natural_History::get_duration_of_immunity(Person* host) {
  // immune forever
  return -1;
}
int HIV_Natural_History::get_incubation_period(Person* host) {
  return -1;
}
int HIV_Natural_History::get_duration_of_symptoms(Person* host) {
  // symptoms last forever
  return -1;
}
bool HIV_Natural_History::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  return false;
}
bool HIV_Natural_History::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  return false;
}
/*
void HIV_Natural_History::update_infection(int day, Person* host, Infection *infection) {
  // put daily updates to host here.
}
*/

/***** MINA BEGIN *****/
//******************************************************
//******************************************************

void HIV_Natural_History::hiv_salvage(double a, int b, int c){
  total_time_on_haart_before_salvage += a;
  total_res_drugs_before_salvage += b;
  total_num_regs_before_salvage += c;
  num_exhausted_clean_regs++;
}

//******************************************************
//******************************************************
//MINA check: change these tables to statis, available everywhere. might be faster?
double HIV_Natural_History::get_HIV_table(int row, int col){     //mina
  return this->hiv_mort_table[row][col];
}
double HIV_Natural_History::get_HIV_AGE_Male_table(int row, int col){
  return this->hiv_male_age_table[row][col];
}
double HIV_Natural_History::get_HIV_AGE_Female_table(int row, int col){
  return this->hiv_female_age_table[row][col];
}

/******************************************************
 Gets the median value of an array of values, a[], of size "size".  Calls Quicksort to sort the array
 ******************************************************/

double HIV_Natural_History::Get_Median(double a[], int size)
{
  double median;
  int index1, index2;
  
  QuickSort(a, 0, size-1);
  
  //if size is even, average middle two values, if odd, take middle value
  if (size%2 == 0)		//size is even
  {
    index1 = size/2 - 1;
    index2 = size/2 ;
    //take average
    median = (a[index1] + a[index2])/2.0;
  }
  else
  {
    index1 = ( (size + 1)/2 ) - 1;
    median = a[index1];
  }
  
  return median;
}

//******************************************************
//******************************************************
/******************************************************
 Swaps the (x+1)th and the (y+1)th element of the array "a".  Used in Quicksort algorithm.
 *****************************************************/

void HIV_Natural_History::Swap( double a[], int x, int y )
{
  double t = a[x];
  a[x] = a[y];
  a[y] = t;
}

//******************************************************
//******************************************************
/******************************************************
 Sorts array a from highest to lowest value
 l and r are the left and right array indices
 *****************************************************/
void HIV_Natural_History::QuickSort(double a[], int l, int r )
{
  int i, j, /*v, *t,*/ind;
  
  double v, t;	//Kim changed v and t to doubles since our array is all doubles.
  
  if ( r > l )
  {
    ind = ((l+r)/2);
    
    v = a[ind]; i = l-1; j = r;
    
    //Swap( a, ind, r );
    t=a[ind];
    a[ind]=a[r];
    a[r]=t;
    
    for ( ;; )
    {
      while ( i < r && a[++i] < v ) {}
      while ( j > l && a[--j] > v ) {}
      
      if ( i >= j ) break;
      //Swap( a, i, j );
      t=a[i];
      a[i]=a[j];
      a[j]=t;
    }
    
    //Swap( a, i, r );
    t=a[i];
    a[i]=a[r];
    a[r]=t;
    QuickSort( a, l, i-1 );
    QuickSort(a, i+1, r );
  }
  
  return;
}

//******************************************************
//******************************************************

double HIV_Natural_History::CD4_at_Diagnosis(){
  // from R code: assumed normal distribution because I do not have any data...
  // qnorm(0.75,mean=327,sd=290)

  double mean = 327;
  double sd = 290;
  std::random_device rd;
  std::normal_distribution<> dis(mean, sd);
  return dis(rd);
}


double HIV_Natural_History::assign_initial_CD4(){
  //CD4 count when infected with HIV :  900 (750–900)  Sanders et al. 2005
  // picked uniform distribution
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(750, 900);
  return dis(gen);
}

int HIV_Natural_History::update_hiv_stage(double cd4){
  // Disease stage - CD4
  // Stage 1 (CD4 >500 cells/μL or >29%)
  // Stage 2 (CD4 200–499 cells/μL or 14%–28%)
  // Stage 3 (AIDS) (OI or CD4 <200 cells/μL or <14%)
  int hiv_stage = 0;
  
  if (cd4 >= 500) hiv_stage = 1;
  else if (cd4 >= 200 && cd4 < 500) hiv_stage = 2;
  else if (cd4 < 200) hiv_stage = 3;
  
  return hiv_stage;
}
