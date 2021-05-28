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
// File: Environmental_Transmission.h
//

#ifndef _FRED_ENVIRONMENTAL_TRANSMISSION_H
#define _FRED_ENVIRONMENTAL_TRANSMISSION_H

#include "Transmission.h"

class Condition;

class Environmental_Transmission: public Transmission {

public:
  Environmental_Transmission() {}
  ~Environmental_Transmission() {}
  void setup(Condition* condition);
  void transmission(int day, int hour, int condition_id, Group* group, int time_block) {}

private:
};

#endif // _FRED_ENVIRONMENTAL_TRANSMISSION_H
