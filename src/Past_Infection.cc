#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>

#include "Past_Infection.h"
#include "Disease.h"
#include "Strain.h"
#include "Person.h"

using namespace std;

Past_Infection :: Past_Infection() {
  disease = NULL;
}

Past_Infection :: Past_Infection(Infection *infection) {
  infection->get_strains(strains);
  assert(strains.size() > 0);
  recovery_date = infection->get_recovery_date();
  age_at_exposure = infection->get_age_at_exposure();
  disease = infection->get_disease();
  host = infection->get_host();
  if ( Global::Report_Immunity ) {
    report();
  }
}

Past_Infection :: Past_Infection(vector<int> &_strains, int _recovery_date, int _age_at_exposure, Disease *_disease, Person *_host) {
  strains = _strains;
  recovery_date = _recovery_date;
  age_at_exposure = _age_at_exposure;
  disease = _disease;
  host = _host;
  report();
}

void Past_Infection :: get_strains(vector<int> &strains) {
  strains = this->strains;
}

void Past_Infection :: report () {
  fprintf(Global::Immunityfp, "%d %d %d %d %zu ", host->get_id(), disease->get_id(), recovery_date, age_at_exposure, strains.size());
  for(unsigned int i=0; i<strains.size(); i++){
    fprintf(Global::Immunityfp, "%d ", strains[i]);
  }
  fprintf(Global::Immunityfp, "\n");
}

string Past_Infection :: format_header() {
  return "# person_id disease_id recovery_date age_at_exposure num_strains [strains]+ \n";
}
