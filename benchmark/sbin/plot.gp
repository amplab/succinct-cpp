# Note you need gnuplot 4.4 for the pdfcairo terminal.
set terminal pdfcairo font "Gill Sans,8" linewidth 4 rounded
set term postscript eps color linewidth 4 rounded

# Line style for axes
set style line 80 lt rgb "#808080"

set size 1,1

# Line style for grid
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey

set grid back linestyle 81
set border 3 back linestyle 80 

set xtics nomirror
set ytics nomirror
set y2tics nomirror

#set log x 2
#set log y
#set mxtics 10    # Makes logscale look good.
#set mytics 10

# Line styles: try to pick pleasing colors, rather
# than strictly primary colors or hard-to-see colors
# like gnuplot's default yellow.  Make the lines thick
# so they're easy to see in small plots in papers.
set style line 1 lt 1 linecolor rgb "#A00000" lw 1 pt 2
set style line 2 lt 1 linecolor rgb "#00A000" lw 1 pt 6
set style line 3 lt 1 linecolor rgb "#5060D0" lw 1 pt 1
set style line 4 lt 2 linecolor rgb "#FF8C00" lw 1 pt 4

set output "../plot/timeseries.eps"
set timefmt "%s"
set xdata time
set format x "%H:%M"
set xlabel "Time (s)" font "Helvetica, 16"
set ylabel "Query Throughput (ops/sec)" font "Helvetica, 16"
set y2label "Memory Footprint (GB)" font "Helvetica, 16"

#set key top right

#set xrange [2:10]
#set yrange [1:]
max(x,y) = (x > y) ? x : y

plot "../res/adashard-bench.req" using ($1/1000/1000):2 title "request-rate" w lp ls 1,\
	"../res/adashard-bench.res" using ($1/1000/1000):2 title "response-rate" w lp ls 2,\
	"../res/adashard-bench.res" using ($1/1000/1000):($3/1024/1024/1024) axes x1y2 title "memory-footprint" w lp ls 3
