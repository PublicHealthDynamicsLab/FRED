
FRED Parameters: 15 Mar 2010

strains [default = 1]: Number of strains

all_strains_antigenically_identical [default = 1]: If true, then
becoming exposed to any strain causes immutity to all other strains.
Implemented by giving the exposed person a d dummy infection for all
other strans, from which the person immediately recovers. See
Health::become_exposed().

days [default = 240]: Number of days in a simulation run.

seed [default = 123456]: Seed for random number generator for first run.
Currently there is only one RNG.

runs [default = 2]: Number of simulation runs in this executaion of
FRED.  Each run starts with a new random number seed.  Doing multiple
runs avoid the process re-reading the synthetic population files.

verbose [default = 1]: If set, print out debugging messages.  Higher
values generally produce more output.

debug [default = 0]: If set, print out debugging messages.  Higher
values generally produce more output.

test [default = 0]: L If set, only people infected on day 0 are
infectious.  Used for estimating R0.

start_day [default = 0]: If value is between 0 and 6, the simulation
starts on the given day of the week, where Sunday = 0 and Saturday = 6.
If value is greater than 6, then a random day of the week is selected
for each run. (This may be deprecated, since it probably make more sense
to runs several runs, say 20 each, with each day of the week.)

reseed_day [default = -1]: If not -1, then reset initialized the RNG
seed on the given day, but use the seed parameter at the start of each
run.  This allows the measure of conditional variance, where the first
part of the simulation is identical on each run, up to the reseed day,
and then things vary from then on.

popfile [default = alleg_pop.txt]: path to the synthetic population
file.  The first line of this file has the format:

Population = <N>

where N is the population size.  Each subsequent line has the format:

id a s m o p H N S C W O P 

where a=age, s=sex, m=marital_status, o=occupation, p=profession,
H=household, N=neighborhood, S=school, C=classroom, W=workplace,
O=office, P=profile (behavior type).

locfile [default = alleg_loc.txt]: path to the location file.

profiles [default = profiles.txt]: path to the behavioral profile file.
This specifies the daily activities patterns for each type of
individual.  Each social network is given an independent probability of
being visited, for each day of the week.  Any number of types may be
defined.  An example of a child profile is shown below.

Profile 0 child
Sun Home 1.0 Nbrhood 1.0 School 0.0 Work 0.0 Travel 0.0
Mon Home 1.0 Nbrhood 0.5 School 0.95 Work 0.0 Travel 0.0
Tue Home 1.0 Nbrhood 0.5 School 0.95 Work 0.0 Travel 0.0
Wed Home 1.0 Nbrhood 0.5 School 0.95 Work 0.0 Travel 0.0
Thu Home 1.0 Nbrhood 0.5 School 0.95 Work 0.0 Travel 0.0
Fri Home 1.0 Nbrhood 0.5 School 0.95 Work 0.0 Travel 0.0
Sat Home 1.0 Nbrhood 1.0 School 0.0 Work 0.0 Travel 0.0

outfile [default = OUT/out]: The basename of the output files containing
daily statistics.  The files are given a suffix with the run number, eg,
out1.txt, out2.txt, etc.  The first few lines looks something like:

Day   0  Str 0  S   23514  E     100  I       0  I_s       0  R       0  N   23614  AR  0.42
Day   1  Str 0  S   23506  E      27  I      81  I_s       0  R       0  N   23614  AR  0.46
Day   2  Str 0  S   23499  E       8  I     107  I_s       0  R       0  N   23614  AR  0.49

tracefile [default = OUT/trace]: the basename of the trace file
containing one record per agent, based on an event such as recovery or
the end of the simulation. daily statistics.  The files are given a
suffix with the run number, eg, trace1.txt, trace2.txt, etc.  The
records are meant to be self-documenting(!). A typical line looks
something like:

0 R id 4224 a 56 s F U exp: 8 inf: 9 rem: 18 places 6 \
infected_at H 2460 infector 4222 infectees 0 antivirals: 0

trans[s] [default = 1.0]: transmissibility of strain s.

symp[s] [default = 0.67]: probability of becoming symtomatic if infected
with strain s.

index_cases[s] [default = 100]: number of index cases for strain s.

days_latent[s] [default = 3 0.0 0.8 1.0]: Discrete CDF for latency
period for strain s (in days).  First number is length of the CDF
vector.  First value is for 0 days.

days_incubating[s] [default = 4 0.0 0.3 0.8 1.0]: Discrete CDF for
incubation period for strain s (in days).  First number is length of the
CDF vector.  First value is for 0 days.

days_asymp[s] [default = 7 0.0 0.0 0.0 0.3 0.7 0.9 1.0]: Discrete CDF
for asymptomatic period for strain s (in days).  First number is length
of the CDF vector.  First value is for 0 days.

days_symp[s] [default = 7 0.0 0.0 0.0 0.3 0.7 0.9 1.0]: Discrete CDF for
symptomatic period for strain s (in days).  First number is length of
the CDF vector.  First value is for 0 days.

immunity_loss_rate[s] [default = 0]: Rate at which recovered agent lose
immunity to strain s (in fraction/day).  When a person recovers from
strain s, a draw from an exponential distribution with this rate is
obtained.  The value is rounded off to the nearest integer, and the
person become susceptible again after the resulting number of days.  See
Strain::get_days_recovered().

symp_infectivity[s] [default = 1.0]: Infectivity of agent who is
symptomatic with strain s.

asymp_infectivity[s] [default = 0.1]: Infectivity of agent who is
asymptomatic but infected with strain s.

prob_stay_home [default = 0.5]: Probability that an agent stays home if
symptomatic with strain s.

mutation_prob [default = 1 0.0]: Probability that a strain mutates.

# Parameters influence rates of infection in various locations.
# See Place::spread_infection()
#
# contacts per location: where location ~= social-network.
# "Contact" here means contact with another person, whether or not 
# they are susceptible.
#
# Note that the # of contacts in a place is independent of the number of
# current visitors to that place.
#
# dimension: contacts/infector/time_period	(location-dependent)
#
household_contacts[s] [default = 0.38]
neighborhood_contacts[s] [default = 145]
community_contacts[s] [default = 1]
school_contacts[s] [default = 2]
classroom_contacts[s] [default = 15]
workplace_contacts[s] [default = 1]
office_contacts[s] [default = 8.5]
hospital_contacts[s] [default = 0]

# Intimacy factors
# There are a function of person-type(infector), person-type(infectee), and strain.
#
# Attenuates Pr(infection) based on the nature of the expected contact
# (distance, intimacy, handshake etc.) & the strain's known mode of transmission.
# 
# "person-type" is: adult, child, HCW, teacher, student etc.
#
# Parameters represent a square matrix with entries that represent the likelihood
# of person-type i transmitting to person-type j.

# household groups [= children adults]
household_prob[0] [default = 4 0.6 0.3 0.3 0.4]

# workplace groups [adult_workers]
workplace_prob[0] [default = 1 0.0575]
office_prob[0] [default = 1 0.0575]

# hospital groups [= patients HCWs]
hospital_prob[0] [default = 4 0.0 0.01 0.01 0.0575]

# school groups [= elem_students mid_students high_students teachers]
school_prob[0] [default = 16 0.0435 0 0 0 0 0.0375 0 0 0 0 0.0315 0 0 0 0 0.0575]
classroom_prob[0] [default = 16 0.0435 0 0 0 0 0.0375 0 0 0 0 0.0315 0 0 0 0 0.0575]

# community groups [= children adults]
community_prob[0] [default = 4 3.175e-5 1.8125e-4 3.175e-5 1.8125e-4]
neighborhood_prob[0] [default = 4 0.0048 0.0048 0.0048 0.0048]

school_closure_policy [default = none]: Values are: "none", "global",
"reactive".

school_closure_day [default = -1]: If > -1, the day on which a global
school closure occurs.

school_closure_threshold [default = 0.1]: If "global", the population
level attack rate that triggers a school closure. If "reactive", the
proportion of symptomatic agents on the populayion, that triggers school
closure.

school_closure_period [default = 10]: How many days a given school is closed.

school_closure_delay [default = 2]: Number of days delay between
school-closure-triggering event and schools actually closing.

number_antivirals [default = 0]: number of distinct antiviral drugs available.
