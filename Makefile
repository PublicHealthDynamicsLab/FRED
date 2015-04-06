##################### FRED Makefile ###########################

DIRS = bin doc input_files src populations tests
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
	FRED/Makefile FRED/LICENSE FRED/bin FRED/doc FRED/input_files \
	FRED/populations/2010_ver1_42003.zip FRED/populations/2010_ver1_42065.zip \
	FRED/populations/Makefile FRED/src FRED/tests)




