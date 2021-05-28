/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Network.cc
//

#include "Clause.h"
#include "Condition.h"
#include "Network.h"
#include "Network_Type.h"
#include "Person.h"
#include "Property.h"
#include "Random.h"

Network::Network(const char* lab, int _type_id, Network_Type* net_type) : Group(lab, _type_id) {
  this->network_type = net_type;
}

void Network::read_edges() {
  pair_vector_t results = Property::get_edges(this->get_label());
  for(int i = 0; i < results.size(); ++i) {
    int p1 = results[i].first;
    int p2 = results[i].second;
    printf("%s.add_edge %d %d\n", this->get_label(), p1, p2);
    Person* person1 = Person::get_person(p1);
    Person* person2 = Person::get_person(p2);
    if(p1 == p2) {
      person1->join_network(this);
    } else {
      person1->join_network(this);
      person2->join_network(this);
      person1->add_edge_to(person2, this);
      person2->add_edge_from(person1, this);
      if(this->is_undirected()) {
        person2->add_edge_to(person1, this);
        person1->add_edge_from(person2, this);
      }
    }
  }
}

void Network::get_properties() {

  // set optional properties
  Property::disable_abort_on_failure();

  // restore requiring properties
  Property::set_abort_on_failure();
}

void Network::print() {
  int day = Global::Simulation_Day;
  char filename[64];
  sprintf(filename, "%s/RUN%d/%s-%d.txt", Global::Simulation_directory, Global::Simulation_run_number, this->get_label(), day);
  FILE* fp = fopen(filename,"w");
  int size = this->get_size();
  // fprintf(fp, "size of %s = %d\n", get_label(), size);
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_member(i);
    int out_degree = person->get_out_degree(this);
    int in_degree = person->get_in_degree(this);
    if(in_degree == 0 && out_degree == 0) {
      fprintf(fp, "%s.add_edge = %d %d 0.0\n", this->get_label(), person->get_id(), person->get_id());
    } else {
      for(int j = 0; j < out_degree; ++j) {
        Person* person2 = person->get_outward_edge(j,this);
        fprintf(fp, "%s.add_edge = %d %d %f\n",
            this->get_label(), person->get_id(), person2->get_id(), person->get_weight_to(person2,this));
      }
    }
  }
  fclose(fp);

  sprintf(filename, "%s/RUN%d/%s-%d.vna", Global::Simulation_directory, Global::Simulation_run_number, this->get_label(), day);
  fp = fopen(filename,"w");
  fprintf(fp, "*node data\n");
  fprintf(fp, "ID age sex race\n");
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_member(i);
    fprintf(fp, "%d %d %c %d\n", person->get_id(), person->get_age(), person->get_sex(),person->get_race());
  }
  fprintf(fp, "*tie data\n");
  fprintf(fp, "from to weight\n");
  for(int i = 0; i < size; ++i) {
    Person* person = this->get_member(i);
    int out_degree = person->get_out_degree(this);
    for(int j = 0; j < out_degree; ++j) {
      Person* person2 = person->get_outward_edge(j,this);
      if(is_undirected()) {
        if(person->get_id() < person2->get_id()) {
          fprintf(fp, "%d %d %f\n", person->get_id(), person2->get_id(), person->get_weight_to(person2,this));
        }
      } else {
        fprintf(fp, "%d %d %f\n", person->get_id(), person2->get_id(), person->get_weight_to(person2,this));
      }
    }
  }
  fclose(fp);

  return;

  if(false) {
    int day = Global::Simulation_Day;
    char filename[64];
    sprintf(filename, "%s/RUN%d/%s-%d.txt", Global::Simulation_directory, Global::Simulation_run_number, this->get_label(), day);
    FILE* fp = fopen(filename,"w");
    int size = this->get_size();
    fprintf(fp, "size of %s = %d\n", this->get_label(), size);
    for(int i = 0; i < size; ++i) {
      Person* person = this->get_member(i);
      int out_degree = person->get_out_degree(this);
      int in_degree = person->get_in_degree(this);
      fprintf(fp, "\nNETWORK %s day %d member %d person %d in_deg %d out_deg %d:\n",
          this->get_label(), Global::Simulation_Day, i, person->get_id(), in_degree, out_degree);
      for(int j = 0; j < out_degree; ++j) {
        Person* person2 = person->get_outward_edge(j,this);
        fprintf(fp, "NETWORK %s day %d person %d age %d sex %c race %d ",
            this->get_label(), Global::Simulation_Day, person->get_id(), person->get_age(), person->get_sex(), person->get_race());
        fprintf(fp, "other %d age %d sex %c race %d\n",
            person2->get_id(), person2->get_age(), person2->get_sex(), person2->get_race());
      }
    }
    fclose(fp);
  }
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
  for(int i = 0; i < size; ++i) {
    Person* person = get_member(i);
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
}

bool Network::is_undirected() {
  return this->network_type->is_undirected();
}

Network* Network::get_network(string name) {
  Network_Type* network_type = Network_Type::get_network_type(name);
  if(network_type == NULL) {
    return NULL;
  } else {
    return network_type->get_network();
  }
}

int Network::get_time_block(int day, int hour) {
  return this->network_type->get_time_block(day, hour);
}


void Network::randomize(double mean_degree, int max_degree) {
  int size = this->get_size();
  if(size < 2) {
    return;
  }
  for(int i = 0; i < size; ++i) {
    Person* person = get_member(i);
    person->clear_network(this);
  }
  // print_stdout();
  int number_edges = mean_degree * size + 0.5;
  FRED_DEBUG(0, "RANDOMIZE size = %d  edges = %d\n\n", size, number_edges);
  int edges = 0;
  int found = 1;
  while(edges < number_edges && found==1) {
    found = 0;
    int pos1 = Random::draw_random_int(0, size - 1);
    Person* src = this->get_member(pos1);
    while(src->get_degree(this) >= max_degree) {
      FRED_DEBUG(0, "RANDOMIZE src degree = %d >= max_degree = %d\n", src->get_degree(this), max_degree);
      pos1 = Random::draw_random_int(0, size - 1);
      src = this->get_member(pos1);
    }
    FRED_DEBUG(0, "RANDOMIZE src person %d sex %c\n", src->get_id(), src->get_sex());

    // get a qualified destination

    // shuffle the order of candidates
    std::vector<int> shuffle_index;
    shuffle_index.clear();
    shuffle_index.reserve(size);
    for(int i = 0; i < size; ++i) {
      shuffle_index.push_back(i);
    }
    FYShuffle<int>(shuffle_index);

    int index = 0;
    while(index < size && found == 0) {
      int pos = shuffle_index[index];
      Person* dest = get_member(pos);
      if(dest == src) {
        ++index;
        continue;
      }
      if(dest->get_degree(this) >= max_degree) {
        ++index;
        continue;
      }
      if(src->is_connected_to(dest, this)) {
        ++index;
        continue;
      }

      // printf("RANDOMIZE ADD EDGE from person %d sex %c to person %d sex %c \n", src->get_id(), src->get_sex(), dest->get_id(), dest->get_sex());
      src->add_edge_to(dest, this);
      dest->add_edge_from(src, this);
      if(this->is_undirected()) {
        src->add_edge_from(dest, this);
        dest->add_edge_to(src, this);
      }
      ++edges;
      found = 1;
    }
  }
  printf("RANDOMIZE size = %d  found = %d edges = %d  mean_degree = %f\n\n", size, found, edges, (edges*1.0)/size);
}

const char* Network::get_name() {
  return this->network_type->get_name();
}

void Network::print_person(Person* person, FILE* fp) {
  int out_degree = person->get_out_degree(this);
  int in_degree = person->get_in_degree(this);
  fprintf(fp, "\nNETWORK %s day %d person %d in_deg %d out_deg %d:\n", this->get_label(), Global::Simulation_Day, person->get_id(), in_degree, out_degree);
  for(int j = 0; j < out_degree; ++j) {
    Person* person2 = person->get_outward_edge(j,this);
    fprintf(fp, "NETWORK %s day %d person %d age %d sex %c race %d ", this->get_label(),
        Global::Simulation_Day, person->get_id(), person->get_age(), person->get_sex(), person->get_race());
    fprintf(fp, "other %d age %d sex %c race %d\n",  person2->get_id(),
        person2->get_age(), person2->get_sex(), person2->get_race());
  }
}
