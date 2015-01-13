FRED's Vaccines Model

Options in params file:

vaccine_file
   Description
     Variable that holds the name of the FRED vaccine file, defining the vaccines in the system
   Type
     string 
   Options
     filename - existing file that defines the vaccines (see vaccine.txt in FRED/tests/vaccine for format)
     none - no vaccination will be done
   Default
     none

vaccine_compliance
   Description
     The probability that someone is willing to take a vaccine. 
   Type
     double
   Options
     Probaility defined between 0.0 and 1.0
     if vaccine_file = none, this parameter is not used
   Default
     0.50

vaccine_prioritize_by_age
   Description
     Set the vaccination strategy to prioritize vaccines by certain age groups
     ( defined by vaccine_priority_age_low and vaccine_priority_age_high)
   Type
     integer
   Options
     0		Do not prioritize
     1 		Prioritize by age
     if vaccine_file = none, this parameter is not used
   Default
     0


vaccine_priority_age_low
   Description
     Lower bound to age prioritization strategy
   Type
     integer
   Options
     Must be lower than vaccine_priority_age_high
     if vaccine_file = none, this parameter is not used
     if vaccine_prioritize_by_age = 0, this parameter is not used
   Default
     0

vaccine_priority_age_high
   Description
     Lower bound to age prioritization strategy
   Type
     integer 
   Options
     Must be higher than vaccine_priority_age_low
     if vaccine_file = none, this parameter is not used
     if vaccine_prioritize_by_age = 0, this parameter is not used
   Default
     100

vaccination_capacity_file
   Description
     Location of file that defines the TimeStepMap for the number of vaccines that can 
     delivered on a given day into the system.
   Type
     string
   Options
     filename - existing file that is of the TimeStepMap format
     if vaccine_file = none, this parameter is not used
   Default
     vaccination_capacity-0.txt

vaccine_tracefile
    Description
      Location of file that records for each realization of the simulation who gets vaccinated
    Type
      string
    Options
     filename - location that will house these files at the end of the run (will overwrite)
    Default
      OUT/vacctr
