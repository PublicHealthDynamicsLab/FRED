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
// File: Vaccine_Manager.cpp
//
#include "Manager.h"
#include "Vaccine_Manager.h"
#include "Policy.h"
#include "Vaccine_Priority_Policies.h"
#include "Population.h"
#include "Vaccines.h"
#include "Vaccine.h"
#include "Person.h"
#include "Health.h"
#include "Activities.h"
#include "Params.h"
#include "Random.h"
#include "Global.h"
#include "Timestep_Map.h"
#include "Utils.h"
#include "Tracker.h"

#include <algorithm>

Vaccine_Manager::Vaccine_Manager() {
  this->vaccine_package = NULL;
  this->vaccine_priority_age_low = -1;
  this->vaccine_priority_age_high = -1;
  this->current_vaccine_capacity = -1;
  this->vaccine_priority_only = false;
  this->vaccination_capacity_map = NULL;
  this->do_vacc = false;
  this->vaccine_dose_priority = -1;
  this->refresh_vaccine_queues_daily = false;
  this->vaccinate_symptomatics = false;
}

Vaccine_Manager::Vaccine_Manager(Population *_pop) :
  Manager(_pop) {

  this->pop = _pop;

  this->vaccine_package = new Vaccines();
  int num_vaccs = 0;
  Params::get_param_from_string("number_of_vaccines", &num_vaccs);
  if(num_vaccs > 0) {
    this->vaccine_package->setup();
    this->vaccine_package->print();
    this->do_vacc = true;
  } else {    // No vaccination specified.
    this->vaccine_priority_age_low = -1;
    this->vaccine_priority_age_high = -1;
    this->vaccination_capacity_map = NULL;
    this->current_vaccine_capacity = -1;
    this->vaccine_dose_priority = -1;
    this->vaccine_priority_only = false;
    this->do_vacc = false;
    return;
  }
  // ACIP Priority takes precidence
  int do_acip_priority;
  current_policy = VACC_NO_PRIORITY;
  Params::get_param_from_string("vaccine_prioritize_acip", &do_acip_priority);
  if(do_acip_priority == 1) {
    cout << "Vaccination Priority using ACIP recommendations\n";
    cout << "   Includes: \n";
    cout << "        Ages 0 to 24\n";
    cout << "        Pregnant Women\n";
    cout << "        Persons at risk for complications\n";
    this->current_policy = VACC_ACIP_PRIORITY;
    this->vaccine_priority_age_low = 0;
    this->vaccine_priority_age_high = 24;
  } else {
    int do_age_priority;
    Params::get_param_from_string("vaccine_prioritize_by_age", &do_age_priority);
    if(do_age_priority) {
      cout << "Vaccination Priority by Age\n";
      this->current_policy = VACC_AGE_PRIORITY;
      Params::get_param_from_string("vaccine_priority_age_low", &this->vaccine_priority_age_low);
      Params::get_param_from_string("vaccine_priority_age_high", &this->vaccine_priority_age_high);
      cout << "      Between Ages " << this->vaccine_priority_age_low << " and "
           << this->vaccine_priority_age_high << "\n";
    } else {
      this->vaccine_priority_age_low = 0;
      this->vaccine_priority_age_high = Demographics::MAX_AGE;
    }
  }

  // should we vaccinate anyone outside of the priority class
  int vacc_pri_only;
  this->vaccine_priority_only = false;
  Params::get_param_from_string("vaccine_priority_only", &vacc_pri_only);
  if(vacc_pri_only) {
    this->vaccine_priority_only = true;
    cout << "      Vaccinating only the priority groups\n";
  }

  // should we exclude people that have had symptomatic infections?
  int vacc_sympt_exclude;
  this->vaccinate_symptomatics = false;
  Params::get_param_from_string("vaccinate_symptomatics", &vacc_sympt_exclude);
  if(vacc_sympt_exclude) {
    this->vaccinate_symptomatics = true;
    cout << "      Vaccinating symptomatics\n";
  }

  // get vaccine_dose_priority
  Params::get_param_from_string("vaccine_dose_priority", &this->vaccine_dose_priority);
  assert(this->vaccine_dose_priority < 4);
  //get_param((char*)"vaccination_capacity",&vaccination_capacity);
  this->vaccination_capacity_map = new Timestep_Map("vaccination_capacity");
  this->vaccination_capacity_map->read_map();
  if(Global::Verbose > 1) {
    this->vaccination_capacity_map->print();
  }

  int refresh;
  Params::get_param_from_string("refresh_vaccine_queues_daily", &refresh);
  this->refresh_vaccine_queues_daily = (refresh > 0);

  // Need to fill the Vaccine_Manager Policies
  this->policies.push_back(new Vaccine_Priority_Policy_No_Priority(this));
  this->policies.push_back(new Vaccine_Priority_Policy_Specific_Age(this));
  this->policies.push_back(new Vaccine_Priority_Policy_ACIP(this));

}
;

Vaccine_Manager::~Vaccine_Manager() {
  if(this->vaccine_package != NULL) {
    delete this->vaccine_package;
  }
  if(this->vaccination_capacity_map != NULL) {
    delete this->vaccination_capacity_map;
  }
}

void Vaccine_Manager::fill_queues() {

  if(!this->do_vacc) {
    return;
  }
  // We need to loop over the entire population that the Manager oversees to put them in a queue.
  for(int ip = 0; ip < pop->get_index_size(); ip++) {
    Person * current_person = this->pop->get_person_by_index(ip);
    if (current_person != NULL) {
      if(this->policies[current_policy]->choose_first_positive(current_person, 0, 0) == true) {
	priority_queue.push_back(current_person);
      } else {
	if(this->vaccine_priority_only == false)
	  this->queue.push_back(current_person);
      }
    }
  }

  vector<Person *> random_queue(this->queue.size());
  copy(this->queue.begin(), this->queue.end(), random_queue.begin());
  FYShuffle<Person *>(random_queue);
  copy(random_queue.begin(), random_queue.end(), this->queue.begin());

  vector<Person *> random_priority_queue(this->priority_queue.size());
  copy(this->priority_queue.begin(), this->priority_queue.end(), random_priority_queue.begin());
  FYShuffle<Person *>(random_priority_queue);
  copy(random_priority_queue.begin(), random_priority_queue.end(),
       this->priority_queue.begin());

  if(Global::Verbose > 0) {
    cout << "Vaccine Queue Stats \n";
    cout << "   Number in Priority Queue      = " << this->priority_queue.size() << "\n";
    cout << "   Number in Regular Queue       = " << this->queue.size() << "\n";
    cout << "   Total Agents in Vaccine Queue = "
         << this->priority_queue.size() + this->queue.size() << "\n";
  }
}

void Vaccine_Manager::add_to_queue(Person* person) {
  if(this->policies[this->current_policy]->choose_first_positive(person, 0, 0) == true) {
    add_to_priority_queue_random(person);
  } else {
    if(this->vaccine_priority_only == false) {
      add_to_regular_queue_random(person);
    }
  }
}

void Vaccine_Manager::remove_from_queue(Person* person) {
  // remove the person from the queue if they are in there
  list<Person *>::iterator pq = find(this->priority_queue.begin(), this->priority_queue.end(),
				     person);
  if(pq != this->priority_queue.end()) {
    this->priority_queue.erase(pq);
    return;
  }
  pq = find(this->queue.begin(), this->queue.end(), person);
  if(pq != this->queue.end()) {
    this->queue.erase(pq);
  }
}

void Vaccine_Manager::add_to_priority_queue_random(Person* person) {
  // Find a position to put the person in
  int size = this->priority_queue.size();
  int position = (int)(Random::draw_random()*size);

  list<Person*>::iterator pq = this->priority_queue.begin();
  for(int i = 0; i < position; ++i) {
    ++pq;
  }
  this->priority_queue.insert(pq, person);
}

void Vaccine_Manager::add_to_regular_queue_random(Person* person) {
  // Find a position to put the person in
  int size = this->queue.size();
  int position = (int)(Random::draw_random() * size);

  list<Person*>::iterator pq = this->queue.begin();
  for(int i = 0; i < position; ++i) {
    ++pq;
  }
  this->queue.insert(pq, person);
}

void Vaccine_Manager::add_to_priority_queue_begin(Person* person) {
  this->priority_queue.push_front(person);
}

void Vaccine_Manager::add_to_priority_queue_end(Person* person) {
  this->priority_queue.push_back(person);
}

string Vaccine_Manager::get_vaccine_dose_priority_string() const {
  switch(this->vaccine_dose_priority) {
  case VACC_DOSE_NO_PRIORITY:
    return "No Priority";
  case VACC_DOSE_FIRST_PRIORITY:
    return "Priority, Place at Beginning of Queue";
  case VACC_DOSE_RAND_PRIORITY:
    return "Priority, Place with other Priority";
  case VACC_DOSE_LAST_PRIORITY:
    return "Priority, Place at End of Queue";
  default:
    FRED_WARNING("Unrecognized Vaccine Dose Priority\n");
    return "";
  }
  FRED_WARNING("Unrecognized Vaccine Dose Priority\n");
  return "";
}

void Vaccine_Manager::update(int day) {
  if(this->do_vacc) {
    this->vaccine_package->update(day);
    // Update the current vaccination capacity
    this->current_vaccine_capacity = this->vaccination_capacity_map->get_value_for_timestep(day,
											    Global::Vaccine_offset);
    cout << "Current Vaccine Stock = " << this->vaccine_package->get_vaccine(0)->get_current_stock()
	 << "\n";

    if(this->refresh_vaccine_queues_daily) {
      // update queues
      this->priority_queue.clear();
      this->queue.clear();
      fill_queues();
    }

    // vaccinate people in the queues:
    vaccinate(day);
  }
}

void Vaccine_Manager::reset() {
  this->priority_queue.clear();
  this->queue.clear();
  if(this->do_vacc) {
    fill_queues();
    this->vaccine_package->reset();
  }
}

void Vaccine_Manager::print() {
  this->vaccine_package->print();
}

void Vaccine_Manager::vaccinate(int day) {
  if(this->do_vacc) {
    cout << "Vaccinating!\n";
  } else {
    cout << "Not vaccinating!\n";
    return;
  }

  int number_vaccinated = 0;
  int n_p_vaccinated = 0;
  int n_r_vaccinated = 0;
  int accept_count = 0;
  int reject_count = 0;
  int reject_state_count = 0;

  // Figure out the total number of vaccines we can hand out today
  int total_vaccines_avail = this->vaccine_package->get_total_vaccines_avail_today();

  if(Global::Debug > 0) {
    cout << "Vaccine Capacity on Day " << day << " = " << current_vaccine_capacity << "\n";
    cout << "Queues at beginning of vaccination:  priority (" << priority_queue.size()
	 << ")    Regular (" << this->queue.size() << ")\n";
  }
  if(total_vaccines_avail == 0 || current_vaccine_capacity == 0) {
    if(Global::Debug > 1) {
      cout << "No Vaccine Available on Day " << day << "\n";
    }
    Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
    Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
    Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
    Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
    return;
  }

  // Start vaccinating Priority
  list<Person*>::iterator ip;
  ip = this->priority_queue.begin();
  //int accept_count = 0;
  //int reject_count = 0;
  //int reject_state_count = 0;
  // Run through the priority queue first 
  while(ip != this->priority_queue.end()) {
    Person* current_person = *ip;

    int vacc_app = this->vaccine_package->pick_from_applicable_vaccines((double)(current_person->get_age()));
    // printf("person = %d age = %.1f vacc_app = %d\n", current_person->get_id(), current_person->get_real_age(), vacc_app);
    if(vacc_app > -1) {
      bool accept_vaccine = false;
      // STB need to refactor to work with multiple diseases
      if((this->vaccinate_symptomatics == false)
	 && (current_person->get_health()->get_symptoms_start_date(0) != -1)
	 && (day >= current_person->get_health()->get_symptoms_start_date(0))) {
        accept_vaccine = false;
        reject_state_count++;
      } else {
        if(current_person->get_health()->is_vaccinated()) {
          accept_vaccine = current_person->acceptance_of_another_vaccine_dose();
        } else {
          accept_vaccine = current_person->acceptance_of_vaccine();
        }
      }
      if(accept_vaccine == true) {
        accept_count++;
        number_vaccinated++;
        this->current_vaccine_capacity--;
        n_p_vaccinated++;
        Vaccine* vacc = this->vaccine_package->get_vaccine(vacc_app);
        vacc->remove_stock(1);
        total_vaccines_avail--;
        current_person->take_vaccine(vacc, day, this);
        ip = this->priority_queue.erase(ip);  // remove a vaccinated person
      } else {
        reject_count++;
        // printf("vaccine rejected by person %d age %.1f\n", current_person->get_id(), current_person->get_real_age());
        // skip non-compliant person under HBM
        // if(strcmp(Global::Behavior_model_type,"HBM") == 0) ++ip;
        if(0) {
          ++ip;
        } else {
          // remove non-compliant person if not HBM
          ip = this->priority_queue.erase(ip);
        }
      }
    } else {
      if(Global::Verbose > 0) {
        cout << "Vaccine not applicable for agent " << current_person->get_id() << " "
	     << current_person->get_real_age() << "\n";
      }
      ++ip;
    }

    if(total_vaccines_avail == 0) {
      if(Global::Verbose > 0) {
        cout << "Vaccinated priority to stock out " << n_p_vaccinated << " agents, for a total of "
	     << number_vaccinated << " on day " << day << "\n";
        cout << "Left in queues:  Priority (" << priority_queue.size() << ")    Regular ("
	     << queue.size() << ")\n";
        cout << "Number of acceptances: " << accept_count << ", Number of rejections: "
	     << reject_count << "\n";
      }
      Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
      Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
      return;
    }
    if(current_vaccine_capacity == 0) {
      if(Global::Verbose > 0) {
        cout << "Vaccinated priority to capacity " << n_p_vaccinated << " agents, for a total of "
	     << number_vaccinated << " on day " << day << "\n";
        cout << "Left in queues:  Priority (" << this->priority_queue.size() << ")    Regular ("
	     << queue.size() << ")\n";
        cout << "Number of acceptances: " << accept_count << ", Number of rejections: "
	     << reject_count << "\n";
      }
      Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
      Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
      return;
    }
  }

  if(Global::Verbose > 0) {
    cout << "Vaccinated priority to population " << n_p_vaccinated << " agents, for a total of "
	 << number_vaccinated << " on day " << day << "\n";
  }

  // Run now through the regular queue
  ip = this->queue.begin();
  while(ip != this->queue.end()) {
    Person* current_person = *ip;
    int vacc_app = this->vaccine_package->pick_from_applicable_vaccines(current_person->get_real_age());
    if(vacc_app > -1) {
      bool accept_vaccine = true;
      if((this->vaccinate_symptomatics == true)
	 && (current_person->get_health()->get_symptoms_start_date(0) != -1)
	 && (day >= current_person->get_health()->get_symptoms_start_date(0))) {
        accept_vaccine = false;
        reject_state_count++;
        // printf("vaccine rejected by person %d age %0.1f -- ALREADY SYMPTOMATIC\n", current_person->get_id(), current_person->get_real_age());
      } else {
        if(current_person->get_health()->is_vaccinated()) {
          // printf("vaccine rejected by person %d age %0.1f -- ALREADY VACCINATED\n", current_person->get_id(), current_person->get_real_age());
          accept_vaccine = current_person->acceptance_of_another_vaccine_dose();
        } else {
          accept_vaccine = current_person->acceptance_of_vaccine();
        }
      }
      if(accept_vaccine == true) {
        // printf("vaccine accepted by person %d age %0.1f\n", current_person->get_id(), current_person->get_real_age());
        accept_count++;
        number_vaccinated++;
        this->current_vaccine_capacity--;
        n_r_vaccinated++;
        Vaccine* vacc = this->vaccine_package->get_vaccine(vacc_app);
        vacc->remove_stock(1);
        total_vaccines_avail--;
        current_person->take_vaccine(vacc, day, this);
        ip = this->queue.erase(ip);  // remove a vaccinated person
      } else {
        // printf("vaccine rejected by person %d age %0.1f\n", current_person->get_id(), current_person->get_real_age());
        reject_count++;
        // skip non-compliant person under HBM
        // if(strcmp(Global::Behavior_model_type,"HBM") == 0) ip++;
        if(0)
          ip++;
        // remove non-compliant person if not HBM
        else
          ip = this->queue.erase(ip);
      }
    } else {
      ip++;
    }
    if(total_vaccines_avail == 0) {
      if(Global::Verbose > 0) {
        cout << "Vaccinated regular to stock_out " << n_r_vaccinated << " agents, for a total of "
	     << number_vaccinated << " on day " << day << "\n";
        cout << "Left in queues:  priority (" << priority_queue.size() << ")    Regular ("
	     << queue.size() << ")\n";
        cout << "Number of acceptances: " << accept_count << ", Number of rejections: "
	     << reject_count << "\n";
      }
      Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
      Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
      return;
    }
    if(this->current_vaccine_capacity == 0) {
      if(Global::Verbose > 0) {
        cout << "Vaccinated regular to capacity " << n_r_vaccinated << " agents, for a total of "
	     << number_vaccinated << " on day " << day << "\n";
        cout << "Left in queues:  priority (" << this->priority_queue.size() << ")    Regular ("
	     << queue.size() << ")\n";
        cout << "Number of acceptances: " << accept_count << ", Number of rejections: "
	     << reject_count << "\n";
      }
      Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
      Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
      Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
      return;
    }
  }

  if(Global::Verbose > 0) {
    cout << "Vaccinated regular to population " << n_r_vaccinated << " agents, for a total of "
	 << number_vaccinated << " on day " << day << "\n";
    cout << "Left in queues:  priority (" << this->priority_queue.size() << ")    Regular ("
	 << queue.size() << ")\n";
    cout << "Number of acceptances: " << accept_count << ", Number of rejections: " << reject_count
	 << "\n";
  }
  Global::Daily_Tracker->set_index_key_pair(day,"V", number_vaccinated);
  Global::Daily_Tracker->set_index_key_pair(day,"Va", accept_count);
  Global::Daily_Tracker->set_index_key_pair(day,"Vr", reject_count);
  Global::Daily_Tracker->set_index_key_pair(day,"Vs", reject_state_count);
  return;
}
