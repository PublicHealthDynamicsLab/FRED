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
// File: Transmission.h
//

#ifndef _FRED_TRANSMISSION_H
#define _FRED_TRANSMISSION_H

class Condition;
class Group;
class Person;
class Place;


class Transmission {

public:
  
  virtual ~Transmission() {}

  /**
   * This static factory method is used to get an instance of a
   * Transmission object of the specified subclass.
   *
   * @property a string containing the requested Transmission mode type
   * @return a pointer to a specific Transmission object
   */

  static Transmission* get_new_transmission(char* transmission_mode);
  virtual void setup(Condition* condition) = 0;
  virtual void transmission(int day, int hour, int condition_id, Group* group, int time_block) = 0;
  bool attempt_transmission(double transmission_prob, Person* source, Person* host,
			    int condition_id, int condition_to_transmit, int day, int hour, Group* group);

protected:

};


class Null_Transmission : public Transmission {
public:
  Null_Transmission() {};
  ~Null_Transmission() {};
  void setup(Condition* condition) {};
  void transmission(int day, int hour, int condition_id, Group* group, int time_block) {};
};


#endif // _FRED_TRANSMISSION_H
