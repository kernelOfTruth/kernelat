#!/usr/bin/env bash

rm -f *.out

kern_rel=`uname -r`

for i in `seq 1 100`
do
	./kernelat-spawner -c $i -t 10 -w 5 >$i.out
	avg=`awk '{ total += $1; count++ } END { print total/count}' $i.out`
	rm "$i.out"
	echo -e "$i\t$avg" >>$kern_rel.out
	echo $i >/dev/stderr
done

echo "set grid;" >plot.out
echo "set xlabel 'Number of threads';" >>plot.out
echo "set ylabel 'Spawn delay, μsecs';" >>plot.out
echo "plot '$kern_rel.out' with linespoints title '$kern_rel';" >>plot.out

