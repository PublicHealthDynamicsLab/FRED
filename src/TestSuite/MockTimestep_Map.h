#include "../Timestep_Map.h"

class MockTimestep_Map : public Timestep_Map {
public:
  MOCK_CONST_METHOD1(get_value_for_timestep,
                     int(int ts));
  MOCK_CONST_METHOD0(is_empty,
                     bool());
  MOCK_CONST_METHOD0(print,
                     void());
};