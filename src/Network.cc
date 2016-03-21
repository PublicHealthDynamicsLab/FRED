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
// File: Network.cc
//

#include "Network.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"

#include "Population.h" // for test only

char Network::TYPE_NETWORK = 'n';
char Network::SUBTYPE_NONE = 'X';
char Network::SUBTYPE_TRANSMISSION = 't';
char Network::SUBTYPE_SEXUAL_PARTNER = 's';

Network::Network(const char* lab) : Mixing_Group(lab) {
  this->set_type(Network::TYPE_NETWORK);
  this->set_subtype(Network::SUBTYPE_NONE);
  this->contacts_per_day = 0.0;
  this->prob_transmission_per_contact = NULL;
}

void Network::get_parameters() {

  // set optional parameters
  Params::disable_abort_on_failure();

  Params::get_param_from_string("network_contacts", &(this->contacts_per_day));
  int n = Params::get_param_matrix((char*)"network_trans_per_contact", &(this->prob_transmission_per_contact));

  if (this->prob_transmission_per_contact != NULL) {

    if(Global::Verbose > 1) {
      printf("\nNetwork_contact_prob:\n");
      for(int i  = 0; i < n; ++i)  {
	for(int j  = 0; j < n; ++j) {
	  printf("%f ", this->prob_transmission_per_contact[i][j]);
	}
	printf("\n");
      }
    }
    
    // normalize contact parameters
    // find max contact prob
    double max_prob = 0.0;
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	if(this->prob_transmission_per_contact[i][j] > max_prob) {
	  max_prob = this->prob_transmission_per_contact[i][j];
	}
      }
    }

    // convert max contact prob to 1.0
    if(max_prob > 0) {
      for(int i  = 0; i < n; ++i)  {
	for(int j  = 0; j < n; ++j) {
	  this->prob_transmission_per_contact[i][j] /= max_prob;
	}
      }
      // compensate contact rate
      this->contacts_per_day *= max_prob;
    }
    
    if(Global::Verbose > 0) {
      printf("\nNetwork_contact_prob after normalization:\n");
      for(int i  = 0; i < n; ++i)  {
	for(int j  = 0; j < n; ++i) {
	  printf("%f ", this->prob_transmission_per_contact[i][j]);
	}
	printf("\n");
      }
      printf("\ncontact rate: %f\n", this->contacts_per_day);
    }
    // end normalization
  }

  // restore requiring parameters
  Params::set_abort_on_failure();
    
}

double Network::get_transmission_prob(int condition_id, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition_id, i);
  int col = get_group(condition_id, s);
  double tr_pr = this->prob_transmission_per_contact[row][col];
  return tr_pr;
}

/////////////////////////////////////////
//
// Network-SPECIFIC TRANSMISSION DATA
//
/////////////////////////////////////////

double Network::get_contacts_per_day(int condition_id) {
  return this->contacts_per_day;
}

double Network::get_contact_rate(int sim_day, int condition_id) {

  Condition* condition = Global::Conditions.get_condition(condition_id);
  // expected number of susceptible contacts for each infectious person
  double contacts = get_contacts_per_day(condition_id) * condition->get_transmissibility();

  return contacts;
}

int Network::get_contact_count(Person* infector, int condition_id, int sim_day, double contact_rate) {
  // reduce number of infective contacts by infector's infectivity
  double infectivity = infector->get_infectivity(condition_id, sim_day);
  double infector_contacts = contact_rate * infectivity;

  FRED_VERBOSE(1, "infectivity = %f, so ", infectivity);
  FRED_VERBOSE(1, "infector's effective contacts = %f\n", infector_contacts);

  // randomly round off the expected value of the contact counts
  int contact_count = static_cast<int>(infector_contacts);
  double r = Random::draw_random();
  if(r < infector_contacts - contact_count) {
    contact_count++;
  }

  FRED_VERBOSE(1, "infector contact_count = %d  r = %f\n", contact_count, r);

  return contact_count;
}

void Network::print() {
  int day = Global::Simulation_Day;
  char filename[64];
  sprintf(filename, "%s/%s-%d.txt", Global::Simulation_directory, get_label(), day);
  FILE* link_fileptr = fopen(filename,"w");
  sprintf(filename, "%s/%s-people-%d.txt", Global::Simulation_directory, get_label(), day);
  FILE* people_fileptr = fopen(filename,"w");
  int size = this->get_size();
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    person->print_network(link_fileptr, this);
    person->print(people_fileptr, 0);
  }
  fclose(link_fileptr);
  fclose(people_fileptr);

  if (strcmp(get_label(), "Transmission_Network") != 0) {
    return;
  }

  int a[20][20];
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 20; j++) {
      a[i][j] = 0;
    }
  }

  int total_out = 0;
  int max = -1;
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    int age_src = person->get_age();
    if (age_src > 99) {
      age_src = 99;
    }
    int out_degree = person->get_out_degree(this);
    for (int j = 0; j < out_degree; j++) {
      int age_dest = person->get_end_of_link(j,this)->get_age();
      if (age_dest > 99) {
	age_dest = 99;
      }
      a[age_src/5][age_dest/5]++;
      if (max < a[age_src/5][age_dest/5]) {
	max = a[age_src/5][age_dest/5];
      }
    }
    total_out += out_degree;
  }
  
  sprintf(filename, "%s/source.dat", Global::Simulation_directory);
  FILE* fileptr = fopen(filename,"w");
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 20; j++) {
      double x = max? (255.0 * (double) a[i][j] / (double) max) : 0.0;
      fprintf(fileptr, "%d %d %3.0f\n", i, j, x);
    }
    fprintf(fileptr, "\n");
  }
  fclose(fileptr);

}

void Network::print_stdout() {
  int size = this->get_size();
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    person->print_network(stdout, this);
  }
  printf("mean degree = %0.2f\n", get_mean_degree());
  printf("=======================\n\n");
  fflush(stdout);
}

bool Network::is_connected_to(Person* p1, Person* p2) {
  return p1->is_connected_to(p2, this);
}

bool Network::is_connected_from(Person* p1, Person* p2) {
  return p1->is_connected_from(p2, this);
}

double Network::get_mean_degree() {
  int size = get_size();
  int total_out = 0;
  for(int i = 0; i < size; i++) {
    Person* person = get_enrollee(i);
    int out_degree = person->get_out_degree(this);
    total_out += out_degree;
  }
  double mean = 0.0;
  if(size != 0) {
    mean = static_cast<double>(total_out) / static_cast<double>(size);
  }
  return mean;
}

void Network::test() {
  return;

  for(int i = 0; i < 50; ++i) {
    Person* p = Global::Pop.select_random_person();
    p->join_network(Global::Transmission_Network);
  }

  printf("create_random_network(1.0)\n");
  create_random_network(1.0);
  print_stdout();
  
  printf("create_random_network(4.0)\n");
  create_random_network(4.0);
  print_stdout();
  exit(0);

  Person* p1 = Global::Pop.select_random_person();
  printf("p1 %d\n", p1->get_id());
  p1->join_network(Global::Transmission_Network);

  Person* p2 = Global::Pop.select_random_person();
  printf("p2 %d\n", p2->get_id());
  p1->join_network(Global::Transmission_Network);

  Person* p3 = Global::Pop.select_random_person();
  printf("p3 %d\n", p3->get_id());
  p1->join_network(Global::Transmission_Network);

  printf("empty net:\n");
  print_stdout();

  printf("p1->create_network_link_to(p2, this);\np2->create_network_link_to(p3, this);\n");
  p1->create_network_link_to(p2, this);
  p2->create_network_link_to(p3, this);
  print_stdout();

  printf("p1->create_network_link_from(p3, this);\n");
  p1->create_network_link_from(p3, this);
  print_stdout();

  printf("p2->destroy_network_link_from(p1, this);\n");
  p2->destroy_network_link_from(p1, this);
  print_stdout();

  printf("p2->create_network_link_to(p1, this);\n");
  p2->create_network_link_to(p1, this);
  print_stdout();
}

void Network::create_random_network(double mean_degree) {
  int size = this->get_size();
  if(size < 2) {
    return;
  }
  for(int i = 0; i < size; ++i) {
    Person* person = get_enrollee(i);
    person->clear_network(this);
  }
  // print_stdout();
  int number_edges = mean_degree * size + 0.5;
  printf("size = %d  edges = %d\n\n", size, number_edges);
  int i = 0;
  while(i < number_edges) {
    int pos1 = Random::draw_random_int(0, size - 1);
    Person* src = this->get_enrollee(pos1);
    int pos2 = pos1;
    while(pos2 == pos1) {
      pos2 = Random::draw_random_int(0, size - 1);
    }
    Person* dest = this->get_enrollee(pos2);
    // printf("edge from %d to %d\n", src->get_id(), dest->get_id());
    if(src->is_connected_to(dest, this) == false) {
      src->create_network_link_to(dest, this);
      ++i;
    }
  }
}
