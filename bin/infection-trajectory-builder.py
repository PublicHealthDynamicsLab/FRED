#!/usr/bin/python

#### infection-trajectory-builder.py
#### given natural hostory parameter distributions, builds a set of infectivity
#### and symptomaticity trajectories that can be added to FRED params file for
#### use with the intra_host_model[strain#] = 1
####
#### J. DePasse 2011-08-23

import sys
import string
import os

from optparse import OptionParser


parser = OptionParser()

parser.add_option("-s", "--strain", dest="strain", default="0",
    help="Strain Number")

parser.add_option("-l", "--latent", dest="latent", default="0 0.8 0.2",
    help="Latent period probabilities (zero-indexed")

parser.add_option("-n", "--incubating", dest="incubating", default="0 0.3 0.5 0.2",
    help="Incubation period probabilities (zero-indexed")

parser.add_option("-i", "--infectious", dest="infectious", default="0 0 0 0.3 0.4 0.2 0.1",
    help="Infectious period probabilities (zero-indexed")

parser.add_option("-v", "--infectivity", dest="infectivity", default="0.5 1",
    help="Infectivity values")

parser.add_option("-y", "--symptomaticity", dest="symptomaticity", default="0.5 1",
    help="Symptomaticity values")

parser.add_option("-t", "--infectionType", dest="infectionType", default="0.3333 0.6666",
    help="Probabilities for infection type (i.e., symptomatic/asymptomatic")

parser.add_option("-d", "--dependentIncubation", dest="dependentIncubation", default="Y",
    help="Should incubation period always be greater than or equal to latent period? (Y|N)")


#parser.add_option("-p", "--plot", dest="plot", default="N",
#    help="Produce plot? Y|N")


(options, args) = parser.parse_args()

###########################################################################

latentPROB = []

for i in options.latent.split(" "):
  latentPROB.append(float(i))

incubatingPROB = []

for i in options.incubating.split(" "):
  incubatingPROB.append(float(i))

infectiousPROB = []

for i in options.infectious.split(" "):
  infectiousPROB.append(float(i))

infectivityVAL = [0.0]

for i in options.infectivity.split(" "):
  infectivityVAL.append(float(i))

symptomaticityVAL = [0.0] 

for i in options.symptomaticity.split(" "):
  symptomaticityVAL.append(float(i))

infectionTypePROB = [0.0] 

for i in options.infectionType.split(" "):
  infectionTypePROB.append(float(i))

trajectories = []

###########################################################################

def buildTrajectories(daysLatent, daysIncubating, daysInfectious, infectivity, symptomaticity):
  symTrj = []
  infTrj = []

  for d in range(daysLatent):
    infTrj.append("0")

  if (options.dependentIncubation != "Y"):
    for d in range(daysIncubating):
      symTrj.append("0")
  else:
    for d in range(daysLatent + daysIncubating):
      symTrj.append("0")

  for d in range(daysInfectious):
    infTrj.append(infectivity)

  if (options.dependentIncubation != "Y"):
    for d in range(daysInfectious + daysLatent - daysIncubating):
      symTrj.append(symptomaticity)
  else:
    for d in range(daysInfectious - daysLatent - daysIncubating):
      symTrj.append(symptomaticity)

  return( (infTrj,symTrj) )

######################################################################

for daysLatent in range(len(latentPROB)):
  latentProb = latentPROB[daysLatent]
  
  for daysIncubating in range(len(incubatingPROB)):
    incubatingProb = incubatingPROB[daysIncubating]

    for daysInfectious in range(len(infectiousPROB)):
      infectiousProb = infectiousPROB[daysInfectious]

      for infectionType in range(len(infectionTypePROB)):
        infectionTypeProb = infectionTypePROB[infectionType]
        infectivity = infectivityVAL[infectionType]
        symptomaticity = symptomaticityVAL[infectionType]

        trjProb = latentProb * incubatingProb * infectiousProb * infectionTypeProb
        
        if (trjProb > 0):
          trajectories.append( (trjProb,buildTrajectories(daysLatent, daysIncubating, daysInfectious, str(infectivity), str(symptomaticity)) ) )

trajectories.sort()

trajectoriesPROB = []

trjProbSum = 0

for t in trajectories:
  trjProbSum = trjProbSum + t[0]
  trajectoriesPROB.append(str(trjProbSum))

trajectoriesPROB[-1] = "1"

print("")

print( "infectivity_profile_probabilities[%s] = %d    %s\n" % (options.strain, len(trajectoriesPROB), " ".join(trajectoriesPROB)) )

for i in range(len(trajectories)):
  t = trajectories[i]
  print( "fixed_infectivity_profile[%s][%d] = %d    %s"  % (options.strain, i, len(t[1][0]), " ".join(t[1][0]) ))

print("")

for i in range(len(trajectories)):
  t = trajectories[i]
  print( "fixed_symptomaticity_profile[%s][%d] = %d    %s"  % (options.strain, i, len(t[1][0]), " ".join(t[1][1]) ))

print("")

##############################################################
'''
if (options.plot == "Y"):
  from pyx import *
  c = canvas.canvas()
  for t in trajectories:
    rows = int(t[0]*1000)
    cols = len(t[1][0])
    trjString = "" 
    for n in range(1,rows):
      for d in range(cols):
        trjString = trjString + "\\" + str(int(255*float(t[1][0][d]))) + "\\0\\0"
      trjString = trjString + "\n "
    print("%d %d %s" % (cols, rows, trjString))
    trjImg = bitmap.image(cols,rows,"RGB",trjString)
    trjBitmap = bitmap.bitmap(0,0,trjImg,height=0.1)
    c.insert(trjBitmap)
  c.writePDFfile("output")
'''



































