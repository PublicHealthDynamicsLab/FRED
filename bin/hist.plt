#!/usr/local/bin/gnuplot

# file hist.plt
#
# displays histogram of values in column 2 of file 'data.txt'
#
# Thanks to Neil Carter: 
#   http://psy.swansea.ac.uk/staff/carter/gnuplot/gnuplot_frequency.htm
#

clear
reset
set term aqua font "Arial,18"

set key off
set border 3

bin_width = 0.01

# Each bar is 100% the (visual) width of its x-range.
set boxwidth (1.0*bin_width) absolute
set style fill solid 1.0 noborder

bin_number(x) = floor(x/bin_width)

stats 'data.txt' using 2 nooutput
N = STATS_records
LO = STATS_lo_quartile
HI = STATS_up_quartile
MED = STATS_median

# stats 'data.txt' using 3 nooutput
# EXP_LO = STATS_min

# stats 'data.txt' using 4 nooutput
# EXP_HI = STATS_min

rounded(x) = bin_width * ( bin_number(x) + 0.5 )
set title "Observed Distribution"
set ylabel "Percent" offset 1.5,0
set ylabel "Frequency" offset 1.5,0
set xlabel "Days" offset 0,0.5
set grid
set xtics nomirror out 1
set ytics out nomirror

# set arrow 1 from LO, graph 0 to LO, graph 1 nohead front lw 2 lc rgb "yellow"
set arrow 2 from MED, graph 0 to MED, graph 1 nohead front lw 2 lc rgb "black"
# set arrow 3 from HI, graph 0 to HI, graph 1 nohead front lw 2 lc rgb "red"
# set arrow 4 from EXP_LO, graph 0 to EXP_LO, graph 1 nohead front lw 2 lc rgb "green"
# set arrow 5 from EXP_HI, graph 0 to EXP_HI, graph 1 nohead front lw 2 lc rgb "orange"

bot_legend = 0.5

set arrow 11 from graph 0.8, graph (bot_legend+0.1) to graph 0.85, graph (bot_legend+0.1) nohead front lw 3 lc rgb "yellow"
set arrow 22 from graph 0.8, graph (bot_legend+0.05) to graph 0.85, graph (bot_legend+0.05) nohead front lw 3 lc rgb "black"
set arrow 33 from graph 0.8, graph bot_legend to graph 0.85, graph (bot_legend) nohead front lw 3 lc rgb "red"

set label 1 "1st Quartile" at graph 0.86, graph (bot_legend+0.1) front
set label 2 "Median" at graph 0.86, graph (bot_legend+0.05) front
set label 3 "3rd Quartile" at graph 0.86, graph (bot_legend) front

set object 1 rect from graph 0.78, graph (bot_legend-0.05) to graph 1, graph (bot_legend+0.15) back
set object 1 rect fc rgb "gray" fillstyle solid 1

set grid
set xrange [0:10]

plot 'data.txt' using (rounded($2)):(1) smooth frequency with boxes lt 6
