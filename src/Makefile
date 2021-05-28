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
## Copyright (c) 2013-2021, University of Pittsburgh, John Grefenstette, Robert Frankeny,
## David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
## All rights reserved.
##
## FRED is distributed under BSD 3-Clause with Commons Clause license.  
## See the file "LICENSE.md" for more information.
##
##########################################################################################

################   OS DEPENDENCIES   ###############################

################   Multithreading   ###############################
# For multithreading, NCPU should be set here or as an argument to make.
# NCPU must be greater than or equal to the maximum number of threads that will
# be used by OpenMP.  The number of threads used by OpenMP can be set at runtime
# with the environment variable OMP_NUM_THREADS.  The value used for OMP_NUM_THREADS
# must be less than or equal the value of NCPU set during compilation.
# If NCPU > omp_get_max_threads, some relatively small amount of memory will be
# wasted, but it's harmless otherwise.
#
# NCPU is defined to be 1 in Global.h if value not set here 
#
## select OpenMP if desired
OPENMP =
NCPU = 1
OSFLAGS =
CSTD = -std=c++11
#CSTD = -std=c++14
#CSTD = -std=c++17
# OPENMP = -fopenmp

# Don't assume that uname is available on Windows
ifeq ($(OS),Windows_NT)
    OSFLAGS += -DWIN32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        OSFLAGS += -DAMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            OSFLAGS += -DAMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            OSFLAGS += -DIA32
        endif
    endif
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        OSFLAGS += -DLINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        OSFLAGS += -DOSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        OSFLAGS += -DAMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        OSFLAGS += -DIA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        OSFLAGS += -DARM
    endif
endif

# Autodetect all available processors for linux or mac
# Fall back to single-threaded operation
### OS := $(shell uname)

ifeq ($(UNAME_S),Linux)

NCPU ?= $(shell grep -c ^processor /proc/cpuinfo)
M64 = -m64

else ifeq ($(shell echo $(UNAME_S) | cut -f1 -d_),CYGWIN)

NCPU ?=  $(shell grep -c ^processor /proc/cpuinfo)
M64 =

else ifeq ($(UNAME_S),Darwin)
NCPU ?= $(shell sysctl -n hw.ncpu)
M64 =-m64

endif


#################  MD5 Program ##############################
UNIX	:= $(shell uname)
ifeq ($(UNIX), Linux)
MD5SUM	:= md5sum
else ifeq ($(shell echo $(UNIX) | cut -f1 -d_),CYGWIN)
MD5SUM := md5sum
else
MD5SUM	:= md5 -q
endif

################   END OS DEPENDENCIES   #########################

################   Compiler Flags   ##############################
FRED_EXECUTABLE_NAME = FRED
FRED_memcheck: FRED_EXECUTABLE_NAME = FRED_memcheck
SRC_DIR = $(shell pwd)

CPP = g++
# CPP = g++-7
CXX = $(CPP)
LDFLAGS =
LFLAGS =

# comment out if not using clang (can also be set using an environmental variable)
# FRED_CLANG_FLAGS = -mllvm -inline-threshold=1000

## FRED message levels
LOGGING_PRESET_0 = 
LOGGING_PRESET_1 = -DFREDWARNING
LOGGING_PRESET_2 = -DFREDSTATUS -DFREDWARNING
LOGGING_PRESET_3 = -DFREDVERBOSE -DFREDSTATUS -DFREDWARNING -DFREDDEBUG

## select desired level of FRED messages
LOGGING_LEVEL = $(LOGGING_PRESET_3)

## recommended for development:
#CPPFLAGS = -g $(CSTD) $(M64) -O0 $(LOGGING_PRESET_3) -Wall

## recommended for production runs:
CPPFLAGS = $(CSTD) $(M64) -O3 $(OPENMP) $(OSFLAGS) $(LOGGING_LEVEL) -DNCPU=$(NCPU) $(INCLUDE_DIRS)

$(info CPPFLAGS: $(CPPFLAGS))
$(info )

FRED_memcheck: 	CPPFLAGS = -g -std=c++11 $(M64) -O0 $(OPENMP) $(LOGGING_LEVEL) -DNCPU=$(NCPU) -fno-omit-frame-pointer $(INCLUDE_DIRS)


###############################################

%.o : %.cc %.h
	$(CPP) $(CPPFLAGS) $(FRED_CLANG_FLAGS) -c $< $(INCLUDES)

CORE_MODULE = Fred.o Global.o Age_Map.o Utils.o Date.o Events.o Random.o State_Space.o \
	Property.o Factor.o Expression.o Predicate.o Clause.o Rule.o

GEO_MODULE = Geo.o Abstract_Grid.o Abstract_Patch.o \
	Admin_Division.o State.o County.o Census_Tract.o Block_Group.o \
	Neighborhood_Layer.o Neighborhood_Patch.o \
	Regional_Layer.o Regional_Patch.o \
	Visualization_Layer.o Visualization_Patch.o

AGENT_MODULE = Person.o Demographics.o Link.o Travel.o Preference.o

EPIDEMIC_MODULE = Condition.o Epidemic.o Natural_History.o \
	Transmission.o Environmental_Transmission.o Network_Transmission.o \
	Proximity_Transmission.o

MIXING_MODULE = Group_Type.o Place_Type.o Network_Type.o Group.o Place.o  Network.o Household.o Hospital.o

OBJ = $(CORE_MODULE) $(GEO_MODULE) $(AGENT_MODULE) $(EPIDEMIC_MODULE) $(MIXING_MODULE) 

SRC = $(OBJ:.o=.cc)

HDR = $(OBJ:.o=.h)

MD5 := FRED.md5

all: FRED FRED.tar.gz $(FSZ) $(MD5) FRED_API

FRED: $(OBJ)
	$(CPP) -o $(FRED_EXECUTABLE_NAME) $(CPPFLAGS) $(INCLUDE_DIRS) $(OBJ) $(LDFLAGS) -ldl
	cp $(FRED_EXECUTABLE_NAME) ../bin

FRED_memcheck: FRED

DEPENDS: $(SRC) $(HDR)
	$(CPP) -std=c++11 -MM $(SRC) $(INCLUDE_DIRS) > DEPENDS

include DEPENDS

FRED.tar.gz: $(HDR) $(SRC) Makefile
	tar -czvf FRED.tar.gz $(HDR) $(SRC) Makefile;

FRED.md5: FRED.tar.gz
	$(MD5SUM) $< > $@

FRED_API: Fred_API.o
	$(CPP) -o FRED_API $(CPPFLAGS) $(INCLUDE_DIRS) Fred_API.o $(LDFLAGS) -ldl
	cp FRED_API ../bin

VERSION:
	awk -F '.' '(NR==1){printf "%s.%s.%s\n", $$1,$$2,$$3+1}' ../VERSION > ../VERSION.tmp
	mv ../VERSION.tmp ../VERSION
	cat ../VERSION

##############################################

print:
	enscript $(SRC) $(HDR)

clean:
	rm -f *.o FRED ../bin/FRED ../bin/FRED_API fsz ../bin/fsz *~
	(cd ../tests; make clean)

tags:
	find . -name \*.[ch]* | xargs etags

##############################################
