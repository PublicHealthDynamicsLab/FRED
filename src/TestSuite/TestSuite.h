/*
 *  TestSuite.h
 *  FRED
 *
 *  Created by Tom on 6/3/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include <stdexcept>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MockAge_Map.h"
#include "MockBehavior.h"
#include "MockPerson.h"
#include "MockPopulation.h"
#include "MockPlace.h"
#include "MockPerceptions.h"
#include "MockStrain.h"
#include "MockTimestep_Map.h"
#include "MockInfection.h"
#include "MockAntiviral.h"
#include "MockAV_Health.h"
#include "MockAV_Manager.h"
#include "MockVaccine.h"
#include "MockVaccine_Dose.h"
#include "MockPolicy.h"
#include "MockHealth.h"
#include "MockDecision.h"

#define YES								1
#define NO								0
#define NONE							0
#define NIL								0.0

#define COMING_OF_AGE					18

#define	EPOCH							0

#define	SUNDAY							0
#define	MONDAY							1
#define	TUESDAY							2
#define	WEDNESDAY						3
#define	THURSDAY						4
#define	FRIDAY							5
#define	SATURDAY						6

#define INFECTIOUS_SYMPTOMATIC			'I'
#define INFECTIOUS_ASYMPTOMATIC			'i'

#define	MANY							30	// must be >= 20
#define FEW								5	

#define PLACE_COUNT						6

#define HOUSEHOLD_INDEX					0
#define NEIGHBORHOOD_INDEX				1
#define SCHOOL_INDEX					2
#define CLASSROOM_INDEX					3
#define WORKPLACE_INDEX					4
#define OFFICE_INDEX					5

#define DEFAULT_GROUP_COUNT				1

#define DEFAULT_GROUP					0

#define SCHOOL_GROUP_COUNT				4
#define CLASSROOM_GROUP_COUNT			4

#define ELEMENTARY_SCHOOL				0
#define MIDDLE_SCHOOL					1
#define HIGH_SCHOOL						2
#define TEACHER							3

#define BASIC_AGE_GROUP_COUNT			2

#define MINOR							0
#define ADULT							1

#define STRAIN_SUSCEPTIBLE				'S'
#define STRAIN_EXPOSED					'E'
#define STRAIN_ASYMPTOMATIC				'i'
#define STRAIN_SYMPTOMATIC				'I'
#define STRAIN_RECOVERED				'R'
#define STRAIN_IMMUNE					'M'

#define DEFAULT_SUSCEPTIBILITY_MULTP	1.0

#define TEST_ASYMPTOMATIC_INFECTIVITY	1.0
#define TEST_SYMPTOMATIC_INFECTIVITY	1.0
#define TEST_INFECTIVITY				4.0
#define TEST_SUSCEPTIBILITY				5.0
#define TEST_SUSCEPTIBILITY_MULTIPLIER	1.3
#define TEST_PERIOD_MULTIPLIER			1.5

#define TEST_CONTACTS_PER_DAY			5
#define TEST_BASE_CONTACT_PROB			0.01

#define TEST_STRAIN_ANY_STATUS			'*'
#define TEST_STRAIN_COUNT				7
#define TEST_STRAIN						0
#define TEST_STRAIN_ID					0xF100

#define TEST_AV_COURSE_LENGTH			5

#define TEST_VACCINE_DOSE_COUNT			5
#define TEST_VACCINE_EFFICACY_DELAY		5

#define TEST_EXPOSURE_DAY				MONDAY

#define TEST_DAYS_ASYMPTOMATIC			5
#define TEST_DAYS_SYMPTOMATIC			5
#define TEST_DAYS_LATENT				5
#define TEST_DAYS_RECOVERY				5

#define TEST_RUN						0
#define	TEST_PROFILE					0

#define TEST_DAY						MONDAY
#define TEST_WEEK_DAY					TUESDAY
#define TEST_WEEKEND_DAY				SUNDAY

#define MONTH							30

#define TEST_PERSON_ID					0xB0B

#define TEST_ADULT_AGE					20
#define TEST_MINOR_AGE					10

#define TEST_PLACE_ID					0x41AC
#define TEST_PLACE_LONGITUDE			0xEEEE
#define TEST_PLACE_LATITUDE				0xFFFF
#define TEST_PLACE_CLOSE_DATE			10
#define TEST_PLACE_OPEN_DATE			20
#define TEST_PLACE_TYPE					'T'
#define TEST_CONTAINER_ID				0xC047

#define	TEST_HOUSEHOLD_ID				0xBEEF
#define	TEST_WORKPLACE_ID				0xDEAD
#define	TEST_OFFICE_ID					0xDEED
#define TEST_SCHOOL_ID					0xAAAA
#define	TEST_CLASSROOM_ID				0xD00D
#define	TEST_NEIGHBORHOOD_ID			0x400D

typedef int								DayOfWeek;

using namespace ::testing;

#endif
