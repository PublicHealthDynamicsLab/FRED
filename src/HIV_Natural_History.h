/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_HIV_Natural_History_H
#define _FRED_HIV_Natural_History_H

#include "Natural_History.h"

class HIV_Natural_History : public Natural_History {

public:
  HIV_Natural_History();
  ~HIV_Natural_History();

  void setup(Disease *disease);
  void update_infection(int day, Person* host, Infection *infection);
  void read_hiv_files();
  
private:
};

#endif
