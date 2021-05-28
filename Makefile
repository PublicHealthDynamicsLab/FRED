################################### FRED Makefile ########################################
##########################################################################################
## 
##  This file is part of the FRED system.
##
## Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
## Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
## Anuroop Sriram, and Donald Burke
## All rights reserved.
##
## Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
## David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
## All rights reserved.
##
## FRED is distributed on the condition that users fully understand and agree to all terms of the 
## End User License Agreement.
##
## FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
##
## See the file "LICENSE" for more information.
##
##########################################################################################

DIRS = bin doc src tests
FRED_HOME = $(CURDIR)

all:
	@for i in $(DIRS); do \
		echo $$i; \
		(cd $$i; make); \
	done

us_maps:
	tar xzvf us_maps.tgz
	mv MAPS input_files/countries/usa

clean:
	@for i in $(DIRS); do \
		echo $$i; \
		(cd $$i; make clean); \
	done

VER := $(shell cat VERSION)
release:
	make clean
	(cd ..; tar cvzf FRED-VER_${VER}.tgz \
	--exclude CVS --exclude '*~' --exclude '\.*' --exclude DEPENDS --exclude 'FRED/data/country/*' --exclude 'FRED/src/*.fred' --exclude 'FRED/src/*.txt' --exclude 'FRED/src/*.mp4' --exclude 'people.txt' --exclude 'household.txt' --exclude 'schools.txt' \
	FRED/Makefile FRED/LICENSE FRED/bin FRED/doc FRED/data FRED/models \
	FRED/src FRED/tests)




