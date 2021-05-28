Module FRED::Asthma
Author: John Grefenstette and Mary Krauland
Created: 15 Apr 2019
Condition: ASTHMA
States: Start AtRisk Acute Recovered Negative Import

Summary: A simple model of asthma risk in the U.S. population. This
sub-model is intended to model risk of exposure to environmnetal
conditions (e.g. severe air pollution) that may trigger acute asthma
attacks within the population.  Therefore, only a nominal model of the
acute phase and recovery is provided.

Rules:

1. All individuals start in the Start state.

2. Each individual moves from the Start state to the AtRisk state based
on the asthma prevalance rates reported by the National Health Interview
Survey, National Center for Health Statistics, CDC.  Prevalence is based
on age, race and sex.

3. If an individual is not assigned to the AtRisk group, it enters the
Negative state and remains there.

4. The AtRisk state is susceptible to external exposure.

5. The sub-model specifies that 1% of the AtRisk individuals are subject
to exposure at the start of the simulation.

5. When an external expose occurs, the affected AtRisk individual
transitions to the Acute state.

6. The Acute state lasts for one day, after which the individual
transitions to the Recovered state.

5. The Recovered state lasts for 10 days, after which the individual
transitions back to the AtRisk state.

