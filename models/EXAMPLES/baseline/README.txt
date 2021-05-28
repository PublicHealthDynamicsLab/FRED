
BASELINE MODEL

The baseline model in FRED includes a pandemic influenza (INF) model. It
is a pandemic model because it includes no prior immunity in the
population. The model has been calibrated to achive a 50% attack rate,
with a 67% chance of symptoms among the cases.

The following parameters in FRED/data/defaults define the INF condition:

# INFLUENZA PARAMETERS
conditions = INF

# STATES
INF.states = S E Is Ia R
INF.exposed_state = E
INF.R.is_recovered = 1

# TRANSMISSION
INF.transmission_mode = respiratory
INF.transmissibility = 1.0
INF.Is.transmissibility = 1.0
INF.Ia.transmissibility = 0.5
INF.R0_a = -0.00570298
INF.R0_b = 0.693071
INF.R0 = -1.0

# IMMUNITY
INF.immunity = 0

# TRANSITIONS
INF.S.transition_period = -1
INF.E.transition_period = 1.9
INF.E.transition_period_dispersion = 1.51
INF.Is.transition_period = 5.0
INF.Is.transition_period_dispersion = 1.5
INF.Ia.transition_period = 5.0
INF.Ia.transition_period_dispersion = 1.5
INF.R.transition_period = -1

# NON-ZERO ENTRIES IN TRASNITION MATRIX
INF.transition[E][Is] = 0.67
INF.transition[E][Ia] = 0.33
INF.transition[Ia][R] = 1
INF.transition[Is][R] = 1

# AGENT BEHAVIOR
INF.Is.probability_of_household_confinement = 0.5
INF.Is.decide_household_confinement_daily = 0

### IMPORTED INFECTIONS
INF.import_file = $FRED_HOME/data/import_10_on_day_0.txt

######################################

The METHODS file shows how to automate your workflow in a script.
It is highly recommended that you create an automated workflow to
facilitate and document your work.

Run the workflow and keep a log as follows:

% METHODS > LOG




