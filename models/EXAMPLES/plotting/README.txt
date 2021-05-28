
PLOTTING WITH THE INF MODEL

This model illustrates how to use additional conditions in FRED to plot
a variety of variables of interest.

FRED automatically generates three daily counts for each state S defined
by the user: newS, S, and totS. The value of newS is the number of
agents entering state S each day. The value of S is the prevalence of S,
or the number of agents currently in state S. The value of totS is the
number of agents that have ever been in state S.

Sometimes it is useful to define new conditions for the purpose of
reporting variables that do not appear in the FRED code.

EXAMPLE 1

As a simple example using the INF model, suppose we want to report the
daily number of people who are infectious each day. This would include
people in either or the two states INF.Ia or INF.Is. The file params1
show one way to do this, by defining a second condition call REPORT, in
which people are either Infectious or Not.

By default, all agents start in the first state, Not.

Since no transition rules are provided, agents stay in each state
forever, unless modified externally.  In this case, we have rules that
changes the REPORT state from Not to Infectious whenever an agent enters
state INF.Ia or INF.Is.

When an agent enters the INF.R recovered state, it also changes from
Infectious back to Not.

The plot commands in METHODS shows examples of plotting these variables.

EXAMPLE 2

A more interesting example is to breakdown the cases into adults
vs. children. The file params2 shows a REPORT condition that keeps track
of infections by age group.  In this case, entering state INF.E also
cause transition to state REPORT.E, which then immediately transitions
into either state REPORT.Child or REPORT.Adult, depending on the age of
the agent.

We use the fact that each transition matrix element is actually a
logistic regression function. We set the probability of changing to the
Child state to 1.0 if the age is between 0 and 15.  Otherwise, we set
the probability of changing to the Adult state to 0.5. The explanation
depends on how the transition probabilities are computed in FRED.  If
any transition probability is exactly 1.0, then all other transition
probabilities are ignored, and the transition is made deterministically
to the target state. (If there is more than one such target state, the
final one will be selected. See Natural_History::get_next_state().)

So in the case, if any of the age-groups matches the agent's age, the
transition will be fro REPORT.E to REPORT.Child.

If none of these age-groups matches, then the only positive probability
with be to REPORT.Adult. In that case, since the probabilties are first
normalized, the transition will always be to REPORT.Adult. Any positive
value less than 1.0 would work the same as 0.5 in this example.

The resulting plots show the breakson between adults and children. Not
that the epidemic among children peaks earlier than the epidemic among
adults, due to the high mixing rates in schools.

######################################

The METHODS file shows how to automate your workflow in a script.
It is highly recommended that you create an automated workflow to
facilitate and document your work.

Run the workflow and keep a log as follows:

% METHODS > LOG




