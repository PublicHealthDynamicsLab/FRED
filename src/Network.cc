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
// #include "Disease.h"
// #include "Disease_List.h"

#include "Population.h" // for test only

//Private static variables that will be set by parameter lookups
double Network::contacts_per_day;
double** Network::prob_transmission_per_contact;

Network::Network(const char* lab) : Mixing_Group(lab) {
  int diseases = Global::Diseases.get_number_of_diseases();
  this->infectious_people = new std::vector<Person*>[diseases];

  this->new_infections = new int[diseases];
  this->current_infections = new int[diseases];
  this->total_infections = new int[diseases];
  this->new_symptomatic_infections = new int[diseases];
  this->current_symptomatic_infections = new int[diseases];
  this->total_symptomatic_infections = new int[diseases];
  this->current_infectious_agents = new int[diseases];
  this->current_symptomatic_agents = new int[diseases];

  // zero out all disease-specific counts
  for(int d = 0; d < diseases; ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;
    this->current_infectious_agents[d] = 0;
    this->current_symptomatic_agents[d] = 0;
    this->infectious_people[d].clear();
  }

  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->exposed_bitset.reset();
}

void Network::get_parameters() {

  Params::get_param_from_string("network_contacts", &Network::contacts_per_day);
  int n = Params::get_param_matrix((char *)"network_trans_per_contact", &Network::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nNetwork_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Network::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if(Network::prob_transmission_per_contact[i][j] > max_prob) {
	      max_prob = Network::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if(max_prob > 0) {
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      Network::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    Network::contacts_per_day *= max_prob;
  }

  if(Global::Verbose > 0) {
    printf("\nNetwork_contact_prob after normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++i) {
	      printf("%f ", Network::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", Network::contacts_per_day);
  }
  // end normalization
}

int Network::enroll(Person* per) {
  if(this->get_size() == this->enrollees.capacity()) {
    // double capacity if needed (to reduce future reallocations)
    this->enrollees.reserve(2 * this->get_size());
  }
  this->enrollees.push_back(per);
  FRED_VERBOSE(1,"Enroll person %d age %d in Network %s\n", per->get_id(), per->get_age(), this->label);
  return this->enrollees.size() - 1;
}

void Network::unenroll(int pos) {
  int size = this->enrollees.size();
  if(!(0 <= pos && pos < size)) {
    printf("Network %s pos = %d size = %d\n", this->label, pos, size);
  }
  assert(0 <= pos && pos < size);
  Person* removed = this->enrollees[pos];
  if(pos < size - 1) {
    Person* moved = this->enrollees[size - 1];
    FRED_VERBOSE(1,"UNENROLL Network %s pos = %d size = %d removed %d moved %d\n",
      this->label, pos, size, removed->get_id(), moved->get_id());
    this->enrollees[pos] = moved;
  } else {
    FRED_VERBOSE(1,"UNENROLL Network %s pos = %d size = %d removed %d moved NONE\n",
      this->label, pos, size, removed->get_id());
  }
  this->enrollees.pop_back();
  FRED_VERBOSE(1,"UNENROLL Network %s size = %d\n", this->label, this->enrollees.size());
}

void Network::print_infectious(int disease_id) {
  printf("INFECTIOUS in Network %s Disease %d: ", this->label, disease_id);
  int size = this->infectious_people[disease_id].size();
  for(int i = 0; i < size; ++i) {
    printf(" %d", this->infectious_people[disease_id][i]->get_id());
  }
  printf("\n");
}

double Network::get_transmission_prob(int disease_id, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease_id, i);
  int col = get_group(disease_id, s);
  double tr_pr = Network::prob_transmission_per_contact[row][col];
  return tr_pr;
}

double Network::get_contacts_per_day(int disease_id) {
  return Network::contacts_per_day;
}

void Network::print() {
  char filename[64];
  sprintf(filename, "%s/%s.txt", Global::Simulation_directory, get_label());
  FILE* link_fileptr = fopen(filename,"w");
  sprintf(filename, "%s/%s-people.txt", Global::Simulation_directory, get_label());
  FILE* people_fileptr = fopen(filename,"w");
  int size = this->get_size();
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    person->print_transmission_network(link_fileptr);
    person->print(people_fileptr,0);
  }
  fclose(link_fileptr);
  fclose(people_fileptr);
}

void Network::print_stdout() {
  int size = this->get_size();
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_enrollee(i);
    person->print_transmission_network(stdout);
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
    p->join_transmission_network();
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
  p1->join_transmission_network();

  Person* p2 = Global::Pop.select_random_person();
  printf("p2 %d\n", p2->get_id());
  p2->join_transmission_network();

  Person* p3 = Global::Pop.select_random_person();
  printf("p3 %d\n", p3->get_id());
  p3->join_transmission_network();

  printf("empty net:\n");
  print_stdout();

  printf ("p1->create_network_link_to(p2, this);\np2->create_network_link_to(p3, this);\n");
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
