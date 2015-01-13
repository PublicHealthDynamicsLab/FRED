/*
 *  MockDecision.h
 *  FRED
 *
 *  Created by Tom on 8/2/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Decision.h"

class Person;

class MockDecision : public Decision {
 public:
  MOCK_CONST_METHOD0(get_name,
      string());
  MOCK_CONST_METHOD0(get_type,
      string());
  MOCK_METHOD3(evaluate,
      int(Person* person, int strain, int current_day));
};
