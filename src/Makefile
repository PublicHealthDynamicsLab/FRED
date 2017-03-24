##################### FRED Makefile ###########################

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
# OPENMP = -fopenmp

# Autodetect all available processors for linux or mac
# Fall back to single-threaded operation
OS := $(shell uname)

ifeq ($(OS),Linux)

NCPU ?= $(shell grep -c ^processor /proc/cpuinfo)
M64 = -m64

else ifeq ($(shell echo $(OS) | cut -f1 -d_),CYGWIN)

NCPU ?=  $(shell grep -c ^processor /proc/cpuinfo)
M64 =

else ifeq ($(OS),Darwin)

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
# CPPFLAGS = -g -std=c++11 $(M64) -O2 $(LOGGING_PRESET_3) -Wall

## recommended for production runs:
CPPFLAGS = -std=c++11 $(M64) -O3 $(OPENMP) $(LOGGING_LEVEL) -DNCPU=$(NCPU) $(INCLUDE_DIRS)

FRED_memcheck: 	CPPFLAGS = -g -std=c++11 $(M64) -O0 -fopenmp $(LOGGING_LEVEL) -DNCPU=$(NCPU) -fno-omit-frame-pointer $(INCLUDE_DIRS)


###############################################

%.o : %.cc %.h
	$(CPP) $(CPPFLAGS) $(FRED_CLANG_FLAGS) -c $< $(INCLUDES)

CORE_MODULE = Fred.o Global.o Age_Map.o Timestep_Map.o Utils.o Params.o Date.o Events.o \
	Random.o State_Space.o Logit.o Logistic_Regression.o

ENVIRONMENTAL_MODULE = Geo.o Abstract_Grid.o Abstract_Patch.o County.o Census_Tract.o \
	Neighborhood_Layer.o Neighborhood_Patch.o \
	Regional_Layer.o Regional_Patch.o \
	Visualization_Layer.o Visualization_Patch.o \
	Seasonality_Timestep_Map.o Seasonality.o \
	Vector_Layer.o Vector_Patch.o

AGENT_MODULE = Person.o Activities.o Person_Place_Link.o Demographics.o Health.o \
	Travel.o Population.o Person_Network_Link.o

EPIDEMIC_MODULE = Condition.o Epidemic.o Condition_List.o \
	Natural_History.o Transmission.o Environmental_Transmission.o \
	Respiratory_Transmission.o Sexual_Transmission.o Vector_Transmission.o \
	HIV_Epidemic.o  HIV_Infection.o HIV_Natural_History.o

MIXING_MODULE = Mixing_Group.o Place.o Household.o Neighborhood.o School.o Classroom.o \
	Workplace.o Office.o Hospital.o Place_List.o Network.o Sexual_Transmission_Network.o \
	Relationships.o

OBJ = $(CORE_MODULE) $(ENVIRONMENTAL_MODULE) $(AGENT_MODULE) $(EPIDEMIC_MODULE) $(MIXING_MODULE) 

SRC = $(OBJ:.o=.cc)

HDR = $(OBJ:.o=.h)

MD5 := FRED.md5

all: FRED FRED.tar.gz $(FSZ) $(MD5)

FRED: $(OBJ)
	$(CPP) -o $(FRED_EXECUTABLE_NAME) $(CPPFLAGS) $(INCLUDE_DIRS) $(OBJ) $(LDFLAGS) -ldl
	cp $(FRED_EXECUTABLE_NAME) ../bin

FRED_memcheck: FRED

FRED_Unit_Tracker: 
	cd TestSuite/Tracker; $(CPP) -std=c++11 -g -O0 -DUNIT_TEST=1 -I../../ Tracker_Unit_Test.cc -c -o Tracker_Unit_Test.o
	cd TestSuite/Tracker; $(CPP) -std=c++11 -g -O0 -o FRED_Unit_Tracker -DUNIT_TEST=1 -I../../ Tracker_Unit_Test.o

DEPENDS: $(SRC) $(HDR)
	$(CPP) -std=c++11 -MM $(SRC) $(INCLUDE_DIRS) > DEPENDS

include DEPENDS

FRED.tar.gz:
	tar -czvf FRED.tar.gz $(HDR) $(SRC) Makefile;

FRED.md5: FRED.tar.gz
	$(MD5SUM) $< > $@

##############################################

print:
	enscript $(SRC) $(HDR)

clean:
	rm -f *.o FRED FRED_Unit_Tracker ../bin/FRED fsz ../bin/fsz *~
	(cd ../tests; make clean)

tags:
	find . -name \*.[ch]* | xargs etags

##############################################
