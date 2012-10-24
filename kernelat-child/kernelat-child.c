/*
    kernelat-child — small tool to measure userspace responsiveness, spawned child
    Copyright © 2012 Oleksandr Natalenko aka post-factum <pfactum@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sysexits.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
	struct timeval time_inside, time_outside;
	gettimeofday(&time_inside, NULL);
	memset(&time_outside, 0, sizeof(struct timeval));

	// parses command line arguments to get time before spawn began
	int opt = 0;
	while (-1 != (opt = getopt(argc, argv, "s:u:")))
	{
		switch (opt)
		{
			case 's':
				time_outside.tv_sec = atoi(optarg);
				break;
			case 'u':
				time_outside.tv_usec = atoi(optarg);
				break;
		}
	}

	// calculates spawn time
	unsigned long int usecs_outside = time_outside.tv_sec * 1000000 + time_outside.tv_usec;
	unsigned long int usecs_inside = time_inside.tv_sec * 1000000 + time_inside.tv_usec;
	unsigned long int spawn_time = usecs_inside - usecs_outside;

	// outputs spawn time (μsecs) to stdout
	fprintf(stdout, "%ld\n", spawn_time);

	return EX_OK;
}

