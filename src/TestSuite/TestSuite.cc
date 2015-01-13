/*
 *  TestSuite.cc
 *  FRED
 *
 *  Created by Tom on 6/4/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include "TestSuite.h"

#include "../Household.h"
#include "../Neighborhood.h"
#include "../Community.h"
#include "../School.h"
#include "../Hospital.h"
#include "../Classroom.h"
#include "../Workplace.h"
#include "../Office.h"

using namespace std;

int main(int argc, char **argv) {
	InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}