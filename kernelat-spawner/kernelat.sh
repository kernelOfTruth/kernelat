#!/usr/bin/env bash

rm -f *.out

kern_rel=`uname -r`

./kernelat-spawner -f 1 -o 50 -t 10 -w 2 -m >$kern_rel.out

echo "set grid;" >plot.out
echo "set xlabel 'Number of threads';" >>plot.out
echo "set ylabel 'Spawn delay, msecs';" >>plot.out
echo "plot '$kern_rel.out' with linespoints title '$kern_rel';" >>plot.out

