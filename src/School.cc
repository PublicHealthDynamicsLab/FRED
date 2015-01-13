/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: School.cc
//

#include "School.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Place_List.h"
#include "Classroom.h"
#include "Date.h"
#include "Utils.h"

//Private static variables that will be set by parameter lookups
double * School::school_contacts_per_day;
double *** School::school_contact_prob;
int School::school_classroom_size = 0;
char School::school_closure_policy[80];
int School::school_closure_day = 0;
double School::school_closure_threshold = 0.0;
double School::individual_school_closure_threshold = 0.0;
int School::school_closure_cases = -1;
int School::school_closure_period = 0;
int School::school_closure_delay = 0;
int School::school_summer_schedule = 0;
char School::school_summer_start[8];
char School::school_summer_end[8];

//Private static variable to assure we only lookup parameters once
bool School::school_parameters_set = false;
bool School::global_closure_is_active = false;
int School::global_close_date = 0;
int School::global_open_date = 0;


School::School( const char *lab, double lon, double lat, Place* container, Population *pop ) {
  type = SCHOOL;
  setup( lab, lon, lat, container, pop );
  get_parameters(Global::Diseases);
  for (int i = 0; i < MAX_GRADE; i++) {
    students_with_age[i] = 0;
    classrooms[i].clear();
  }
  total_classrooms = 0;
  closure_dates_have_been_set = false;
}

void School::prepare() {
  int diseases = Global::Diseases;
  for (int s = 0; s < diseases; s++) {
    susceptibles[s].clear();
    infectious[s].clear();
    Sympt[s] = S[s] = I[s] = 0;
    total_cases[s] = total_deaths[s] = 0;
  }
  for (int i = 0; i < MAX_GRADE; i++) {
    next_classroom[i] = 0;
    next_classroom_without_teacher[i] = 0;
  }
  close_date = 0;
  open_date = 0;
  if (Global::Verbose > 2) {
    printf("prepare place: %d\n", id);
    print(0);
    fflush(stdout);
  }
}


void School::get_parameters(int diseases) {
  char param_str[80];
  
  if (School::school_parameters_set) return;
  
  School::school_contacts_per_day = new double [ diseases ];
  School::school_contact_prob = new double** [ diseases ];
  
  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "school_contacts[%d]", s);
    Params::get_param((char *) param_str, &School::school_contacts_per_day[s]);
    
    sprintf(param_str, "school_prob[%d]", s);
    n = Params::get_param_matrix(param_str, &School::school_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nschool_contact_prob:\n");
      for (int i  = 0; i < n; i++)  {
        for (int j  = 0; j < n; j++) {
          printf("%f ", School::school_contact_prob[s][i][j]);
        }
        printf("\n");
      }
    }
  }
  
  Params::get_param_from_string("school_classroom_size", &School::school_classroom_size);
  Params::get_param_from_string("school_closure_policy", School::school_closure_policy);
  Params::get_param_from_string("school_closure_day", &School::school_closure_day);
  Params::get_param_from_string("school_closure_threshold", &School::school_closure_threshold);
  Params::get_param_from_string("individual_school_closure_threshold", &School::individual_school_closure_threshold);
  Params::get_param_from_string("school_closure_cases", &School::school_closure_cases);
  Params::get_param_from_string("school_closure_period", &School::school_closure_period);
  Params::get_param_from_string("school_closure_delay", &School::school_closure_delay);
  Params::get_param_from_string("school_summer_schedule", &School::school_summer_schedule);
  Params::get_param_from_string("school_summer_start", School::school_summer_start);
  Params::get_param_from_string("school_summer_end", School::school_summer_end);
 
  School::school_parameters_set = true;
}

int School::get_group(int disease_id, Person * per) {
  int age = per->get_age();
  if (age <12) { return 0; }
  else if (age < 16) { return 1; }
  else if (age < Global::ADULT_AGE) { return 2; }
  else return 3;
}

double School::get_transmission_prob(int disease_id, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease_id, i);
  int col = get_group(disease_id, s);
  double tr_pr = School::school_contact_prob[disease_id][row][col];
  return tr_pr;
}


bool School::should_be_open(int day, int disease_id) {
  
  // no students
  if (N == 0) return false;

  // summer break
  if (School::school_summer_schedule > 0 && 
      Date::day_in_range_MMDD(Global::Sim_Current_Date, School::school_summer_start, School::school_summer_end)) {
    if (Global::Verbose > 1) {
      fprintf(Global::Statusfp,"School %s closed for summer\n", label);
      fflush(Global::Statusfp);
    }
    return false;
  }

  // stick to previously made decision to close
  if (closure_dates_have_been_set) {
    return is_open(day);
  }

  // global school closure policy in effect
  if (strcmp(School::school_closure_policy, "global") == 0) {
    apply_global_school_closure_policy(day, disease_id);
    return is_open(day);
  }

  // individual school closure policy in effect
  if (strcmp(School::school_closure_policy, "individual") == 0) {
    apply_individual_school_closure_policy(day, disease_id);
    return is_open(day);
  }

  // if school_closure_policy is not recognized, then open
  return true;
}

void School::apply_global_school_closure_policy(int day, int disease_id) {

  // Only test triggers for school closure if not global closure is not already activated
  if (School::global_closure_is_active == false) {
    // Setting school_closure_day > -1 overrides other global triggers.
    // Close schools if the closure date has arrived (after a delay)
    if (School::school_closure_day > -1) {
      if (School::school_closure_day <= day) {
	// the following only happens once
	School::global_closure_is_active = true;
	School::global_close_date = day + School::school_closure_delay;
	School::global_open_date = day + School::school_closure_delay + School::school_closure_period;
      }
    }
    else {
      // Close schools if the global clinical attack rate has reached the threshold (after a delay)
      Disease * disease = Global::Pop.get_disease(disease_id);
      if (School::school_closure_threshold <= disease->get_clinical_attack_rate()) {
	// the following only happens once
	School::global_closure_is_active = true;
	School::global_close_date = day + School::school_closure_delay;
	School::global_open_date = day + School::school_closure_delay + School::school_closure_period;
      }
    }
  }
  if (School::global_closure_is_active) {
    // set close and open dates for this school (only once)
    close_date = School::global_close_date;
    open_date = School::global_open_date;
    closure_dates_have_been_set = true;

    // log this school closure decision
    if (Global::Verbose > 1) {
      Disease * disease = Global::Pop.get_disease(disease_id);
      printf("School %d day %d ar %5.2f cases = %d / %d (%5.2f) close_date %d open_date %d\n",
	     id, day, disease->get_clinical_attack_rate(),
	     get_total_cases(disease_id), N, get_clinical_attack_rate(disease_id), close_date, open_date);
    }
  }
}

void School::apply_individual_school_closure_policy(int day, int disease_id) {

  // don't apply any policy prior to School::school_closure_day
  if (day <= School::school_closure_day) {
    return;
  }

  // don't apply any policy before the epdemic reaches a noticeable threshold
  Disease * disease = Global::Pop.get_disease(disease_id);
  if (disease->get_clinical_attack_rate() < School::school_closure_threshold) {
    return;
  }

  bool close_this_school = false;

  // if school_closure_cases > -1 then close if this number of cases occurs
  if (School::school_closure_cases != -1) {
    close_this_school = (School::school_closure_cases <= Sympt[disease_id]);
  }
  else {
    // close if attack rate threshold is reached
    close_this_school = (School::individual_school_closure_threshold <= get_incidence_rate(disease_id));
  }

  if (close_this_school) {
    // set close and open dates for this school (only once)
    close_date = day + School::school_closure_delay;
    open_date = day + School::school_closure_delay + School::school_closure_period;
    closure_dates_have_been_set = true;

    // log this school closure decision
    if (Global::Verbose > 0) {
      Disease * disease = Global::Pop.get_disease(disease_id);
      printf("School %d day %d ar %5.2f cases = %d / %d (%5.2f) close_date %d open_date %d\n",
	     id, day, disease->get_clinical_attack_rate(),
	     get_total_cases(disease_id), N, get_clinical_attack_rate(disease_id), close_date, open_date);
    }
  }
}
  

double School::get_contacts_per_day(int disease_id) {
  return School::school_contacts_per_day[disease_id];
}

void School::enroll(Person * per) {
  N++;
  int age = per->get_age();
  if (age < MAX_GRADE) {
    students_with_age[age]++;
  }
  else {
    students_with_age[MAX_GRADE-1]++;
  }
}

void School::unenroll(Person * per) {
  N--;
  int age = per->get_age();
  if (age < MAX_GRADE) {
    students_with_age[age]--;
  }
  else {
    students_with_age[MAX_GRADE-1]--;
  }
}

void School::print(int disease_id) {
  fprintf(Global::Statusfp, "Place %d label %s type %c ", id, label, type);
  fprintf(Global::Statusfp, "S %d I %d N %d\n", S[disease_id], I[disease_id], N);
  for (int g = 0; g < MAX_GRADE; g++) {
    fprintf(Global::Statusfp, "%d students with age %d | ", students_with_age[g], g);
  }
  fprintf(Global::Statusfp, "\n");
  fflush(Global::Statusfp);
}

int School::get_number_of_rooms() {
  int total_rooms = 0;
  if (School::school_classroom_size == 0)
    return 0;
  for (int a = 0; a < MAX_GRADE; a++) {
    int n = students_with_age[a];
    next_classroom[a] = 0;
    next_classroom_without_teacher[a] = 0;
    if ( n == 0 ) continue;
    int rooms = n / School::school_classroom_size;
    if ( n % School::school_classroom_size ) rooms++;
    total_rooms += rooms;
  }
  return total_rooms;
}

void School::setup_classrooms( Allocator< Classroom > & classroom_allocator ) {
  if (School::school_classroom_size == 0)
    return;

  for (int a = 0; a < MAX_GRADE; a++) {
    int n = students_with_age[a];
    next_classroom[a] = 0;
    next_classroom_without_teacher[a] = 0;
    if (n == 0) continue;
    int rooms = n / School::school_classroom_size;
    if (n % School::school_classroom_size) rooms++;
   
    FRED_STATUS( 1, "school %d %s age %d number %d rooms %d\n",
        id, label, a, n, rooms );
    
    for (int c = 0; c < rooms; c++) {
      char new_label[128];
      sprintf(new_label, "%s-%02d-%02d", this->get_label(), a, c+1);

      Place *p = new ( classroom_allocator.get_free() ) Classroom( new_label,
           this->get_longitude(),
           this->get_latitude(),
           this,
           this->get_population());

      //Global::Places.add_place(p);
      classrooms[a].push_back(p);
      total_classrooms++;

      //FRED_STATUS( 1, "school %d %s age %d added classroom %d %s %d\n",
      //    id, label, a, c, p->get_label(), p->get_id() );
    }
  }
}


Place * School::assign_classroom(Person *per) {
  if (School::school_classroom_size == 0)
    return NULL;
  int age = per->get_age();
  // if (age < Global::ADULT_AGE) {

  // assign classroom to a student
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp,
	    "assign classroom for student %d in school %d %s for age %d\n",
	    per->get_id(), id, label, age);
    fflush(Global::Statusfp);
  }
  assert((age < MAX_GRADE) && (classrooms[age].size() > 0));

  // pick next classroom, round-robin
  int c = next_classroom[age];
  next_classroom[age]++;
  if (next_classroom[age]+1 > (int) classrooms[age].size())
    next_classroom[age] = 0;
    
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "room = %d %s %d\n",
	    c, classrooms[age][c]->get_label(), classrooms[age][c]->get_id());
    fflush(Global::Statusfp);
  }
  return classrooms[age][c];

  // the following code is for assigning teachers -- NOT YET IMPLEMENTED
  /*
  }
  else {
    // assign classroom to a teacher
    if (Global::Verbose > 1) {
      fprintf(Global::Statusfp,
	      "assign classroom for teacher %d in school %d %s for age %d == ",
	      per->get_id(), id, label, age);
      fflush(Global::Statusfp);
    }

    // pick next classroom, round-robin
    for (int a = 0; a < MAX_GRADE; a++) {
      int n = (int) classrooms[a].size();
      if (n == 0) continue;
      if (next_classroom_without_teacher[a] < n) {
        int c = next_classroom_without_teacher[a];
        next_classroom_without_teacher[a]++;
        if (Global::Verbose > 1) {
          fprintf(Global::Statusfp, "room = %d %s %d\n",
                  c, classrooms[a][c]->get_label(), classrooms[a][c]->get_id());
          fflush(Global::Statusfp);
        }
        return classrooms[a][c];
      }
    }

    // all classrooms have a teacher, so assign to a random classroom
    int x = IRAND(0,total_classrooms-1);
    for (int a = 0; a < MAX_GRADE; a++) {
      for (unsigned int c = 0 ; c < classrooms[a].size(); c++) {
        if (x == 0) {
          if (Global::Verbose > 1) {
            fprintf(Global::Statusfp, "room = %d %s %d\n",
		    c, classrooms[a][c]->get_label(), classrooms[a][c]->get_id());
            fflush(Global::Statusfp);
          }
          return classrooms[a][c];
        }
        else { x--; }
      } 
    }
    Utils::fred_abort("Help! Can't find classroom for teacher\n\n"); 
  }
  */

  //Should never get here (i.e. fred_abort will happen), but avoids the compiler warning
  return NULL;
}

