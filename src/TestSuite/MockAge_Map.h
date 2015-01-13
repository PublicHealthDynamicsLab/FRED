/*
 *  MockAge_Map.h
 *  FRED
 *
 *  Created by Tom on 7/17/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Age_Map.h"

class MockAge_Map : public Age_Map {
public:
  MOCK_CONST_METHOD0(get_num_ages, int());
  MOCK_CONST_METHOD0(is_empty, bool());
  MOCK_METHOD1(read_from_input, void(string));
  MOCK_METHOD3(add_value, void(int, int, double));
  MOCK_CONST_METHOD1(find_value, double(int));
  MOCK_CONST_METHOD0(print, void());
  MOCK_CONST_METHOD0(quality_control, bool());
};
