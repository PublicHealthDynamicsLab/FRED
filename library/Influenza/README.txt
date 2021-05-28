Module FRED::Influenza
Author: John Grefenstette
Created: 14 Apr 2019
Condition: INF
States: S E Is Ia R Import

Summary: A simple model of pandemic influenza in a modifed S-E-I-R
model with rules:

1. All individuals start in the susceptible state S.

2. An individual enters state E when exposed by another infectious
individual or by importation.

3. The latent period (state E) lasts between about 1.5 and 3.5 days
(lognormal(1.9,1.23)) after which about 67% of exposed individuals
become infectious with symptoms (state Is) and the rest become
infectious while asymptomatic (state Ia).

4. Individuals with symptoms are twice as infectious as those who are
asymptomatic.

5. Individuals with symptoms have a 50% chance of household confinement
for the duration of thie illness.

5. The infectious period lasts between about 3.3 and 7.5 days
(lognormal(5.0,1.5) distribution).

6. After the infectious period, all individuals recover (state R) and
remain immune for the remainder of the simulation.
