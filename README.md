kernelat
===============

Silly userspace utility to measure desktop interactivity under different loads.

Compiling
---------

### Prerequisites

* gcc
* make
* pthreads library (-lpthread)
* math library (-lm)
* pww library (-lpww)
* gnuplot (optional for charts plotting)

### Compilation

Enter kernelat directory and type `make`.

Benchmarking
------------

### First of all

Disable everything: X, daemons, networking etc. Leave shell only.

### Setting benchmark options

Enter **kernelat-spawner** directory and open **kernelat.sh** file for editing. Find `./kernelat-spawner blah-blah` row and change the following:

* -f — starting threads count.
* -o — ending threads count.
* -t — number of tries for each step. With higher tries value more accurate results are produced.
* -w — number of real I/O threads (writing).
* -d — number of dummy I/O threads (copying from __/dev/zero__ to __/dev/null__).
* -b — block size of I/O.
* -m — use msecs instead of usecs.

### Benchmarking

Enter **kernelat-spawner** directory and run `./kernelat.sh`.

### Using results

**kernelat.sh** script produces **plot.out** file that may be plotted directly using `gnuplot -persist plot.out`. Also you may use **your-kernel-version.out** file to create own charts.

Distribution and Contribution
-----------------------------

kernelat is provided under terms and conditions of GPLv3+. See file "COPYING" for details. Mail any suggestions, bugreports and comments to me: pfactum@gmail.com.
