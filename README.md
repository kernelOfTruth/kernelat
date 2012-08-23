kernelat
===============

Silly userspace utility to measure desktop interactivity under different loads.

Compiling
-------

### Prerequisites

* gcc
* make
* pthreads library (-lpthread)
* openssl library (-lcrypt)
* math library (-lm)

### Compilation

Enter kernelat directory and type "make".

### Setting benchmark options

Enter "kernelat-spawner" directory and open "kernelat.sh" file for editing. Find "seq 1 100" expression and replace it with your preferred threads number (seq from_number to_number). Then find "./kernelat-spawner blah-blah" row inside "for" loop and change the following:

* -t — number of tries for each step. With higher tries value more accurate results are produced.
* -w — number of real I/O threads (writing).
* -d — number of dummy I/O threads (copying from /dev/zero to /dev/null).

### Benchmarking

Enter "kernelat-spawner" and run "./kernelat.sh".

### Using results

"kernelat.sh" script produces "plot.out" file that may be plotted using gnuplot. Also you may use "`uname -r`.out" file to create own charts.

Distribution and Contribution
-----------------------------

kernelat is provided under terms and conditions of GPLv3+. See file "COPYING" for details. Mail any suggestions, bugreports and comments to me: pfactum@gmail.com.