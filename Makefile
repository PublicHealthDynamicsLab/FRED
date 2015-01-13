##################### FRED Makefile ###########################

all:
	(cd populations; make)
	(cd region; make)
	(cd src; make)



