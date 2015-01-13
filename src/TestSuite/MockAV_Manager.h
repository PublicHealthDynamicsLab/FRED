/*
 *  MockAV_Health.h
 *  FRED
 *
 *  Created by Tom on 7/26/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../AV_Manager.h"

class MockAV_Manager : public AV_Manager {
public:
  MOCK_CONST_METHOD0(do_antivirals,
      bool());
  MOCK_CONST_METHOD0(get_overall_start_day,
      int());
  MOCK_CONST_METHOD0(get_current_av,
      Antiviral*());
  MOCK_CONST_METHOD0(get_antivirals,
      Antivirals*());
  MOCK_CONST_METHOD0(get_num_antivirals,
      int());
  MOCK_METHOD1(disseminate,
      void(int day));
  MOCK_METHOD1(update,
      void(int day));
  MOCK_METHOD0(reset,
      void());
  MOCK_METHOD0(print,
      void());
};