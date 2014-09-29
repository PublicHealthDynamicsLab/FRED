/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Fred.h
//

#ifndef _FRED_H

int main(int argc, char* argv[]);
void fred_setup(int argc, char* argv[]);
void fred_step(int day);
void fred_finish();

#define _FRED_H

#endif // _FRED_H
