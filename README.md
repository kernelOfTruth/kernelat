kernelat
===============

Silly userspace utility to measure desktop interactivity under different loads.

Compiling
---------

### Prerequisites

* gcc
* cmake
* make
* pthreads library (-lpthread)
* [pww library](https://github.com/pfactum/libpww) (-lpww)
* ZMQ library (-lzmq)
* gnuplot (optional for charts plotting)

### Compilation

Create **build** directory, enter it and type `cmake ..`. Then type `make` to build binaries. After that copy **kernelat-spawner** and **kernelat-child** into **kernelat-tester** directory.

Benchmarking
------------

### First of all

Disable everything: X, daemons, networking etc. Leave shell only.

### Setting benchmark options

Enter **kernelat-tester** directory and open **kernelat-tester.sh** file for editing. Find `./kernelat-spawner blah-blah` row and change the following:

* -f — starting threads count.
* -o — ending threads count.
* -t — number of tries for each step. With higher tries value more accurate results are produced.
* -w — number of real I/O threads (writing).
* -d — number of dummy I/O threads (copying from __/dev/zero__ to __/dev/null__).
* -b — block size of I/O.
* -m — use msecs instead of usecs.

### Benchmarking

Enter **kernelat-tester** directory and run `./kernelat-tester.sh`.

### Using results

**kernelat-tester.sh** script produces **plot.out** file that may be plotted directly using `gnuplot -persist plot.out`. Also you may use **your-kernel-version.out** file to create own charts.

Special thanks
--------------

* To Farcaller for help in debugging.
* To Ivan Titov for testing.
* To Skolot for source code review.

Distribution and Contribution
-----------------------------

kernelat is provided under terms and conditions of GPLv3+. See file "COPYING" for details. Mail any suggestions, bugreports and comments to me: oleksandr@natalenko.name.
