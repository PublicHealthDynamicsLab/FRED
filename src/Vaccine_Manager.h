/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Vaccine_Manager.h
//

#ifndef _FRED_VACCINE_MANAGER_H
#define _FRED_VACCINE_MANAGER_H

#define VACC_NO_PRIORITY  0
#define VACC_AGE_PRIORITY 1
#define VACC_ACIP_PRIORITY 2

#define VACC_DOSE_NO_PRIORITY 0
#define VACC_DOSE_FIRST_PRIORITY 1
#define VACC_DOSE_RAND_PRIORITY 2
#define VACC_DOSE_LAST_PRIORITY 3

#include <list>
#include <vector>
#include <string>
#include "Manager.h"

using namespace std;

class Manager;
class Population;
class Vaccines;
class Person;
class Policy;
class Timestep_Map;

class Vaccine_Manager: public Manager {
  //Vaccine_Manager handles a stock of vaccines
public:
  // Construction and Destruction
  Vaccine_Manager();
  Vaccine_Manager(Population *_pop);
  ~Vaccine_Manager();
  
  //Parameters Access
  bool do_vaccination()                     const { return do_vacc; }
  Vaccines* get_vaccines()                  const { return vaccine_package; }
  list < Person* > get_priority_queue()     const { return priority_queue;}
  list < Person* > get_queue()              const { return queue;}
  int get_number_in_priority_queue()        const { return priority_queue.size(); }
  int get_number_in_reg_queue()             const { return queue.size(); }
  int get_current_vaccine_capacity()        const { return current_vaccine_capacity; }
  
  // Vaccination Specific Procedures
  void fill_queues();
  void vaccinate(int day);
  void add_to_queue(Person* person);                 //Adds person to queue based on current policies
  void remove_from_queue(Person* person);            //Remove person from the vaccine queue
  void add_to_priority_queue_random(Person* person); //Adds person to the priority queue in a random spot
  void add_to_regular_queue_random(Person* person);  //Adds person to the regular queue in a random spot
  void add_to_priority_queue_begin(Person* person);  //Adds person to the beginning of the priority queue
  void add_to_priority_queue_end(Person* person);    //Adds person to the end of the priority queue

  //Paramters Access Members
  int get_vaccine_priority_age_low()  const {return vaccine_priority_age_low;}
  int get_vaccine_priority_age_high() const {return vaccine_priority_age_high;}
  int get_vaccine_dose_priority()     const {return vaccine_dose_priority;}
  string get_vaccine_dose_priority_string() const;
  
  // Utility Members
  void update(int day);
  void reset();
  void print();
  
private:
  Vaccines* vaccine_package;              //Pointer to the vaccines that this manager oversees
  list < Person* > priority_queue;      //Queue for the priority agents
  list < Person* > queue;               //Queue for everyone else
  
  //Parameters from Input 
  bool   do_vacc;                         //Is Vaccination being performed
  bool   vaccine_priority_only;           //True - Vaccinate only the priority
  
  int vaccine_priority_age_low;           //Age specific priority
  int vaccine_priority_age_high;
  int vaccine_dose_priority;              //Defines where people getting multiple doses fit in the queue
                                          // See defines above for values
  
  Timestep_Map *vaccination_capacity_map; // How many people can be vaccinated now,
                                          // gets its value from the capacity change list
  int current_vaccine_capacity;           // variable to keep track of how many persons this 
                                          // can vaccinate each timestep.
};


#endif


