#ifndef _FRED_EVOLUTION_H
#define _FRED_EVOLUTION_H

#include <map>

class Infection;
class Transmission;
class AV_Health;
class Antiviral;
class Infection;
class Health;
class Disease;
class Person;

class Evolution {
public:
  virtual void setup(Disease *disease);

  /**
   * Put this object back to its original state
   */
  virtual void reset(int reset);

  virtual double residual_immunity(Person *person, int challenge_strain, int day);

  /**
   * @param infection a pointer to an Infection object
   * @param loads a pointer to a map of viral loads
   */
  virtual Infection *transmit(Infection *infection, Transmission *transmission, Person *infectee);

  /**
   * @param av a pointer to an Antiviral object
   * @param health a pointer to a Health object
   * @param disease the disease to which this applies
   * @param cur_day the simulation day
   * @param av_health a pointer to a particular AV_Health object
   */
  virtual void avEffect(Antiviral *av, Health *health, int disease, int cur_day, AV_Health *av_health);

  /**
   * @param the simulation day
   * @return a pointer to a map of Primary Loads for a given day
   */
  virtual std::map<int, double> * get_primary_loads(int day);

  /**
   * @param the simulation day
   * @param the particular strain of the disease
   * @return a pointer to a map of Primary Loads for a particular strain of the disease on a given day
   */
  virtual std::map<int, double> * get_primary_loads(int day, int strain);

  /**
   * Print out information about this object
   */
  virtual void print();
  
  virtual void print_stats(int day) { }

  virtual double antigenic_distance(int strain1, int strain2) { 
    if(strain1 == strain2) return 0;
    else return 1;
  }

  virtual double antigenic_diversity(Person *p1, Person *p2);

  virtual void terminate_person(Person *p) {} 

protected:
  Disease *disease;
};

#endif


