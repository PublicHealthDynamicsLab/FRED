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
// File: Block_Group.cc
//

#include "Block_Group.h"
#include "Census_Tract.h"
#include "Property.h"
#include "Utils.h"

Block_Group::~Block_Group() {
}

Block_Group::Block_Group(long long int admin_code) : Admin_Division(admin_code) {

  // get the census tract associated with this code, creating a new one if necessary
  long long int census_tract_admin_code = get_admin_division_code() / 10;
  Census_Tract* census_tract = Census_Tract::get_census_tract_with_admin_code(census_tract_admin_code);
  this->higher = census_tract;
  census_tract->add_subdivision(this);
  
  this->adi_national_rank = 0;
  this->adi_state_rank = 0;
  if (Block_Group::Enable_ADI) {
    this->adi_national_rank = adi_national_rank_map[admin_code];
    this->adi_state_rank = adi_state_rank_map[admin_code];
  }

}

//////////////////////////////
//
// STATIC METHODS
//
//////////////////////////////


// static variables
std::vector<Block_Group*> Block_Group::block_groups;
std::unordered_map<long long int,Block_Group*> Block_Group::lookup_map;
std::unordered_map<long long int,int> Block_Group::adi_national_rank_map;
std::unordered_map<long long int,int> Block_Group::adi_state_rank_map;
int Block_Group::Enable_ADI = 0;

Block_Group* Block_Group::get_block_group_with_admin_code(long long int block_group_admin_code) {
  Block_Group* block_group = NULL;
  std::unordered_map<long long int,Block_Group*>::iterator itr;
  itr = Block_Group::lookup_map.find(block_group_admin_code);
  if (itr == Block_Group::lookup_map.end()) {
    // this is a new block group
    block_group = new Block_Group(block_group_admin_code);
    Block_Group::block_groups.push_back(block_group);
    Block_Group::lookup_map[block_group_admin_code] = block_group;
  }
  else {
    block_group = itr->second;
  }
  return block_group;
}

void Block_Group::read_adi_file() {
  char adi_file [FRED_STRING_SIZE];
  
  FRED_VERBOSE(0, "read_adi_file entered\n");
  Property::get_property("enable_adi_rank", &Block_Group::Enable_ADI);
  Property::get_property("adi_file", adi_file);

  if (Block_Group::Enable_ADI) {
    char line [FRED_STRING_SIZE];
    long long int gis;
    int st;
    long long int fips;
    int state_rank;
    int national_rank;
  
    FRED_VERBOSE(0, "read_adi_file %s\n", adi_file);
    FILE* fp = Utils::fred_open_file(adi_file);

    // skip header line
    fgets(line, FRED_STRING_SIZE, fp);

    // read first data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);

    int items = sscanf(line, "G%lld,%d,%lld,%d,%d", &gis, &st, &fips, &state_rank, &national_rank);
    // printf("items = %d  G%lld,%d,%lld,%d,%d\n", items, gis, st, fips, state_rank, national_rank);fflush(stdout);
    while (items == 5) {
      adi_national_rank_map[fips] = national_rank;
      adi_state_rank_map[fips] = state_rank;
      // get next line
      strcpy(line, "");
      fgets(line, FRED_STRING_SIZE, fp);
      items = sscanf(line, "G%lld,%d,%lld,%d,%d", &gis, &st, &fips, &state_rank, &national_rank);
      // printf("items = %d  G%lld,%d,%lld,%d,%d\n", items, gis, st, fips, state_rank, national_rank);fflush(stdout);
    }
    fclose(fp);
  }
  FRED_VERBOSE(0, "read_adi_file finished\n");
}

