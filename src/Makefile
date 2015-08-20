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

################   Snappy Compression  ###############################
## Snappy can significantly improve FRED startup time by reading
## compressed population files.  If you have snappy installed,
## set the following flag to 1, and make sure the library locations
## below are correct.
SNAPPY = 0
ifeq ($(SNAPPY),1)
SNAPPY_DIR = /usr/local
SNAPPY_INC_DIR = $(SNAPPY_DIR)/include
SNAPPY_HDR = $(SNAPPY_INC_DIR)/snappy.h 
INCLUDE_DIRS = -I$(SNAPPY_INC_DIR)
SNAPPY_LIB_DIR = $(SNAPPY_DIR)/lib

# location of library differs by OS
OS := $(shell uname)
ifeq ($(OS),Linux)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.so
else ifeq ($(shell echo $(OS) | cut -f1 -d_),CYGWIN)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.la 
else ifeq ($(OS),Darwin)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.dylib
endif

SNAPPY_LFLAGS = $(SNAPPY_LIB_DIR)/libsnappy.a
SNAPPY_LDFLAGS = -L$(SNAPPY_LIB_DIR)
SNAPPY_OBJ = Compression.o
FSZ = fsz
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
# CPPFLAGS = -g -std=c++11 $(M64) -O2 $(LOGGING_PRESET_3) -Wall -DSNAPPY=$(SNAPPY)

## recommended for production runs:
CPPFLAGS = -std=c++11 $(M64) -O3 $(OPENMP) $(LOGGING_LEVEL) -DNCPU=$(NCPU) -DSNAPPY=$(SNAPPY) $(INCLUDE_DIRS)

FRED_memcheck: 	CPPFLAGS = -g -std=c++11 $(M64) -O0 -fopenmp $(LOGGING_LEVEL) -DNCPU=$(NCPU) -DSNAPPY=$(SNAPPY) -fno-omit-frame-pointer $(INCLUDE_DIRS)


###############################################

%.o : %.cc %.h
	$(CPP) $(CPPFLAGS) $(FRED_CLANG_FLAGS) -c $< $(INCLUDES)

CORE_MODULE = Fred.o Global.o Age_Map.o Timestep_Map.o Utils.o Params.o Date.o Events.o Random.o $(SNAPPY_OBJ)

ENVIRONMENTAL_MODULE = Geo.o Abstract_Grid.o Abstract_Patch.o County.o \
	Neighborhood_Layer.o Neighborhood_Patch.o \
	Regional_Layer.o Regional_Patch.o \
	Visualization_Layer.o Visualization_Patch.o \
	Seasonality_Timestep_Map.o Seasonality.o \
	Vector_Layer.o Vector_Patch.o

AGENT_MODULE = Person.o Activities.o Person_Place_Link.o Demographics.o Health.o \
	Behavior.o Intention.o Perceptions.o Travel.o Population.o 

DISEASE_MODULE = Disease.o Epidemic.o Infection.o HIV_Infection.o \
	Natural_History.o HIV_Natural_History.o Transmission.o \
	Past_Infection.o Strain.o StrainTable.o Trajectory.o Disease_List.o 

PLACE_MODULE = Place.o Household.o Neighborhood.o School.o Classroom.o \
	Workplace.o Office.o Hospital.o Place_List.o 

INTERVENTION_MODULE = Decision.o Policy.o Manager.o \
	Antiviral.o Antivirals.o AV_Decisions.o AV_Policies.o AV_Manager.o AV_Health.o \
	Vaccine_Health.o Vaccine_Dose.o Vaccine.o Vaccines.o \
	Vaccine_Priority_Decisions.o Vaccine_Priority_Policies.o Vaccine_Manager.o

VIRAL_EVOLUTION_MODULE = EvolutionFactory.o Evolution.o	MSEvolution.o Piecewise_Linear.o

OBJ = $(CORE_MODULE) $(ENVIRONMENTAL_MODULE) $(AGENT_MODULE) $(DISEASE_MODULE) $(PLACE_MODULE) $(INTERVENTION_MODULE) $(VIRAL_EVOLUTION_MODULE) 

SRC = $(OBJ:.o=.cc)

HDR = $(OBJ:.o=.h)

MD5 := FRED.md5

all: FRED FRED.tar.gz $(FSZ) $(MD5)

FRED: $(OBJ)
	$(CPP) -o $(FRED_EXECUTABLE_NAME) $(CPPFLAGS) $(INCLUDE_DIRS) $(OBJ) $(LDFLAGS) $(SNAPPY_LFLAGS) -ldl
	cp $(FRED_EXECUTABLE_NAME) ../bin

Compression.o: $(SNAPPY_LIB)
	$(CPP) -o Compression.o $(CPPFLAGS) $(SNAPPY_INCLUDE_DIRS) -c Compression.cc

fsz: $(SNAPPY_LIB) $(SNAPPY_OBJ) Global.h fsz.cc
	$(CPP) -o fsz $(CPPFLAGS) $(INCLUDE_FLAGS) $(SNAPPY_LDFLAGS) $(SNAPPY_OBJ) $(SNAPPY_LFLAGS) fsz.cc
	cp fsz ../bin

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
	(cd ../populations; make clean)
	(cd ../tests; make clean)

tags:
	find . -name \*.[ch]* | xargs etags

##############################################
