/*
 *  MockPolicy.h
 *  FRED
 *
 *  Created by Tom on 6/25/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include "../Policy.h"

#include <gmock/gmock.h>

class MockPolicy : public Policy {
 public:
  MOCK_METHOD3(choose,
      int(Person* person, int strain, int current_day));
  MOCK_CONST_METHOD0(get_manager,
      Manager*());
  MOCK_CONST_METHOD0(print,
      void());
  MOCK_METHOD0(reset,
      void());
};