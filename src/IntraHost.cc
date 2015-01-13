

#include "IntraHost.h"
#include "DefaultIntraHost.h"
#include "FixedIntraHost.h"
#include "Utils.h"
//#include "ODEIntraHost.h"

using namespace std;

void IntraHost :: setup(Disease *disease) {
  this->disease = disease;
  max_days = 0;
  }

IntraHost *IntraHost :: newIntraHost(int type) {
  switch(type) {
    case 0:
      return new DefaultIntraHost;

    case 1:
      return new FixedIntraHost;

      //    case 2:
      //      return new ODEIntraHost;

    default:
      Utils::fred_warning("Unknown IntraHost type (%d) supplied to IntraHost factory.  Using DefaultIntraHost.\n", type);
      return new DefaultIntraHost;
    }
  }

int IntraHost :: get_days_symp() {
  return 0;
  }
