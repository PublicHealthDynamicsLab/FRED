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
// File: Travel.cc
//

#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Geo_Utils.h"
#include "Travel.h"
#include "Person.h"
#include "Place_List.h"
#include "Household.h"

#include <stdio.h>
#include <vector>
#include <set>

typedef set <Person*> pset;			// set of person ptrs
typedef set<Person*>::iterator piter;  // iterator to set of person ptrs
typedef pset * pset_ptr;			// pointer to above type
static pset * traveler_list;		   // list of travelers, per day
static pset_ptr * traveler_list_ptr;        // pointers to above lists
static double mean_trip_duration;		// mean days per trip
typedef vector <Person*> pvec;			// vector of person ptrs

// runtime parameters
static double * Travel_Duration_Cdf;		// cdf for trip duration
static int max_Travel_Duration = 0;		// number of days in cdf
Age_Map *travel_age_prob;

// travel hub record:
typedef struct hub_record {
  int id;
  double lat;
  double lon;
  pvec users;
  int pop;
  int pct;
} hub_t;

// list of trave hubs
vector < hub_t > hubs;
int num_hubs;

// a matrix containing the number of trips per day
// between hubs, read from an external file
int ** trips_per_day;

void Travel::setup(char * directory) {
  assert(Global::Enable_Travel);
  read_hub_file();
  read_trips_per_day_file();
  setup_travelers_per_hub();
  setup_travel_lists();
  travel_age_prob = new Age_Map("Travel Age Probability");
  travel_age_prob->read_from_input("travel_age_prob");
}

void Travel::read_hub_file() {
  char hub_file[FRED_STRING_SIZE];
  Params::get_param_from_string("travel_hub_file", hub_file);
  FILE *fp = Utils::fred_open_file(hub_file);
  if (fp == NULL) { Utils::fred_abort("Help! Can't open travel_hub_file %s\n", hub_file); }
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "reading travel_hub_file %s\n", hub_file);
    fflush(stdout);
  }
  hubs.clear();
  hub_t hub;
  hub.users.clear();
  while (fscanf(fp, "%d %lf %lf %d", &hub.id, &hub.lat, &hub.lon, &hub.pop) == 4) {
    hubs.push_back(hub);
  }
  fclose(fp);
  num_hubs = (int) hubs.size();
  printf("num_hubs = %d\n", num_hubs);
  trips_per_day = new int * [num_hubs];
  for (int i = 0; i < num_hubs; i++)
    trips_per_day[i] = new int [num_hubs];
}

void Travel::read_trips_per_day_file() {
  char trips_per_day_file[FRED_STRING_SIZE];
  Params::get_param_from_string("trips_per_day_file", trips_per_day_file);
  FILE *fp = Utils::fred_open_file(trips_per_day_file);
  if (fp == NULL) { Utils::fred_abort("Help! Can't open trips_per_day_file %s\n", trips_per_day_file); }
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "reading trips_per_day_file %s\n", trips_per_day_file);
    fflush(stdout);
  }
  for (int i = 0; i < num_hubs; i++) {
    for (int j = 0; j < num_hubs; j++) {
      int n;
      if (fscanf(fp, "%d ", &n) != 1) {
	Utils::fred_abort("ERROR: read failed on file %s", trips_per_day_file);
      }
      trips_per_day[i][j] = n;
    }
  }
  fclose(fp);
  if (Global::Verbose > 1) {
    for (int i = 0; i < num_hubs; i++) {
      for (int j = 0; j < num_hubs; j++) {
	printf("%d ", trips_per_day[i][j]);
      }
      printf("\n");
    }
    fflush(stdout);
    fprintf(Global::Statusfp, "finished reading trips_per_day_file %s\n", trips_per_day_file);
    }
}

void Travel::setup_travelers_per_hub() {
  int households = Global::Places.get_number_of_households();
  FRED_VERBOSE(0,"Preparing to set households: %li \n",households);
  for(int i = 0; i < households; i++) {
    Household * h = Global::Places.get_household_ptr(i);
    double h_lat = h->get_latitude();
    double h_lon = h->get_longitude();
    int census_tract_index = h->get_census_tract_index();
    long int h_id = Global::Places.get_census_tract_with_index(census_tract_index);
    int c = h->get_county();
    int h_county = Global::Places.get_county_with_index(c);
    FRED_VERBOSE(2,"h_id: %li h_county: %i \n", h_id, h_county);
    // find the travel hub closest to this household
    double max_distance = 166.0;		// travel at most 100 miles to nearest airport
    double min_dist = 100000000.0;
    int closest = -1;
    for(int j = 0; j < num_hubs; j++) {
       double dist = Geo_Utils::xy_distance(h_lat, h_lon, hubs[j].lat, hubs[j].lon);
	     /* if (dist < max_distance && dist < min_dist) {
	          closest = j;
	          min_dist = dist;
	     }*/
       //Assign travelers to hub based on 'county' rather than distance
      if(hubs[j].id == h_county){
	      closest = j;
	      min_dist = dist;
      }
    }
    if(closest > -1) {
      FRED_VERBOSE(1,"h_id: %li from county: %i  assigned to the airport: %i, distance:  %f\n",h_id,h_county,hubs[closest].id,min_dist); 
      // add everyone in the household to the user list for this hub
      int housemates = h->get_size();
      for(int k = 0; k < housemates; k++) {
	      Person * person = h->get_housemate(k);
	      hubs[closest].users.push_back(person);
      }
    }
  }

  // adjustment for partial user base
  for(int i = 0; i < num_hubs; i++) {
    hubs[i].pct = 0.5 + (100.0 * hubs[i].users.size()) / hubs[i].pop;
  }
  // print hubs
  for(int i = 0; i < num_hubs; i++) {
    printf("Hub %d: lat = %f lon = %f users = %d pop = %d pct = %d\n",
	   hubs[i].id, hubs[i].lat, hubs[i].lon, (int) hubs[i].users.size(),
	   hubs[i].pop, hubs[i].pct);
  }
  fflush(stdout);
}

void Travel::setup_travel_lists(){
  int n;
  Params::get_param_from_string("travel_duration",&n);
  Travel_Duration_Cdf = new double [n];
  max_Travel_Duration = Params::get_param_vector((char *) "travel_duration",
           Travel_Duration_Cdf) - 1;
 
  if(Global::Verbose > 0) {
    for (int i = 0; i <= max_Travel_Duration; i++) {
      fprintf(Global::Statusfp,"travel duration = %d %f ",i,Travel_Duration_Cdf[i]);
    }
    fprintf(Global::Statusfp,"\n");
  }
  mean_trip_duration = (max_Travel_Duration + 1) * 0.5;

  // set up empty vectors of people currently on trips
  // one vector for each day remaining
  traveler_list = new pset[max_Travel_Duration+1];
  for(int i = 0; i <= max_Travel_Duration; i++) {
    traveler_list[i].clear();
  }

  // setup pointers to the above lists, to be advanced each day
  // (so that lists are not copied)
  traveler_list_ptr = new pset_ptr[max_Travel_Duration+1];
  for(int i = 0; i <= max_Travel_Duration; i++) {
    traveler_list_ptr[i] = &traveler_list[i];
  }
}

void Travel::update_travel(int day) {

  if(!Global::Enable_Travel) {
    return;
  }

  if(Global::Verbose > 1) {
    fprintf(Global::Statusfp, "update_travel entered\n");
    fflush(Global::Statusfp);
  }

  // initiate new trips
  for(int i = 0; i < num_hubs; i++) {
    if(hubs[i].users.size() == 0) {
      continue;
    }
    for(int j = 0; j < num_hubs; j++) {
      if(hubs[j].users.size() == 0) {
	      continue;
      }
      int successful_trips = 0;
      int count = (trips_per_day[i][j] * hubs[i].pct + 0.5) / 100;
      for(int t = 0; t < count; t++) {
	      // select a potential traveler determined by travel_age_prob param
	      Person * visitor = NULL;
	      Person * host = NULL;
	      int attempts = 0;
	      while(visitor == NULL && attempts < 100) {
	        // select a random member of the source hub's user group
	        int v = IRAND(0,hubs[i].users.size()-1);
	        visitor = hubs[i].users[v];
	        double prob_travel_by_age = travel_age_prob->find_value(visitor->get_real_age());
	        if(prob_travel_by_age < RANDOM()) {
	          visitor = NULL;
	        }
	        attempts++;
	      }
	      if(visitor != NULL) {
	        // select a potential travel host determined by travel_age_prob param
	        attempts = 0;
	        while(host == NULL && attempts < 100) {
	          // select a random member of the destination hub's user group
	          int v = IRAND(0,hubs[j].users.size()-1);
	          host = hubs[j].users[v];
	          double prob_travel_by_age = travel_age_prob->find_value(host->get_real_age());
	          if(prob_travel_by_age < RANDOM()) {
	            host = NULL;
	          }
	          attempts++;
	        }
	      }
	      // travel occurs only of if both visitor and host are not already traveling
	      if(visitor != NULL && (!visitor->get_travel_status()) &&
	         host != NULL && (!host->get_travel_status())) {
	        // put traveler in travel status
	        visitor->start_traveling(host);
	        // put traveler on list for given number of days to travel
	        int duration = draw_from_distribution(max_Travel_Duration, Travel_Duration_Cdf);
	        traveler_list_ptr[duration]->insert(visitor);
	        successful_trips ++;
	        //TODO Decide when this should print
	        if(0) {
	          printf("DAY %d SRC = %d DEST = %d TRIP = %d TRAVELER = %d age = %d HOST = %d age = %d duration = %d\n",
		         day, hubs[i].id, hubs[j].id, t, visitor->get_id(), visitor->get_age(),
		        host->get_id(), host->get_age(), duration);
	          fflush(stdout);
	        }
	      }
      }
      FRED_VERBOSE(0,"DAY %d SRC = %d DEST = %d TRIPS = %d\n",
		   day, hubs[i].id, hubs[j].id, successful_trips);
    }
  }

  // process travelers who are returning home
  // printf("returning home:\n"); fflush(stdout);
  for(piter it=traveler_list_ptr[0]->begin(); it != traveler_list_ptr[0]->end(); it++) {
    (*it)->stop_traveling();
  }
  traveler_list_ptr[0]->clear();
  
  // update days still on travel (after today)
  pset * tmp = traveler_list_ptr[0];
  for(int i = 0; i < max_Travel_Duration; i++) {
    traveler_list_ptr[i] = traveler_list_ptr[i+1];
  }
  traveler_list_ptr[max_Travel_Duration] = tmp;
  
  if(Global::Verbose > 1) {
    fprintf(Global::Statusfp, "update_travel finished\n");
    fflush(Global::Statusfp);
  }

  return;
}

void Travel::terminate_person(Person *person) {
  for(unsigned int i = 0; i <= max_Travel_Duration; i++) {
    piter it = traveler_list[i].find(person);
    if(it != traveler_list[i].end()) {
      traveler_list[i].erase(it);
      Global::Pop.clear_mask_by_index( fred::Travel, person->get_pop_index() );
      break;
    }
  }
}

void Travel::quality_control(char * directory) {
}
