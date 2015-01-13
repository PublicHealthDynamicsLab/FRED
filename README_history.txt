Following is the struct definition, as shown in History.h:

struct health_event{
  unsigned int day:14;  // [0-16383 --> 45 yrs.]
  unsigned int type:4;  // [0-15]
  unsigned int val1:14; // [0-16384] -- for "small" values
  unsigned int val2:32; // [0-4G]    -- for "large" values
}

For event type:		val1 / val2 are:

(Health object)
HE_INFECTION		strain ID / infection status
HE_INFECTED_PLACE	strain ID / place ID (where contacted the infector)
HE_INFECTED_BY		strain ID / person ID (the infector)
HE_IMMUNITY		strain ID / boolean (have immunity?)
HE_ANTIVIRAL		strain ID / administered by (manager ID)
HE_MUTATION		old strain ID / new (mutated) strain ID

(Behavior object)
HE_VISIT		<skip> / place ID (visited a place)
HE_FAVORITE		<skip> / place ID (added a new favorite)

(Perceptions object)
HE_SUSCEPTIBILITY	strain / new perceived susceptibility value
HE_SEVERITY		strain / new perceived severity value
HE_BENEFITS		strain / new perceived benefits value
HE_BARRIERS		strain / new perceived barriers value
HE_EFFICACY		strain / new perceived self-efficacy value

Some Ranges of Values:
strain ID: 0-16383 (14 bits)
Person ID (also infector): 4 billion people (32 bits)
place ID: [4 billion]  (32 bits)
strain status: 4 states (2 bits)
 'R'=recovered
 'I'=symptomatic
 'E'= ?? exposed ??
 'i'=infectious
