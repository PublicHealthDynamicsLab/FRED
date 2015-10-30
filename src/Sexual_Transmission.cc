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
// File: Sexual_Transmission.cc
//

#include <vector>
using namespace std;


#include "Disease.h"
#include "Epidemic.h"
#include "Network.h"
#include "Person.h"
#include "Random.h"
#include "Sexual_Transmission.h"
#include "Sexual_Transmission_Network.h"

void Sexual_Transmission::get_parameters() {
}

void Sexual_Transmission::setup(Disease* disease) {
}

void Sexual_Transmission::spread_infection(int day, int disease_id, Mixing_Group* mixing_group) {
  if(dynamic_cast<Sexual_Transmission_Network*>(mixing_group) == NULL) {
    //Sexual_Transmission must occur on a Sexual_Transmission_Network type
    return;
  } else {
    this->spread_infection(day, disease_id, dynamic_cast<Sexual_Transmission_Network*>(mixing_group));
  }
}

void Sexual_Transmission::spread_infection(int day, int disease_id, Sexual_Transmission_Network* sexual_trans_network) {
  
  int infectious_hosts = sexual_trans_network->get_number_of_infectious_people(disease_id);
  FRED_VERBOSE(0, "SEXUAL_TRANS: day %d infectious = %d\n", day, infectious_hosts);
  if (infectious_hosts == 0) {
    return;
  }

  // count all links from infectious hosts
  std::vector<int> link_sum;
  link_sum.clear();

  int total_number_of_links = 0;
  for (int i = 0; i < infectious_hosts; i++) {
    Person* infector = sexual_trans_network->get_infectious_person(disease_id, i);
    int out_degree = infector->get_out_degree(sexual_trans_network);
    total_number_of_links += out_degree;
    link_sum.push_back(total_number_of_links);
  }

  // max number of expected transmissions
  double trans_per_contact = sexual_trans_network->get_sexual_transmission_per_contact();
  double number_of_contacts_per_link = sexual_trans_network->get_sexual_contacts_per_day();
  int max_transmissions = round(total_number_of_links * number_of_contacts_per_link * trans_per_contact);

  FRED_VERBOSE(0, "SEXUAL_TRANS: day %d infectious = %d links = %d contacts = %f trans = %f max = %d\n",
	            day, infectious_hosts, total_number_of_links,
	            number_of_contacts_per_link, trans_per_contact, max_transmissions);

  // select this number of links (with replacement)
  for(int attempt = 0; attempt < max_transmissions; ++attempt) {

    FRED_VERBOSE(0, "SEXUAL_TRANS attempt %d starting\n", attempt);

    // select a link at random
    int selected = Random::draw_random_int(0, total_number_of_links-1);
    FRED_VERBOSE(0, "SEXUAL_TRANS total_links %d selected %d\n", total_number_of_links, selected);

    bool found = false;
    int i = 0;
    while (link_sum[i] < selected+1) {
      i++;
    }
    assert(i < infectious_hosts);
    Person* infector = sexual_trans_network->get_infectious_person(disease_id, i);
    FRED_VERBOSE(0, "SEXUAL_TRANS infector %d\n", infector->get_id());

    int infector_out_degree = link_sum[i] - (i==0?0:link_sum[i-1]);
    int out_degree = infector->get_out_degree(sexual_trans_network);
    int infector_link = infector_out_degree - (link_sum[i] - selected);

    FRED_VERBOSE(0, "SEXUAL_TRANS infector out_degree %d %d link %d\n", infector_out_degree, out_degree, infector_link);

    // find the destination of the selected link
    Person* infectee = infector->get_end_of_link(infector_link, sexual_trans_network);

    // if the dest is susceptible, infection occurs
    if(infectee->is_susceptible(disease_id)) {
      bool success = attempt_transmission(infector, infectee,
					  disease_id, day, sexual_trans_network);
    }
    FRED_VERBOSE(0, "SEXUAL_TRANS attempt %d complete\n", attempt);
  }
  FRED_VERBOSE(0, "SEXUAL_TRANS complete\n");
}

bool Sexual_Transmission::attempt_transmission(Person* infector, Person* infectee,
					       int disease_id, int day, Sexual_Transmission_Network* sexual_trans_network) {

  FRED_STATUS(0, "infector %d -- infectee %d is susceptible\n", infector->get_id(), infectee->get_id());

  double infectivity = infector->get_infectivity(disease_id, day);

  // reduce infectivity due to infector's hygiene (face masks or hand washing)
  // infectivity *= infector->get_transmission_modifier_due_to_hygiene(disease_id);

  double susceptibility = infectee->get_susceptibility(disease_id);

  // reduce susceptibility due to infectee's hygiene (face masks or hand washing)
  // susceptibility *= infectee->get_susceptibility_modifier_due_to_hygiene(disease_id);
    
  double infection_prob = infectivity * susceptibility;

  double r = Random::draw_random();
  if(r < infection_prob) {
    // successful transmission; create a new infection in infectee
    infector->infect(infectee, disease_id, NULL, day);

    FRED_VERBOSE(0, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob);

    // notify the epidemic
    Global::Diseases.get_disease(disease_id)->get_epidemic()->become_exposed(infectee, day);

    return true;
  } else {
    FRED_VERBOSE(0, "transmission failed: r = %f  prob = %f\n", r, infection_prob);
    return false;
  }
}
