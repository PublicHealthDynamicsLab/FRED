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
// File: Visualization_Layer.h
//

#ifndef _FRED_VISUALIZATION_GRID_H
#define _FRED_VISUALIZATION_GRID_H

#include "Abstract_Grid.h"

typedef std::pair<double, double> point;

class Visualization_Patch;

class Visualization_Layer : public Abstract_Grid {
public:
  Visualization_Layer();
  ~Visualization_Layer() {}
  Visualization_Patch* get_patch(int row, int col);
  Visualization_Patch* get_patch(double x, double y);
  void update_data(double x, double y, int count, int popsize);
  int get_period() {
    return this->period;
  }

protected:
  int period;
  Visualization_Patch** grid;            // Rectangular array of patches
};

#endif // _FRED_VISUALIZATION_GRID_H
