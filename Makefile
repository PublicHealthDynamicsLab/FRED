##################### FRED Makefile ###########################

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

VER = 2.8.0

release:
	make clean
	(cd ..; tar cvzf FRED-V${VER}-`date +"%Y-%m-%d"`.tgz \
	--exclude CVS --exclude '*~' --exclude '\.*' --exclude DEPENDS \
	FRED/Makefile FRED/LICENSE FRED/bin FRED/doc FRED/data \
	FRED/src FRED/tests)	

