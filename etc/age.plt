set term aqua font "Arial,20"
set title "Age-Stratified Infection in Allegheny County"
set xrange [0:90]
set yrange [0:*]
set xtics 10
set grid
set xlabel "Age Group"
set ylabel "Percent Infected" rotate
plot 'age.dat' using 1:3:7:8 notitle wi errorbars lw 1 lt 1, \
'age.dat' using 1:3 title "Serologic" wi lines lw 3 lt 1, \
'age.distr' using 1:3 title "FRED" wi linesp lw 3 lt 3 \

