##################### FRED Region Makefile ###########################

GZIP := pop_Alleg.txt.gz loc_Alleg.txt.gz trips.txt.gz cell_pop.txt.gz
TXT := $(GZIP:.txt.gz=.txt)

all: $(TXT)

%.txt : %.txt.gz
	gunzip -c $< > $@

clean:
	rm -f $(TXT) *~

