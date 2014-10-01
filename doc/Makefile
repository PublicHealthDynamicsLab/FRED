###########################################
#
# FRED Makefile
#

all: FRED_UserGuide.pdf # FRED_web_analytics.pdf FRED_web_simulation.pdf FRED_mobile.pdf
	

FRED_UserGuide.pdf: FRED_UserGuide.rst FRED.css
	rst2pdf FRED_UserGuide.rst -o FRED_UserGuide.pdf --repeat-table-rows -s FRED.css

FRED_web_analytics.pdf: FRED_web_analytics.rst FRED.css
	rst2pdf FRED_web_analytics.rst -o FRED_web_analytics.pdf --repeat-table-rows -s FRED.css

FRED_web_simulation.pdf: FRED_web_simulation.rst FRED.css
	rst2pdf FRED_web_simulation.rst -o FRED_web_simulation.pdf --repeat-table-rows -s FRED.css

FRED_mobile.pdf: FRED_mobile.rst FRED.css
	rst2pdf FRED_mobile.rst -o FRED_mobile.pdf --repeat-table-rows -s FRED.css

clean:
	
