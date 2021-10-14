## FRED Public
### A Framework for Reconstructing Epidemiological Dynamics
An agent-based modeling framework developed by the University of Pittsburgh [Public Health Dynamics Laboratory](http://www.phdl.pitt.edu "PHDL website") in collaboration with the [Pittsburgh Supercomputing Center](http://www.psc.edu "PSC website") and the [Carnegie Mellon University School of Computer Science](http://www.cs.cmu.edu "CMU CS website").

#### Version Information
This is the University of Pittsburgh's public version of the FRED Modeling Framework. This version will always be several check-ins behind the private version used by the University of Pittsburgh's Public Health Dynamics Laboratory.

The official commercial version of FRED is now owned, maintained, and updated by [Epistemix](https://www.epistemix.com "Epistemix website"). That version will have the most up-to-date features and cutting edge development. Please feel free to [contact Epistemix](https://www.epistemix.com/contact-us "Epistemix Contact Page") for more information.

#### License Information
FRED is distributed on the condition that users fully understand and agree to all terms of the End User License Agreement.
FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.

See the file "LICENSE" for more information.

#### Synthetic Populations
The FRED platform uses synthetic populations as the basis for creating realistic spatial population distributions and mixing group patterns. 
Synthetic populations are statistically realistic representations of the actual populations based on Public Use Microdata (PUMS) data and
Census aggregated data. The populations include individuals in geolocated households, as well as schools and workplaces (where data is available).

FRED's U.S. Synthetic Population is based on the U.S. Synthetic Population 2010 (Version 1) developed by RTI International, and includes group quarters 
such as college dormitories, prisons, nursing homes, and military barracks.

Synthetic Populations for counties and states in the United States can be downloaded from [Our Website](https://fred.publichealth.pitt.edu/syn_pops "PHDL FRED Population Download site")

#### Notes on FRED
FRED has been built, compiled and run on MacOS. It has also been compiled and run on multiple Linux flavors (notably RHEL, CentOS, OpenSUSE, and Ubuntu). It will not compile and run on a Windows system without significant changes. There have been successful attempts to compile and run on Windows using Cygwin, but this option is left up to users.

Using the make command from the top-level directory should compile all of the source-code as well as unzip any population files that are included with the FRED Download.

#### Notes on Compiling FRED
The Makefile in src has the default set for a Production optimization and c++11 standards. There are commented out lines to give examples of switching to development compiling optimization and switching to c++14 or c++17 standards.

To set the  compiler flags for optimization, uncomment the appropriate line for your environment.
```
## recommended for development:
#CPPFLAGS = -g $(CSTD) $(M64) -O0 $(LOGGING_PRESET_3) -Wall
## recommended for production runs:
CPPFLAGS = $(CSTD) $(M64) -O3 $(OPENMP) $(OSFLAGS) $(LOGGING_LEVEL) -DNCPU=$(NCPU) $(INCLUDE_DIRS)
```

To set the c++ standard uncomment the appropriate line for your compiler:
```
CSTD = -std=c++11
#CSTD = -std=c++14
#CSTD = -std=c++17
```

Once FRED has been compiled, you will want to setup some environment variables. Here is an idea of what should be in your .profile, .bashrc, or .zshrc (wherever you put your personal environment variables):

```
# FRED Environment Variables
export FRED_HOME=$HOME/FRED
export PATH="$FRED_HOME/bin:${PATH}"
export FRED_GNUPLOT=<PATH TO GNUPLOT>   #e.g. /opt/local/bin/gnuplot
```
