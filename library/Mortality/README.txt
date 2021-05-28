Module FRED::Mortality
Author: John Grefenstette
Created: 14 Apr 2019
Condition: MORTALITY
States: Wait Screening Female Male Survival Death

Rules:

1. Each individual begins in a Wait state that lasts randomly between 1
and 365 days, in order to spread out mortality events uniformly in the
population. The Wait state leads to the Screening state.

2. The Screening state leads immediately to either the Male or Female
state depending on the sex of the individual.

3. The Female state leads immediately leads to either Survival or Death
according to the individual's age-specific annual probability of
mortality for females (citation).

4. The Male state leads immediately leads to either Survival or Death
according to the individual's age-specific annual probability of
mortality for males (citation).

5. The Survival state lasts 365 days, leading to the individual's annual
Screening.

6. The Death state is a fatal state.
