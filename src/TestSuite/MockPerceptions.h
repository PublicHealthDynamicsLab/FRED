/*
 *  MockPerceptions.h
 *  FRED
 *
 *  Created by Tom on 6/2/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Perceptions.h"

class MockPerceptions : public Perceptions {
public:
	MockPerceptions() { }
	
	MOCK_METHOD0(reset, void());
	MOCK_METHOD1(update, void(int));
	MOCK_METHOD0(will_keep_kids_home, int());
};