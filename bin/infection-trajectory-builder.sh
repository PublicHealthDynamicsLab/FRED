#!/bin/sh

# infection-trajectory-builder.sh: wrapper/config script for 
# infection-trajectory-builder.py
#
# J. DePasse 2011-08-23
#
# ./infection-trajectory-builder.py --help for more information.
# set intra_host_model[#] = 1 and infection_model[#] = 1
# in the FRED params files

python ./infection-trajectory-builder.py        \
  --strain              "0"                     \
  --latent              "0 0.8 0.2"             \
  --incubating          "0.8 0.2"               \
  --infectious          "0 0 0 0.3 0.4 0.2 0.1" \
  --infectivity         "0.5 1"                 \
  --symptomaticity      "0.5 1"                 \
  --infectionType       "0.3333 0.6666"         \
  --dependentIncubation "Y"

# "infectionType" is used to represent asymptomatic vs. symptomatic
# (though it could be used to have any number of different types of
# infections)


############ example for independent incubation period; allows
############ symtoms to occur before onset of infectiousness:

# --incubating      "0 0.3 0.5 0.2" --dependentIncubation N

