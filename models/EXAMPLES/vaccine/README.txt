
VACCINE MODEL

This vaccine model in FRED adds a vaccine uptake condition to the
pandemic influenza INF model.  This model is meant to illustrate one way
to model vaccination, and it is not calibrated to represent any specific
vaccine.

The states for the VAC conditions are:

VAC.states = Unavail Avail Receive Immune Failed ImportStart ImportVaccine

By default all ordinary agents start in the first state Unavail.  The
Import meta-agent starts in state ImportStart.

When vaccine becomes available (as specified in the rules for the Import
agent), agents who are "exposed" to the vaccine availability transition
to the Avail state.

Agents in the Avail state decide to Receive a vaccine after a period of
between 5 and 20 days (median = 10, dispersion = 2).

After a delay of 14 days, agents receiving a vaccine transition to the
Immune state with probability 0.8 and to the Failed state with
probability 0.2.

A side effect for agents entering the Immune state is that they
reduce their susceptibility to INF by 90%.

The program vaccine.fred shows how to specify that vaccine supply for
30% of people arrives on day 0 with no spatial or age restrictions.

The programs vaccine10.fred and vaccine30.fred show how to specify that
vaccine supply arrives on day 10 or day 30, respectively. Note that
these programs include the vaccine.fred program, and merely override the
delay before vaccine arrives.

######################################

The METHODS file shows how to automate your workflow in a script.
It is highly recommended that you create an automated workflow to
facilitate and document your work.

The METHODS file shows how to create a plot comparing the effects of
vaccine arrival on day 10 vs day 30.

Run the workflow and keep a log as follows:

% METHODS > LOG




