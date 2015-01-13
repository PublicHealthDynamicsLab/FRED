#ifndef _FRED_MSEVOLUTION_H
#define _FRED_MSEVOLUTION_H

#include "MSEvolution.h"
#include "Random.h"
#include "Evolution.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Global.h"
#include "IntraHost.h"
#include "Antiviral.h"
#include "Health.h"
#include "Person.h"
#include "Piecewise_Linear.h"
#include "Past_Infection.h"
#include "Transmission.h"
#include "Strain.h"
#include "Params.h"
#include <map>
#include <cmath>
#include <fstream>
#include <cfloat>
#include "Utils.h"
#include "Population.h"
#include "Geo_Utils.h"
#include "Place.h"


class Infection;
class AV_Health;
class Antiviral;
class Infection;
class Health;
class Disease;
class Trajectory;
class Piecewise_Linear;

using namespace std;

class MSEvolution : public Evolution 
{
 public:
  MSEvolution();
  virtual ~MSEvolution();
  virtual void setup(Disease *disease);
  virtual Infection *transmit(Infection *infection, Transmission *transmission, Person *infectee);
  virtual double antigenic_distance(int strain1, int strain2);

 protected:
  virtual double residual_immunity(Person *person, int challenge_strain, int day);
  virtual double prob_inf_blocking(int old_strain, int new_strain, int time, int age);
  virtual double prob_vac_blocking(int old_strain, int new_strain, int time, int age);
  virtual double prob_blocking(int old_strain, int new_strain, int time, double halflife, double prob_blocking);
  virtual double prob_past_infections(Person *infectee, int new_strain, int day);
  virtual double prob_past_vaccinations(Person *infectee, int new_strain, int day);
  virtual double get_prob_taking(Person *infectee, int new_strain, double quantity, int day);
  virtual double prob_inoc(double quantity);
  ofstream file;

 private:
  Age_Map *halflife_inf, *halflife_vac;
  double init_prot_inf, init_prot_vac;
  double sat_quantity;
  Piecewise_Linear *protection;
};

#endif

