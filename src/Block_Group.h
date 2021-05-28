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
// File: Block_Group.h
//


#ifndef _FRED_BLOCK_GROUP_H
#define _FRED_BLOCK_GROUP_H

#include <vector>
#include <unordered_map>
using namespace std;

#include "Admin_Division.h"

class Block_Group : public Admin_Division {
public:

  Block_Group(long long int _admin_code);

  ~Block_Group();

  int get_adi_national_rank() {
    return this->adi_national_rank;
  }

  int get_adi_state_rank() {
    return this->adi_state_rank;
  }

  static int get_number_of_block_groups() {
    return Block_Group::block_groups.size();
  }

  static Block_Group* get_block_group_with_index(int i) {
    return Block_Group::block_groups[i];
  }

  static Block_Group* get_block_group_with_admin_code(long long int block_group_code);

  static void read_adi_file();

private:

  int adi_national_rank;
  int adi_state_rank;

  // static variables
  static std::vector<Block_Group*> block_groups;
  static std::unordered_map<long long int,Block_Group*> lookup_map;
  static std::unordered_map<long long int,int> adi_national_rank_map;
  static std::unordered_map<long long int,int> adi_state_rank_map;
  static int Enable_ADI;

};

#endif // _FRED_BLOCK_GROUP_H
