/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_EVOLUTION_H
#define _FRED_EVOLUTION_H

class Disease;
class Person;
class Regional_Layer;


class Evolution {
public:
  virtual void setup( Disease * disease );
  virtual double residual_immunity(Person *person, int challenge_strain, int day);
  virtual void print();
  virtual void update( int day ) { }
  virtual double antigenic_distance(int strain1, int strain2) { 
    if(strain1 == strain2) return 0;
    else return 1;
  }
  virtual double antigenic_diversity(Person *p1, Person *p2);
  virtual void terminate_person(Person *p) {} 
  virtual void initialize_reporting_grid( Regional_Layer * grid ) { };
  virtual void init_prior_immunity( Disease * disease ) { };
  Disease * get_disease() { return disease; }


protected:
  Disease *disease;
};

#endif


