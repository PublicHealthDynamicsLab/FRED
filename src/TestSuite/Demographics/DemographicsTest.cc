/*
 *  DemographicsTest.cc
 *  FRED
 *
 *  Created by Tom on 6/23/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include "../TestSuite.h"

#include "../../Demographics.h"

#define	TEST_AGE			5
#define TEST_PROFESSION		3
#define TEST_SEX			'F'
#define TEST_OCCUPATION		'S'
#define TEST_MARITAL_STATUS	'S'

class DemographicsTest : public ::testing::Test {
protected:
  MockAge_Map pregnancy_map;
  MockPopulation population;
  MockPerson person;
	Demographics *testee;
  
  void SetUp() {
    // setup person
    EXPECT_CALL(person, get_population())
      .WillRepeatedly(Return(&population));
                      
    // setup population
    EXPECT_CALL(population, get_pregnancy_prob())
      .WillRepeatedly(Return(&pregnancy_map));
    
    // setup pregnancy map
    {
      EXPECT_CALL(pregnancy_map, get_num_ages())
        .WillRepeatedly(Return(2));
    
      EXPECT_CALL(pregnancy_map, find_value(Lt(COMING_OF_AGE + MANY)))
        .WillRepeatedly(Return(YES));
        
      EXPECT_CALL(pregnancy_map, find_value(Ge(COMING_OF_AGE + MANY)))
        .WillRepeatedly(Return(NIL));
    }
  }
};

TEST_F(DemographicsTest, Update) {
	// does nothing
}

TEST_F(DemographicsTest, GetAge) {
	for (int age = 1; age < 100; age += 10) {
		testee = new Demographics(&person, age, TEST_SEX, TEST_MARITAL_STATUS, TEST_PROFESSION);
		
		ASSERT_EQ(age, testee->get_age(EPOCH));
		ASSERT_EQ(age, testee->get_age());
		
		delete testee;
	}
}

TEST_F(DemographicsTest, IsPregnant) {
	for (int age = 1; age < 100; age += 10) {
		testee = new Demographics(&person, age, TEST_SEX, TEST_MARITAL_STATUS, TEST_PROFESSION);
		
		ASSERT_EQ(age < COMING_OF_AGE + MANY ? true : false, testee->is_pregnant());
		
		delete testee;
	}
}

TEST_F(DemographicsTest, GetSex) {
	for (int sex = 0; sex < MANY; sex++) {
		testee = new Demographics(&person, TEST_AGE, sex % 2 ? 'M' : 'F', TEST_MARITAL_STATUS, TEST_PROFESSION);
		
		ASSERT_EQ(sex % 2 ? 'M' : 'F', testee->get_sex());
		
		delete testee;
	}
}

/*
TEST_F(DemographicsTest, SetOccupation) {
	for (int age = 1; age < 100; age++) {
		testee = new Demographics(&person, age, TEST_SEX, TEST_MARITAL_STATUS, TEST_PROFESSION);
		
		// testee
		testee->set_occupation();
		
		if (age < 5) ASSERT_EQ('C', testee->get_occupation());
		else if (age < 19) ASSERT_EQ('S', testee->get_occupation());
		else if (age < 65) ASSERT_EQ('W', testee->get_occupation());
		else ASSERT_EQ('R', testee->get_occupation());

		delete testee;
	}
}

TEST_F(DemographicsTest, GetOccupation) {
	testee = new Demographics(&person, TEST_AGE, TEST_SEX, TEST_MARITAL_STATUS, TEST_PROFESSION);
	
	ASSERT_EQ(TEST_OCCUPATION, testee->get_occupation());
	
	delete testee;
}
*/

TEST_F(DemographicsTest, GetMaritalStatus) {
	for (int status = 0; status < MANY; status++) {
		testee = new Demographics(&person, TEST_AGE, TEST_SEX, status % 2 ? 'M' : 'S', TEST_PROFESSION);
		
		ASSERT_EQ(status % 2 ? 'M' : 'S', testee->get_marital_status());
		
		delete testee;
	}
}

TEST_F(DemographicsTest, GetProfession) {
	for (int profession = 0; profession < MANY; profession++) {
		testee = new Demographics(&person, TEST_AGE, TEST_SEX, TEST_MARITAL_STATUS,  profession % 5);
		
		ASSERT_EQ(profession % 5, testee->get_profession());
		
		delete testee;
	}
}
