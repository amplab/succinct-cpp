set xrange [-10:10]
set yrange [-20:20]
set y2range [-40:40]

set ytics 10 nomirror tc lt 1
set ylabel '2*x' tc lt 1

set y2tics 20 nomirror tc lt 2
set y2label '4*x' tc lt 2

plot 2*x linetype 1, 4*x/2+.5 linetype 2
