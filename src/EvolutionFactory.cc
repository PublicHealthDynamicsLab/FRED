
#include "EvolutionFactory.h"
#include "Evolution.h"
#include "MSEvolution.h"
#include "FergEvolution.h"
//#include "BFEvolution.h"
#include "Global.h"
#include "Utils.h"

Evolution *EvolutionFactory :: newEvolution(int type) {
  switch(type) {
    case 0:
      return new Evolution;
    case 1:
      return new MSEvolution;
    case 2:
      return new FergEvolution;
    //case 3:
    //  return new BFEvolution;
    default:
      Utils::fred_warning("Unknown Evolution type (%d) supplied to EvolutionFactory.  Using the default.", type);
      return new Evolution;
    }
  }
