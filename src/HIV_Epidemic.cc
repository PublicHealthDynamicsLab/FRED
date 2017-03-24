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
//  File HIV_Epidemic.cc

#include "Condition.h"
#include "Date.h"
#include "distributions.h"
#include "Global.h"
#include "HIV_Infection.h"
#include "HIV_Epidemic.h"
#include "HIV_Natural_History.h"
#include "Person.h"
#include "Place.h"
#include "Place_List.h"
#include "Random.h"
#include "Transmission.h"
#include "Utils.h"

FILE * HIV_Epidemic::hiv_file1 = NULL;
FILE * HIV_Epidemic::hiv_incidence = NULL;
FILE * HIV_Epidemic::hiv_prevalence = NULL;
FILE * HIV_Epidemic::hiv_prevalence_age_group = NULL;

int HIV_Epidemic::HIV_inc_agegroup[100][Sexual_Transmission_Network::ROW_age_group] = {{0}};
int HIV_Epidemic::HIV_inc_f_agegroup[100][Sexual_Transmission_Network::ROW_age_group] = {{0}};
int HIV_Epidemic::HIV_inc_m_agegroup[100][Sexual_Transmission_Network::ROW_age_group] = {{0}};


int HIV_Epidemic::HIV_stage_f[100][3] = {{0}}; //stage 1, 2, 3 (AIDS)
int HIV_Epidemic::HIV_stage_m[100][3] = {{0}}; //stage 1, 2, 3 (AIDS)   //left

double HIV_Epidemic::HIV_screening_prob = 0.242930333;

// ***** Initial HIV Prevalence *****//
// AGE distribution
double HIV_Epidemic::HIV_prev_agegroup0 = 0.052071006;   //13-24
double HIV_Epidemic::HIV_prev_agegroup1 = 0.142011834;   //25-34
double HIV_Epidemic::HIV_prev_agegroup2 = 0.199605523;   //35-44
double HIV_Epidemic::HIV_prev_agegroup3 = 0.3617357;     //45-54
double HIV_Epidemic::HIV_prev_agegroup4 = 0.244575937;   //55+

// Disease stage - CD4
//Stage 1 (CD4 >500 cells/μL or >29%)
//Stage 2 (CD4 200–499 cells/μL or 14%–28%)
//Stage 3 (AIDS) (OI or CD4 <200 cells/μL or <14%)
double HIV_Epidemic::HIV_prev_diseaseStage_F1 = 0.320128429;
double HIV_Epidemic::HIV_prev_diseaseStage_F2 = 0.373690773;
double HIV_Epidemic::HIV_prev_diseaseStage_F3 = 0.305180798;
double HIV_Epidemic::HIV_prev_diseaseStage_M1 = 0.20177665;
double HIV_Epidemic::HIV_prev_diseaseStage_M2 = 0.355329949;
double HIV_Epidemic::HIV_prev_diseaseStage_M3 = 0.442893401;

int HIV_Epidemic::imported_HIV_requested_male[5] = {0};
int HIV_Epidemic::imported_HIV_requested_female[5] = {0};
int HIV_Epidemic::imported_AIDS_male[5] = {0};
int HIV_Epidemic::imported_AIDS_female[5] = {0};


bool HIV_Epidemic::CONDOM_USE = 1;
double HIV_Epidemic::DIAGNOSED_RATE = 0.86;

// Disease stage - viral load

HIV_Epidemic::HIV_Epidemic(Condition* _condition) :
Epidemic(_condition) {
  
  this->total_qa_surv_time = -1.0;
  this->total_qa_disc_surv_time = -1.0;
  this->total_disc_surv_time = -1.0;
  this->total_surv_time = -1.0;
  
  this->mean_surv_time= -1.0;
  this->mean_qa_surv_time= -1.0;
  this->mean_qa_disc_surv_time= -1.0;
  this->mean_disc_surv_time= -1.0;
  this->mean_disc_hospital_cost= -1.0;
  this->mean_hospital_cost= -1.0;
  
  this->total_cost = -1.0;
  this->total_disc_cost = -1.0;
  this->total_drug_cost = -1.0;
  this->total_drug_disc_cost = -1.0;
  this->total_care_cost = -1.0;
  this->total_care_disc_cost = -1.0;
  this->total_hospital_cost = -1.0;
  this->total_hospital_disc_cost = -1.0;
  
  this->total_hiv_deaths = -1;
  this->total_age_deaths = -1;
  this->total_censored = -1;
  this->numDeaths = 0;
  
  this->total_muts_all_pats = -1;
  this->total_res_muts_all_pats = -1;
  this->TotalPatsRes1st1years = -1;
  this->TotalPatsRes1st5years = -1;
  
  this->TotalPatsRes1st1years = 0;
  this->TotalPatsRes1st5years = 0;
  this->totalAIDSOrDeath = -1;
  
}

static bool compare_id (Person* p1, Person* p2) {
  return p1->get_id() < p2->get_id();
}

//*******************************************************
//*******************************************************
// initial HIV Prevalence
// map in FRED.cc : prepare_conditions() in condition_list : epidemic->
// prepare() : virtual void prepare();

void HIV_Epidemic::prepare() {
  
  Epidemic::prepare();

  int day = Global::Simulation_Day;
  FRED_VERBOSE(0, "prepare() HIV_epidemic started. day %d map_size %d\n", day, this->import_map.size());
  
  this->N = Global::Pop.get_population_size();
  
  for(int i = 0; i < this->import_map.size(); ++i) {
    Import_Map* tmap = this->import_map[i];
    if(tmap->sim_day_start <= day && day <= tmap->sim_day_end) {
      FRED_VERBOSE(0,"IMPORT MST:\n");
      
      fred::geo lat = tmap->lat;
      fred::geo lon = tmap->lon;
      double radius = tmap->radius;
      //long int fips = tmap->fips;
      
      int age_bundle = 0;   //FRED's age groups 0 and 1
      import_aux1('M', age_bundle, day, radius, lat, lon);
      import_aux1('F', age_bundle, day, radius, lat, lon);
      
      age_bundle = 1;     //FRED's age groups 2 and 3
      import_aux1('M', age_bundle, day, radius, lat, lon);
      import_aux1('F', age_bundle, day, radius, lat, lon);
      
      age_bundle = 2;     //FRED's age groups 4 and 5
      import_aux1('M', age_bundle, day, radius, lat, lon);
      import_aux1('F', age_bundle, day, radius, lat, lon);
      
      age_bundle = 3;     //FRED's age groups 6 and 7
      import_aux1('M', age_bundle, day, radius, lat, lon);
      import_aux1('F', age_bundle, day, radius, lat, lon);
      
      age_bundle = 4;     //FRED's age group 8
      import_aux1('M', age_bundle, day, radius, lat, lon);
      import_aux1('F', age_bundle, day, radius, lat, lon);
    }
  }
  
  FRED_VERBOSE(0, "active_people_list.size  %d\n", this->active_people_list.size());
  FRED_VERBOSE(0, "prepare() epidemic finished.\n");
  
} // end of prepare()

//*******************************************************
//*******************************************************

void HIV_Epidemic::import_aux1(char sex, int age_bundle, int day, double radius, fred::geo lat, fred::geo lon){
  
  FRED_VERBOSE(0, "import_aux1 sex %c age_bundle %d day %d radius %f lat %f lon %f\n",
	       sex, age_bundle, day, radius, lat, lon);

  int imported_HIV_requested = 0;
  int imported_AIDS_requested = 0;
  
  if (sex == 'M') {
    imported_HIV_requested = imported_HIV_requested_male[age_bundle];
    imported_AIDS_requested = imported_AIDS_male[age_bundle];
  }
  else if (sex == 'F') {
    imported_HIV_requested = imported_HIV_requested_female[age_bundle];
    imported_AIDS_requested = imported_AIDS_female[age_bundle];
  }
  FRED_VERBOSE(0, "HIV_req %d AIDS_req %d\n", imported_HIV_requested, imported_AIDS_requested);
  
  int searches_within_given_location = 1;
  while(searches_within_given_location <= 10) {
    FRED_VERBOSE(0,"IMPORT search number %d ", searches_within_given_location);
    
    //list of candidates
    std::vector<Person*> people;
    people.clear();
    // find households that qualify by location
    int hsize = Global::Places.get_number_of_households();
    // printf("IMPORT: houses  %d\n", hsize); fflush(stdout);
    
    for(int i = 0; i < hsize; ++i) {
      Household* hh = Global::Places.get_household(i);
      double dist = 0.0;
      if(radius > 0) {
        dist = Geo::xy_distance(lat,lon,hh->get_latitude(),hh->get_longitude());
        if(radius < dist) {
          continue;
        }
      }
      
      // this household qualifies by location
      // find all susceptible housemates who qualify by age.
      int size = hh->get_size();
      //printf("IMPORT: house %d %s size %d fips %ld\n", i, hh->get_label(), size, hh->get_census_tract_fips()); fflush(stdout);
      
      for(int j = 0; j < size; ++j) {
        Person* person = hh->get_enrollee(j);
        Relationships* relationships = person->get_relationships();
        
        //Males
        if(sex == 'M' && person->get_sex() == 'M' && person->get_health()->is_susceptible(this->id)
           //mina change later ******************
           && relationships->check_relationships_eligiblity() //select people who are eligible for relationships?? No because people who are not eligible for relationships in our model could still be infected by HIV!??? keep for now and infect people younger than 45 years old
           ) {
          
          double rand1 = Random::draw_random();
          
          double age = person->get_real_age();
          int age_group = relationships->get_current_age_group();
          
          //if(age_bundle == 0 && (age_group == 0 || age_group == 1)) people.push_back(person);
          if(age_bundle == 0) {
            if ((age_group == 0 && rand1 <= 0.3) || (age_group == 1 && rand1 > 0.3) ) {   //30% for age group 0, and 70% for age gorup 1
              people.push_back(person);
            }
          }
          
          else if(age_bundle == 1 && (age_group == 2 || age_group == 3)) people.push_back(person);
          else if(age_bundle == 2 && (age_group == 4 || age_group == 5)) people.push_back(person);
          else if(age_bundle == 3 && (age_group == 6 || age_group == 7)) people.push_back(person);
          else if(age_bundle == 4 && age_group == 8 ) people.push_back(person);
        }
        
        // Females
        else if(sex == 'F' && person->get_sex() == 'F' &&  person->get_health()->is_susceptible(this->id)
                //mina change later ******************
                && relationships->check_relationships_eligiblity() //select people who are eligible for relationships?? No because people who are not eligible for relationships in our model could still be infected by HIV!??? keep for now and infect people younger than 45 years old
                ) {
          double rand1 = Random::draw_random();
          
          double age = person->get_real_age();
          int age_group = relationships->get_current_age_group();
          
          //if(age_bundle == 0 && (age_group == 0 || age_group == 1)) people.push_back(person);
          if(age_bundle == 0) {
            if ((age_group == 0 && rand1 <= 0.3) || (age_group == 1 && rand1 > 0.3) ) {   //30% for age group 0, and 70% for age gorup 1
              people.push_back(person);
            }
          }
          else if(age_bundle == 1 && (age_group == 2 || age_group == 3)) people.push_back(person);
          else if(age_bundle == 2 && (age_group == 4 || age_group == 5)) people.push_back(person);
          else if(age_bundle == 3 && (age_group == 6 || age_group == 7)) people.push_back(person);
          else if(age_bundle == 4 && age_group == 8 ) people.push_back(person);
        }
      }
    }
    
    //sort the candidates
    std::sort(people.begin(), people.end(), compare_id);
    
    //    if(age_bundle == 0) cout << " size 0 " << people.size() << endl;
    //    else if(age_bundle == 1) cout << " size 1 " << people.size() << endl;
    //    else if(age_bundle == 2) cout << " size 2 " << people.size() << endl;
    
    searches_within_given_location = import_aux2(people, imported_HIV_requested, imported_AIDS_requested, radius, day, searches_within_given_location);
    
    //cout << "searches_within_given_location " << searches_within_given_location << endl;
    
  } //End while(searches_within_given_location <= 10)
  
  // after 10 tries, return with a warning
  //FRED_VERBOSE(0, "IMPORT FAILURE test 1\n");
}

//*******************************************************
//*******************************************************

int HIV_Epidemic::import_aux2(vector<Person*> people, int imported_HIV_requested, int imported_AIDS_requested, double radius, int day, int searches_within_given_location){
  
  FRED_VERBOSE(0, "import_aux started.\n");
  
  int imported_cases_HIV = 0;
  int imported_cases_HIV_remaining = imported_HIV_requested - imported_cases_HIV;
  int imported_cases_AIDS = 0;
  int imported_cases_AIDS_remaining = imported_AIDS_requested - imported_cases_AIDS;
  
  
  
  //cout << "imported_cases_remaining  " << imported_cases_remaining << endl;
  
  FRED_VERBOSE(0, "IMPORT: seeking %d candidates, found %d\n", imported_cases_HIV_remaining, (int) people.size());
  
  if(imported_cases_HIV_remaining <= people.size()) {
    // we have at least the minimum number of candidates.
    for(int n = 0; n < imported_cases_HIV_remaining; ++n) {
      FRED_VERBOSE(1, "IMPORT candidate %d people.size %d\n", n, (int)people.size());
      
      // pick a candidate without replacement
      int pos = Random::draw_random_int(0,people.size()-1);
      Person* infectee = people[pos];
      people[pos] = people[people.size() - 1];
      people.pop_back();
      
      // infect the candidate
      FRED_VERBOSE(0, "HEALTH IMPORT infecting candidate %d id %d\n", n, infectee->get_id());
      infectee->become_exposed(this->id, NULL, NULL, day);
      become_exposed(infectee, day);
      imported_cases_HIV++;
      
      if(imported_cases_AIDS < imported_AIDS_requested){
        imported_cases_AIDS++;
        infectee->get_health()->get_hiv_infection()->set_has_AIDS();
        infectee->get_health()->get_hiv_infection()->set_HIV_stage(3);   //left: add stages in HIV_infection setup and assign cd4 and VL based on stage.
      }
      
      // 86% diagnosed
      double rand1 = Random::draw_random();
      if (strcmp(Global::Conditions.get_condition(this->id)->get_natural_history_model(), "hiv") == 0) {
        if(rand1 <= DIAGNOSED_RATE) infectee->get_health()->get_hiv_infection()->set_diagnosed();
      }
      
      
    }
    FRED_VERBOSE(0, "IMPORT SUCCESS: %d imported cases\n", imported_cases_HIV);
    return 11; // success!
    
  } else {
    // infect all the candidates
    for(int n = 0; n < people.size(); ++n) {
      Person* infectee = people[n];
      FRED_VERBOSE(0, "HEALTH IMPORT infecting candidate %d id %d\n", n, infectee->get_id());
      infectee->become_exposed(this->id, NULL, NULL, day);
      become_exposed(infectee, day);
      imported_cases_HIV++;
     
      if(imported_cases_AIDS < imported_AIDS_requested){
        imported_cases_AIDS++;
        infectee->get_health()->get_hiv_infection()-> set_has_AIDS();
        infectee->get_health()->get_hiv_infection()-> set_HIV_stage(3);
      }
      
      //86% diagnosed
      double rand1 = Random::draw_random();
      if(rand1 <= DIAGNOSED_RATE) infectee->get_health()->get_hiv_infection()-> set_diagnosed();
    }
  }
  
  if(radius > 0) {
    // expand the distance and try again
    radius = 2 * radius;
    FRED_VERBOSE(1, "IMPORT: increasing radius to %f\n", radius);
    searches_within_given_location++;
  }	else {
    // return with a warning
    FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases_HIV, imported_HIV_requested);
    getchar();
    return searches_within_given_location;
  }
  
  return searches_within_given_location;
  
} //end of import aux

//*******************************************************
//*******************************************************

void HIV_Epidemic::setup() {    // Epidemic setup() is run in Condition::setup()
  
  Epidemic::setup();
  
  FRED_VERBOSE(0, "HIV_epidemic setup\n");

  // initialize HIV specific-variables here:
  if((HIV_Epidemic::hiv_file1 = fopen( "hiv_file1.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  if((HIV_Epidemic::hiv_incidence = fopen( "hiv_incidence.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  if((HIV_Epidemic::hiv_prevalence = fopen( "hiv_prevalence.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }
  if((HIV_Epidemic::hiv_prevalence_age_group = fopen( "hiv_prevalence_age_group.txt", "w" )) == NULL ){
    printf("ERROR: The testing file was not opened\n" );
    exit(0);
  }

  fprintf(HIV_Epidemic::hiv_prevalence, "Day\tMale\tFemale\tMale Diagnosed\tFemale Diagnosed\n");
  fprintf(HIV_Epidemic::hiv_prevalence_age_group, "Day\t0\t1\t2\t3\t4\t5\t6\t0\t1\t2\t3\t4\t5\t6\n");

  this->total_qa_surv_time = 0.0;
  this->total_qa_disc_surv_time = 0.0;
  this->total_disc_surv_time = 0.0;
  this->total_surv_time = 0.0;
  this->mean_surv_time = 0.0;
  this->mean_qa_surv_time = 0.0;
  this->mean_qa_disc_surv_time = 0.0;
  this->mean_disc_surv_time = 0.0;
  this->mean_disc_hospital_cost = 0.0;
  this->mean_hospital_cost= 0.0;
  
  this->total_cost = 0.0;
  this->total_disc_cost = 0.0;
  this->total_drug_cost = 0.0;
  this->total_drug_disc_cost = 0.0;
  this->total_care_cost = 0.0;
  this->total_care_disc_cost = 0.0;
  this->total_hospital_cost = 0.0;
  this->total_hospital_disc_cost = 0.0;
  
  this->total_hiv_deaths = 0;
  this->total_age_deaths = 0;
  this->total_censored = 0;
  this->numDeaths = 0;
  
  this->total_muts_all_pats = 0;
  this->total_res_muts_all_pats = 0;
  this->TotalPatsRes1st1years = 0;
  this->TotalPatsRes1st5years = 0;
  
  this->TotalPatsRes1st1years = 0;
  this->TotalPatsRes1st5years = 0;
  this->totalAIDSOrDeath = 0;
  
  //imported HIV and AIDS cases in HIV_Epidemic::prepare()
  HIV_Epidemic::imported_HIV_requested_male[0] = 13;
  HIV_Epidemic::imported_HIV_requested_male[1] = 35;
  HIV_Epidemic::imported_HIV_requested_male[2] = 49;
  HIV_Epidemic::imported_HIV_requested_male[3] = 90;
  HIV_Epidemic::imported_HIV_requested_male[4] = 61;
  
  HIV_Epidemic::imported_HIV_requested_female[0] = 25;
  HIV_Epidemic::imported_HIV_requested_female[1] = 68;
  HIV_Epidemic::imported_HIV_requested_female[2] = 96;
  HIV_Epidemic::imported_HIV_requested_female[3]= 174;
  HIV_Epidemic::imported_HIV_requested_female[4]= 117;
  
  HIV_Epidemic::imported_AIDS_male[0] = 2;
  HIV_Epidemic::imported_AIDS_male[1] = 19;
  HIV_Epidemic::imported_AIDS_male[2] = 52;
  HIV_Epidemic::imported_AIDS_male[3] = 65;
  HIV_Epidemic::imported_AIDS_male[4] = 40; //55+
  
  HIV_Epidemic::imported_AIDS_female[0] = 3;
  HIV_Epidemic::imported_AIDS_female[1] = 25;
  HIV_Epidemic::imported_AIDS_female[2] = 70;
  HIV_Epidemic::imported_AIDS_female[3] = 87;
  HIV_Epidemic::imported_AIDS_female[4] = 54; //55+

  //General output (for each person. should be looped over all patients!!! MOVE from Disease.cc )
  if (SURV_RATES) fprintf(HIV_Epidemic::hiv_file1, "Start age\t Age SD\t Start CD4\t CD4 SD\t CD4_treat\t Start_hiv\t HIV SD\t Tox\t Util_dec\t Num_patients\t HIV Deaths\t Age Deaths\t Median Survival Time\t Mean Survival Time\t Median QALYS\t Mean QALYS\t");
  fprintf(HIV_Epidemic::hiv_file1, "\n\n");
  
  FRED_VERBOSE(0, "HIV_epidemic setup finish\n");
}

//*******************************************************
//*******************************************************

void HIV_Epidemic::update(int day) {
  FRED_VERBOSE(0, "HIV_Epidemic::update started.\n");
  
  // Epidemic::update(day);
  // code from Epidemic::update(day) needed for HIV Epidemic
  // import infections from unknown sources
  //if (strcmp(condition->get_natural_history_model(), "hiv") == 1) {  //mina added: we do not want this for hiv model
  //  get_imported_cases(day);}
  // Utils::fred_print_epidemic_timer("imported infections");
  
  // handle scheduled transitions
  int size = this->state_transition_event_queue.get_size(day);
  FRED_VERBOSE(1, "STATE_TRANSITION_EVENT_QUEUE day %d %s size %d\n",
	       day, Date::get_date_string().c_str(), size);
    
  for(int i = 0; i < size; ++i) {
    Person* person = this->state_transition_event_queue.get_event(day, i);
    update_state_of_person(person, day);
  }
  this->state_transition_event_queue.clear_events(day);

  // FRED_VERBOSE(0, "day %d ACTIVE_PEOPLE_LIST size = %d\n", day, this->active_people_list.size());

  // update list of active people
  for(person_set_iterator itr = this->active_people_list.begin(); itr != this->active_people_list.end(); ) {
    Person* person = (*itr);
    
    FRED_VERBOSE(1, "update_infection for person %d day %d\n", person->get_id(), day);
    person->update_condition(day, this->id);
    
    // handle case fatality
    if(person->is_case_fatality(this->id)) {
      // update epidemic fatality counters
      this->people_becoming_case_fatality_today++;
      this->total_case_fatality_count++;
      // record removed person
      this->current_removed_people++;
      if(Global::Enable_Visualization_Layer) {
        print_visualization_data_for_case_fatality(day, person);
      }
      if(Global::Report_Epidemic_Data_By_County) {
        int fips = Global::Places.get_county_for_place(person->get_household());
        this->county_infection_counts_map[fips].current_active--;
        this->county_infection_counts_map[fips].current_case_fatalities++;
      }
    }
    
    // note: case fatalities will be uninfected at this point
    if(person->is_infected(this->id) == false) {
      FRED_VERBOSE(1, "update_infection for person %d day %d - deleting from active_people_list list\n", person->get_id(), day);
      // delete from infected list
      this->active_people_list.erase(itr++);
    } else {
      
      // update person's mixing group infection counters
      person->update_household_counts(day, this->id);
      person->update_school_counts(day, this->id);
      // move on the next infected person
      ++itr;
    }
  }
  Utils::fred_print_epidemic_timer("schedules updated");
  
  // update the daily activities of infectious people
  Sexual_Transmission_Network* st_network = Global::Sexual_Partner_Network;
  for(person_set_iterator itr = this->infectious_people_list.begin(); itr != this->infectious_people_list.end(); ++itr ) {
    Person* person = (*itr);
    FRED_VERBOSE(0, "ADDING INF person %d to sexual transmission network\n", person->get_id());
    // this will insert the infectious person onto the infectious list in sexual partner network
    st_network->add_infectious_person(this->id, person);
  }
  FRED_VERBOSE(0, "calling spread infection\n");
  this->condition->get_transmission()->spread_infection(day, this->id, st_network);
  FRED_VERBOSE(0, "spread infection finished\n");
  st_network->clear_infectious_people(this->id);
  FRED_VERBOSE(0, "clear_infectious_people finished\n");

  this->add_element_to_hivdeaths_today();
  this->add_element_to_agedeaths_today();
  
  int patients = this->active_people_list.size();
  
  //  std::vector<double>CD4_counts;
  //  CD4_counts.reserve(patients);
  //  std::vector<double>VL_counts;
  //  VL_counts.reserve(patients);
  //mina change . we should find a way to set an array for patients in HIV_epidemic to calculate qa_time median for all patients
  //update mean survival time every day??? since the number of patients might change every day
  //  for(int i = 0; i < patients; i++) {
  //        Person * person = this->active_people_list[i];
  //        cd4_counts[i] = person->get_health()->get_hiv_infection()->get_cd4_count();
  //
  //  }
  //Mina change
  //  int i=0;
  //  for(std::set<Person*>::iterator it = this->active_people_list.begin(); it != this->active_people_list.end(); ) {
  //    Person* person = (*it);
  //
  //    CD4_counts[i] = person->get_health()->get_hiv_infection()->get_CD4_count();
  //    VL_counts[i] =  person->get_health()->get_hiv_infection()->get_VL_count();
  //    i++;
  //  }
  // printf( "Mina 1 \n");     //debug
  
  
  
  
  //Mina add prevalence report
  //left
  size = this->active_people_list.size();
  int size_f_total = 0;
  int size_m_total = 0;
  
  int size_f_diagnosed = 0;
  int size_m_diagnosed = 0;

  Person* person;
  
  int size_f_total_age_group[Sexual_Transmission_Network::ROW_age_group] = {0};
  int size_m_total_age_group[Sexual_Transmission_Network::ROW_age_group] = {0};
  
  //prevalence by gender (TOTAL)
  for(std::set<Person*>::iterator it = this->active_people_list.begin(); it != this->active_people_list.end(); ++it) {
    person = (*it);
    Relationships* relationships = person->get_relationships();
    bool diagnosed = false;
    int age_group = relationships->get_current_age_group();
    
    //diagnosed
    if (strcmp(Global::Conditions.get_condition(this->id)->get_natural_history_model(), "hiv") == 0) {
      diagnosed = person->get_health()->get_hiv_infection()-> get_diagnosed();
    }

    if (person->get_sex() == 'F') {
      size_f_total++;
      if (diagnosed) size_f_diagnosed++;
      
      for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
        if (age_group == i) size_f_total_age_group[i]++;
      }
      
        
      
    }
    else if (person->get_sex() == 'M') {
      size_m_total++;
      if (diagnosed) size_m_diagnosed++;
      
      for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
        if (age_group == i) size_m_total_age_group[i]++;
      }

    }
  }
  
  if(day%365 == 0) {
    fprintf(HIV_Epidemic::hiv_prevalence, "%d\t%d\t%d\t", day, size_m_total, size_f_total);
    fprintf(HIV_Epidemic::hiv_prevalence, "%d\t%d\n", size_m_diagnosed, size_f_diagnosed);
    
  }
  
  if(day%365 == 0) {
    fprintf(HIV_Epidemic::hiv_prevalence_age_group, "%d", day);
  
    for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
      fprintf(HIV_Epidemic::hiv_prevalence_age_group, "\t%d", size_m_total_age_group[i]);
    }
    for (int i = 0; i < Sexual_Transmission_Network::ROW_age_group; i++) {
      fprintf(HIV_Epidemic::hiv_prevalence_age_group, "\t%d", size_f_total_age_group[i]);
    }
    fprintf(HIV_Epidemic::hiv_prevalence_age_group, "\n");
  }

  
  
  FRED_VERBOSE(0, "day %d epidemic update finished for condition %d \n", day, id);
  
}

//*******************************************************
//*******************************************************

/*
void HIV_Epidemic::annual_update(Person* person, int day) {
  // int new_state = this->logit_model->get_state(person);
  // transition_person(person, day, new_state);
  //mina: can add stages like asymptomatic, symptomatic and AIDS to this...
}
*/

//*******************************************************
//*******************************************************

void HIV_Epidemic::report_condition_specific_stats(int day) {
  // put values that should appear in outfile here:
  // this is just an example for testing:
  // int hiv_count = day;
  // track_value(day, (char*)"HIV", hiv_count);
  // print additional daily output here:
}

//*******************************************************
//*******************************************************

void HIV_Epidemic::increment_total_qa_surv_time(double qa_time, double increment){
  this->total_qa_surv_time += qa_time;
  this->total_qa_disc_surv_time += qa_time / pow( (1+DISC_RATE), increment);
}

void HIV_Epidemic::increment_total_disc_surv_time(double increment){
  this->total_disc_surv_time += 1 / pow( (1+DISC_RATE), increment);
}

void HIV_Epidemic::increment_total_cost(double cost){
  this->total_cost += cost;
}

void HIV_Epidemic::increment_total_disc_cost(double cost, double increment){
  this->total_disc_cost += cost / pow( (1+DISC_RATE), increment);
}

void HIV_Epidemic::increment_total_drug_cost(double drug_cost){
  this->total_drug_cost += drug_cost;
}

void HIV_Epidemic::increment_total_drug_disc_cost(double drug_cost, double increment){
  this->total_drug_disc_cost += drug_cost / pow( (1+DISC_RATE), increment);
}

void HIV_Epidemic::increment_total_care_cost(double care_cost){
  this->total_care_cost += care_cost;
}

void HIV_Epidemic::increment_total_care_disc_cost(double care_cost, double increment){
  this->total_care_disc_cost += care_cost / pow( (1+DISC_RATE), increment);
}

void HIV_Epidemic::increment_total_hospital_cost(double hospital_cost){
  this->total_hospital_cost += hospital_cost;
}

void HIV_Epidemic::increment_total_hospital_disc_cost(double hospital_cost, double increment){
  this->total_hospital_disc_cost += hospital_cost / pow( (1+DISC_RATE), increment);
}

void  HIV_Epidemic::increment_total_hiv_deaths() {
  this->total_hiv_deaths++;
}

void  HIV_Epidemic::increment_total_age_deaths() {
  printf("\n MINA: incrementing total age_deaths.\n");
  this->total_age_deaths++;
}

void HIV_Epidemic::increment_hivdeaths(int cyclenum, int cycles_per_year){
  this->hivdeaths[int(cyclenum/CYCLES_PER_YEAR)]++;
}

void HIV_Epidemic::increment_agedeaths(int cyclenum){
  this->agedeaths[int(cyclenum/CYCLES_PER_YEAR)]++;
}

void  HIV_Epidemic::increment_total_AIDSorDeath(){
  this->totalAIDSOrDeath++;
}

void HIV_Epidemic::increment_total_hiv_surv_time(double duration){
  this->total_surv_time += duration;		//gives cumulative total years since patient entered model
  //this->timeTillDeath[this->numDeaths] = (double) duration;
  this->numDeaths++;
}

//*******************************************************
//*******************************************************
//used in Sexual_Transmission::attempt_transmission() when transmitted condition is hiv
void HIV_Epidemic::increment_hiv_incidence(Person* infectee){
  
  int age = infectee->get_age();
  int age_group = -1;
  int year = Global::Simulation_Day/365;
  
  if (age >= 15 && age < 20)
    age_group = 0;
  else if (age >= 20 && age < 25)
    age_group = 1;
  else if (age >= 25 && age < 30)
    age_group = 2;
  else if (age >= 30 && age < 35)
    age_group = 3;
  else if (age >= 35 && age < 40)
    age_group = 4;
  else if (age >= 40 && age < 45)
    age_group = 5;
  else if (age >= 45)
    age_group = 6;
  
  if(age_group!=-1) this->HIV_inc_agegroup[year][age_group]++;
  if(age_group!=-1 && infectee->get_sex() == 'F') this->HIV_inc_f_agegroup[year][age_group]++;
  if(age_group!=-1 && infectee->get_sex() == 'M') this->HIV_inc_m_agegroup[year][age_group]++;
  //  cout << "age_group " << age_group << "HIV incidence updated." << endl;
  //  getchar();
  
}

//*******************************************************
//*******************************************************

void HIV_Epidemic::end_of_run(){
  
  // print end-of-run statistics here:
  /*********************************************************
   Prints output.  The infrastructure for a variety of output is coded below, though most of it may be
   commented out.  Uncomment according to what you want to see printed.  At the same time, you may need
   to go into the rest of the code and uncomment parts related to the calculation of the output you want.
   ********************************************************/
  FRED_VERBOSE(0, "End of run for HIV epidemic started.\n");
  
  Person* person;
  int i, j, goodcurve = 1, num_alive;
  double median_surv_time, median_qalys, mean_qalys;
  double medianPriorHaart = 0, medianOnHaart = 0, medianPostHaart = 0;
  double meanPriorHaart = 0, meanOnHaart = 0, meanPostHaart = 0;
  int numPatientsOnReg1 = 0, numPatientsOnReg2 = 0, numPatientsOnReg3 = 0;
  double proportion_alive;
  double mean_time_before_salvage, mean_num_regs_before_salvage, mean_res_drugs_before_salvage;
  
  int patients = this->active_people_list.size();
  int total_hiv_patients = total_hiv_deaths + total_age_deaths + patients;
  double mean_surv_time_temp;
  int total_surv_time_temp = 0;
  
  if (total_hiv_patients == 0) return;
  
  /*  Important!  Adding Retention in Care code here because it adds costs to the
   total_cost which gets used to calculate the mean_cost below. */
  cout << "MINA DEBUG HIV EPIDEMIC: active_people_list size = " << patients << endl;
  
  //total_sur days for alive people:
  for(std::set<Person*>::iterator it = this->active_people_list.begin(); it != this->active_people_list.end(); ++it) {
    person = (*it);
    total_surv_time_temp = total_surv_time_temp + person->get_health()->get_hiv_infection()->get_survival_days_alive();
  }
  
  
  mean_surv_time_temp = double(total_surv_time_temp / patients);
  cout << " total_surv_day " << total_surv_time_temp << "\t\tmean: " << mean_surv_time_temp << endl;
  
  //mean_surv_time = (total_surv_time/(double)CYCLES_PER_YEAR)/PATIENTS;
  //mean_disc_surv_time = (total_disc_surv_time/(double)CYCLES_PER_YEAR)/PATIENTS;
  mean_surv_time = (total_surv_time/(double)CYCLES_PER_YEAR) / (total_hiv_deaths + total_age_deaths);
  mean_disc_surv_time = (total_disc_surv_time/(double)CYCLES_PER_YEAR)/total_hiv_patients;
  
  //mean_qalys = (total_qa_surv_time/(double)CYCLES_PER_YEAR)/PATIENTS;
  //mean_qa_disc_surv_time = (total_qa_disc_surv_time/(double)CYCLES_PER_YEAR)/PATIENTS;
  
  //median_surv_time = Get_Median(timeTillDeath, numDeaths )/(double)CYCLES_PER_YEAR;
  //median_qalys = Get_Median(qamedian, numDeaths)/(double)CYCLES_PER_YEAR;
  
  mean_cost = total_cost/total_hiv_patients;
  mean_disc_cost = total_disc_cost/total_hiv_patients;
  
  mean_lab_cost = total_lab_cost/total_hiv_patients;
  mean_disc_lab_cost = total_lab_disc_cost/total_hiv_patients;
  
  mean_drug_cost = total_drug_cost/total_hiv_patients;
  mean_disc_drug_cost = total_drug_disc_cost/total_hiv_patients;
  
  mean_care_cost = total_care_cost/total_hiv_patients;
  mean_disc_care_cost = total_care_disc_cost/total_hiv_patients;
  
  mean_hospital_cost = total_hospital_cost/total_hiv_patients;
  mean_disc_hospital_cost = total_hospital_disc_cost/total_hiv_patients;
  
  //output
  fprintf (HIV_Epidemic::hiv_file1, "\n  total_surv_time %.2f\tmean %.2f\t\n", total_surv_time, mean_surv_time);
  if (SURV_RATES) fprintf(HIV_Epidemic::hiv_file1, "%.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.2f\t %.3f\t %d\t %d\t %d\t %.3f\t %.3f\t %.3f\t %.3f\t", AVG_AGE, AVG_AGE_SD, 3333333.0, AVG_CD4_SD, CD4_TREAT, AVG_HIV, AVG_HIV_SD, HAART_TOX, DELTA_UTILITY_WITH_HAART, total_hiv_patients, total_hiv_deaths, total_age_deaths, median_surv_time, mean_surv_time, median_qalys, mean_qalys);
  
  
  
  /*
   if (ACTIVE_HAART_INFO || REG_INFO_FOR_SENSITIVITY)
   {
   //values output below
   mean_time_before_salvage = ((double)total_time_on_haart_before_salvage/(double)num_exhausted_clean_regs)/(double)CYCLES_PER_YEAR;
   mean_num_regs_before_salvage = (double)total_num_regs_before_salvage / (double)num_exhausted_clean_regs;
   mean_res_drugs_before_salvage = (double)total_res_drugs_before_salvage / (double)num_exhausted_clean_regs;
   }
   
   if (TIME_TO_TREATMENT_FAILURE_OF_REGIMENS_1_2_3 || REG_INFO_FOR_SENSITIVITY )
   {
   for (i=0; i<3; i++)
   {
   //initialize
   mean_reg_time[i]=0;
   median_reg_time[i]=0;
   
   if (numPeopleCompletedReg[i])
   {
   mean_reg_time[i] = (total_completed_reg_cycles[i]/(double)CYCLES_PER_YEAR)/numPeopleCompletedReg[i];
   median_reg_time[i] = Get_Median(cyclesToCompleteReg[i], numPeopleCompletedReg[i])/(double)CYCLES_PER_YEAR;
   }
   } 	//output below
   }
   
   //In the following, prob1 is Prob(survive to time ti | survived up to time t(i-1)).
   //Prob2 is the Kaplan-Meier estimator of the survival probability at time t(i).
   if (KAPLAN_MEIER_CURVE_OF_TIME_TO_TREATMENT_FAILURE_REG_1_2_3)
   {
   double tttfDat_prob1;
   double tttfDat_prob2;
   double tttfDat_prevProb2[3];
   int tttfDat_total;
   
   int dontStop = 1;
   
   //initialize to some nonsense value greater than 0;
   tttfDat_prevProb2[0] = tttfDat_prevProb2[1] = tttfDat_prevProb2[2] = 999999;
   
   fprintf (output, "\n\nData for Kaplan Meier Curve of Time To Treatment Failure\n\n\n cycle_num\tyears\tprop_on_reg1\tprop_on_reg2\tprop_on_reg3\n");
   
   for (int i=0; i<TIME*CYCLES_PER_YEAR && dontStop; i++)
   {
   fprintf(output, "%d\t %f", i, i/double(CYCLES_PER_YEAR));
   
   dontStop = 0;
   
   for (int j=0; j<3; j++)
   {
   //Stop calculating when all have reached the event
   if (tttfDat_prevProb2[j] >0 )
   {
   tttfDat_total = tttfDat[i][j].survived + tttfDat[i][j].censored + tttfDat[i][j].failed;
   
   //We want to do this until we run out of patients alive.  But we run into a problem
   //if the last living patient was censored.  If the last patient was censored, the probability
   //of being alive doesn't change beyond that time.
   if (tttfDat_total !=0)
   {
   tttfDat_prob1 = 1 - (double)tttfDat[i][j].failed/(double)tttfDat_total;
   dontStop = 1;
   }
   else tttfDat_prob1 = 1;
   
   //The first doesn't get multiplied
   if (i == 0) tttfDat_prob2 = tttfDat_prob1;
   else tttfDat_prob2 = tttfDat_prob1 * tttfDat_prevProb2[j];
   
   tttfDat_prevProb2[j] = tttfDat_prob2;
   
   fprintf(output, "\t%f", tttfDat_prob2);
   
   }
   
   else fprintf(output, "\t0", 0);
   }
   
   fprintf(output, "\n");
   }
   }
   
   if (HIV_VS_AGE_DEATHS )
   {
   fprintf (output, "HIV_VS_AGE_DEATHS\n\n");
   
   fprintf(output, "Year\t HIV deaths\t Age deaths\t Num Censored\n");
   
   for ( i = 0 ; i < TIME ; i++ )
   {
   fprintf(output, "%d\t %d\t %d\t %d\n", i, hivdeaths[i], agedeaths[i], agedeaths[i]);
   }
   }
   
   if (YEARLY_CD4_AND_YEARLY_CHANGE_IN_CD4_AFTER_START_OF_THERAPY )
   {
   fprintf (output, "\n\nYEARLY_CD4_AND_YEARLY_CHANGE_IN_CD4_AFTER_START_OF_THERAPY\n\n\nYear\tmedian CD4\tmeanCD4\n\n");
   
   for ( i = 0 ; i < TIME && numAliveAtYearIntoHaart[i]!=0; i++ )
   {
   double medianCD4, meanCD4, totalCD4;
   
   medianCD4 = Get_Median(YearlyCD4[i], numAliveAtYearIntoHaart[i]);
   
   //Calculate mean
   totalCD4=0;
   for (j=0; j<numAliveAtYearIntoHaart[i]; j++) totalCD4 += YearlyCD4[i][j];
   meanCD4 = totalCD4/numAliveAtYearIntoHaart[i];
   
   fprintf (output, "%d\t%.2f\t%.2f\n", i, medianCD4, meanCD4);
   }
   }
   
   if (KAPLAN_MEIER_SURVIVAL_CURVE )
   {
   fprintf (output, "\n\nData for Kaplan Meier Curve (Proportion Alive)\n\n\nCycle\tYears\thiv_deaths\tage_deaths\tnum_censored\tnum_alive\n\n");
   
   num_alive = PATIENTS;
   for ( j = 0 ; (j < TIME*CYCLES_PER_YEAR) && (num_alive != 0); j++)
   {
   //SurvivalPlot[0][j] are the HIV deaths, [1][j] are the Age deaths, [2][j] are Censored
   num_alive = num_alive - (SurvivalPlot[0][j] + SurvivalPlot[1][j] + SurvivalPlot[2][j]);
   proportion_alive = num_alive/(double)PATIENTS;
   fprintf (output, "%d\t %.3f\t %d\t %d\t %d\t %d\n ", j, j/double(CYCLES_PER_YEAR), SurvivalPlot[0][j], SurvivalPlot[1][j], SurvivalPlot[2][j], num_alive);
   }
   }
   
   #ifdef ONE_WAY_SENSITIVITY_ANALYSIS
   if (REG_INFO_FOR_SENSITIVITY)
   {
   senseFile << label << "\t" << start_age << "\t" << start_cd4 << "\t" << cd4_treat << "\t" << start_hiv << "\t" << haart_tox << "\t" << DELTA_UTILITY_WITH_HAART << "\t" << PATIENTS << "\t" << total_hiv_deaths << "\t" << total_age_deaths << "\t" <<  median_surv_time << "\t" << mean_surv_time << "\t" << median_qalys << "\t" << mean_qalys << "\t" << mean_time_before_salvage << "\t" << mean_num_regs_before_salvage << "\t" << mean_res_drugs_before_salvage << "\t" << mean_reg_time[0] << "\t" << mean_reg_time[1] << "\t" << mean_reg_time[2] << "\t" << num_exhausted_clean_regs;
   
   //Add columns at the end for number of initial mutations of each drugs class
   for (int drugclass=0; drugclass<6; drugclass++)
   {
   senseFile<<"\t"<<(double)total_res_after_initial_muts[drugclass]/PATIENTS;
   }
   senseFile<<endl;
   }
   else senseFile << label << "\t" << start_age << "\t" << start_cd4 << "\t" << cd4_treat << "\t" << start_hiv << "\t" << haart_tox << "\t" << DELTA_UTILITY_WITH_HAART << "\t" << PATIENTS << "\t" << total_hiv_deaths << "\t" << total_age_deaths << "\t" <<  median_surv_time << "\t" << mean_surv_time << "\t" << median_qalys << "\t" << mean_qalys << "\t" << endl;
   #endif
   
   #ifdef ONE_WAY_SENSITIVITY_ANALYSIS_RIC
   senseFile << label <<"\t"<< outreach_interv_enabled<< "\t"<< risk_reduction_interv_enabled<< "\t"<<secondary_prevention_interv_enabled << "\t"<<start_cd4 <<"\t"<< start_age <<"\t"<< comp <<"\t"<< cost_reg1_poor <<"\t"<< cost_reg2_poor <<"\t"<< cost_of_care_poor <<"\t"<< hospital_cost_poor <<"\t"<< prob_es1_to_es2 << "\t" << pre_ART_mult_for_prob_es1_to_es2 << "\t" << prob_es2_to_es4 << "\t" << prob_es5_to_es6 << "\t"<< mutation_mult_es4 << "\t"<<outreach_prob_identify<<"\t"<<outreach_prob_find<<"\t"<<outreach_prob_relink<<"\t"<<outreach_start_trigger<<"\t"<<outreach_end_trigger<<"\t"<<outreach_finding_cost<<"\t"<<outreach_cd4<<"\t"<<risk_reduction_rr_prob_es1_to_es2<<"\t"<<risk_reduction_intervention_cost<<"\t"<<risk_reduction_cd4<<"\t"<<secondary_prevention_rr_prob_es1_to_es2<<"\t"<<secondary_prevention_intervention_cost<<"\t"<<secondary_prevention_cd4<<"\t"<< prob_ART_init_at_CD4_treat <<"\t"<< PATIENTS <<"\t"<<total_ES1_HIV_death_known<<"\t"<< total_ES1_HIV_death_unknown << "\t" << total_ES1_AGE_death_known<<"\t"<<total_ES1_AGE_death_unknown<<"\t"<<total_ES3_HIV_death<<"\t"<<total_ES3_AGE_death<<"\t"<<total_ES4_HIV_death<<"\t"<<total_ES4_AGE_death <<"\t"<<mean_ES1_time<<"\t"<<mean_ES3_time<<"\t"<<mean_ES4_time<<"\t"<<mean_time_onART<< "\t" << mean_surv_time << "\t"<<mean_disc_surv_time<<"\t"<<median_surv_time <<"\t"<<mean_time_VL_less1000<<"\t"<<median_qalys << "\t" << mean_qalys << "\t" <<mean_cost<<"\t"<<mean_disc_cost<<"\t"<<mean_lab_cost<<"\t"<<mean_drug_cost<<"\t"<<mean_care_cost<<"\t"<<mean_hospital_cost<<"\t"<<mean_outreach_cost<<"\t"<<mean_RR_intervention_cost<<"\t"<<mean_SP_intervention_cost<<"\t"<<total_num_outreach<<"\t"<<mean_num_outreach<<"\t"<<total_LTFU<<"\t"<<mean_LTFU<<"\t"<< endl;
   #endif
   
   
   if (CHANGE_IN_VL_and_CD4_1_3_5_YEARS_INTO_THERAPY)
   {
   fprintf(Global::hiv_file1, "\t");
   for (int years=0; years<3; years++) fprintf(Global::hiv_file1, "%.2f\t ", Get_Median(ChangeInCD4_1_3_5_YearsIntoTherapy[years], numAlive_1_3_5_YearsIntoHaart[years]));
   fprintf(Global::hiv_file1, "\t");
   for (int years=0; years<3; years++) fprintf(Global::hiv_file1, "%.2f\t ", Get_Median(ChangeInVL_1_3_5_YearsIntoTherapy[years], numAlive_1_3_5_YearsIntoHaart[years]));
   }
   */
  
  //fprintf(HIV_Epidemic::hiv_incidence, "\tAge group\n");
  fprintf(HIV_Epidemic::hiv_incidence, "Year\t0\t1\t2\t3\t4\t5\t6\n");
  
  int years = Global::Simulation_Day/365;
  
  cout << " HIV INCIDENCE: year 1 " << HIV_inc_agegroup[0][1] << endl;
  for (int i = 0 ; i < years; i++) {
    fprintf(HIV_Epidemic::hiv_incidence, "%d", i);
    
    for (int j = 0 ; j < Sexual_Transmission_Network::ROW_age_group ; j++) {
      fprintf(HIV_Epidemic::hiv_incidence, "\t%d", HIV_inc_agegroup[i][j]);
    }
    fprintf(HIV_Epidemic::hiv_incidence, "\n");
  }
  
  cout << " HIV INCIDENCE MALE: year 1 " << HIV_inc_m_agegroup[0][1] << endl;
  for (int i = 0 ; i < years; i++) {
    fprintf(HIV_Epidemic::hiv_incidence, "%d", i);
    
    for (int j = 0 ; j < Sexual_Transmission_Network::ROW_age_group ; j++) {
      fprintf(HIV_Epidemic::hiv_incidence, "\t%d", HIV_inc_m_agegroup[i][j]);
    }
    fprintf(HIV_Epidemic::hiv_incidence, "\n");
  }

  cout << " HIV INCIDENCE FEMALE: year 1 " << HIV_inc_f_agegroup[0][1] << endl;
  for (int i = 0 ; i < years; i++) {
    fprintf(HIV_Epidemic::hiv_incidence, "%d", i);
    
    for (int j = 0 ; j < Sexual_Transmission_Network::ROW_age_group ; j++) {
      fprintf(HIV_Epidemic::hiv_incidence, "\t%d", HIV_inc_f_agegroup[i][j]);
    }
    fprintf(HIV_Epidemic::hiv_incidence, "\n");
  }

  FRED_VERBOSE(0,"End of run for HIV epidemic finished.\n");
  
}   //end of end_of_run(){

//*******************************************************
//*******************************************************

void HIV_Epidemic::add_element_to_hivdeaths_today(){
  this->hivdeaths.push_back(0);   //create element in vector, for number of HIV deaths today
}
void HIV_Epidemic::increment_hivdeaths_today(int today){
  this->hivdeaths.at(today)++;
}
void HIV_Epidemic::add_element_to_agedeaths_today(){
  this->agedeaths.push_back(0);   //create element in vector, for number of HIV deaths today
}
void HIV_Epidemic::increment_agedeaths_today(int today){
  this->agedeaths.at(today)++;
}

//*******************************************************
//*******************************************************

int HIV_Epidemic::time_to_care(){   //number of days after diagnosis that patient seek care
  //Keruly et al. 2007
  // picked uniform distribution
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(69, 1094);
  return dis(gen);
  
}

