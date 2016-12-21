/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_HIV_EPIDEMIC_H
#define _FRED_HIV_EPIDEMIC_H

#include "Epidemic.h"
class Condition;

class HIV_Epidemic : public Epidemic {

public:
  HIV_Epidemic(Condition * condition);

  ~HIV_Epidemic() {}

  void setup();

  void update(int day);

  void report_condition_specific_stats(int day);

  void end_of_run();

private:

};

#endif
