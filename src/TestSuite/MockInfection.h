#include <gmock/gmock.h>

#include "../Infection.h"

class MockInfection : public Infection {
public:
	MockInfection() { }
	
	MOCK_CONST_METHOD0(get_strain_status, char ());
	MOCK_CONST_METHOD0(get_strain, Strain *());
	MOCK_METHOD0(become_infectious, void());
	MOCK_METHOD0(become_symptomatic, void());
	MOCK_METHOD0(become_susceptible, void());
	MOCK_METHOD0(recover, void());
	MOCK_METHOD1(update, void(int today));
	MOCK_METHOD2(possibly_mutate, bool(Health *health, int day));
	MOCK_CONST_METHOD0(get_infector, Person*());
	MOCK_CONST_METHOD0(get_infected_place, Place*());
	MOCK_CONST_METHOD0(get_infectee_count, int());
	MOCK_METHOD0(add_infectee, int());
	MOCK_CONST_METHOD0(print, void());
	MOCK_CONST_METHOD0(get_exposure_date, int());
	MOCK_CONST_METHOD0(get_infectious_date, int());
	MOCK_CONST_METHOD0(get_symptomatic_date, int());
	MOCK_CONST_METHOD0(get_recovery_date, int());
	MOCK_CONST_METHOD0(get_susceptible_date, int());
	MOCK_METHOD2(modify_asymptomatic_period, void(double multp, int cur_day));
	MOCK_METHOD2(modify_symptomatic_period, void(double multp, int cur_day));
	MOCK_METHOD2(modify_infectious_period, void(double multp, int cur_day));
	MOCK_CONST_METHOD0(is_symptomatic, bool());
	MOCK_CONST_METHOD0(get_susceptibility, double());
	MOCK_CONST_METHOD0(get_infectivity, double());
	MOCK_CONST_METHOD0(get_symptoms, double());
	MOCK_METHOD2(modify_develops_symptoms, void(bool symptoms, int cur_day));
	MOCK_METHOD1(modify_susceptibility, void(double multp));
	MOCK_METHOD1(modify_infectivity, void(double multp));
};
