Module FRED::Maternity
Authors: John Grefenstette and Mary Krauland
Created: 14 Apr 2019
Condition: MATERNITY
States = Start Female Male InitNotPregnant InitPregnant Screening
NotPregnant Pregnant ChildBirth PostPartum PostReproductive

Rules:

1. The Start state divides individuals into Male and Female by sex.

2. The Male state is dormant.

3. All females less than 15 are assumed to be not pregnant at start
(InitNotPregnant).

4. All females age 50+ are consider post-reproductive
(PostReproductive).  PostReproductive is a dormant state.

5. All other females are assumed to be initially pregnant (InitPregnant)
with 75% of the individual's age-specific annual probability of
pregnancy.

6. The InitPregnant state has a duration of 1..280 days with a uniform
distribution in order to spread out the initial delivery dates.

7. The InitPregnant state always transitions to ChildBirth.

8. The InitNotPregnant state has a duration of 1..365 days with a
uniform distribution in order to spread out subsequent pregnancies.

9. After the InitNotPregnant state, the individual changes to the
Screening state.

10. The Screening state branches to Pregnant, NotPregnant or
PostReproductive based on the individual's age-specific annual
probability of pregnancy.

11. The NotPregnant state has a duration of 365 day, after which the
individual has its annual Screening.

12. Pregnancy has a duration of between 259 and 294, drawn from a normal
distribution with mean 280 days and std dev 7 days.

13. Pregnancy always transitions to ChildBirth.

14. ChildBirth lasts 1 day and leads to the PostPartum state. ChildBirth
generates a maternity event (a new individual).

15. The PostPartum period duration is drawn from a normal distribution
with mean 30 days and a std dev of 15 days, with a lower bound of 10
days.

16. The PostPartum state transitions to the Screening state, resulting
in either pregnancy or a new annual screening cycle.
