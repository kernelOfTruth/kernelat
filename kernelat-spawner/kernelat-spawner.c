/*
    kernelat-spawner — small tool to measure userspace responsiveness, spawner
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
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <execinfo.h>

// common vars used by workers
unsigned long int dummy_io_worker_stop, write_worker_stop, time_sum, block_size = 4096;
pthread_mutex_t dummy_io_worker_mutex, write_worker_mutex, time_output_mutex;

void signal_handler(int sig)
{
	void *array[10];
	size_t size;

	size = backtrace(array, 10);
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(EX_SOFTWARE);
}

// worker that spawns child to get spawn time
static void *spawner_worker(void *nothing)
{
	(void) nothing;

	char *command = calloc(256, sizeof(char));
	struct timeval spawn_time;

	gettimeofday(&spawn_time, NULL);	
	sprintf(command, "../kernelat-child/kernelat-child -s %ld -u %ld", spawn_time.tv_sec, spawn_time.tv_usec);
	FILE *pf = popen(command, "r");
	if (pf == NULL)
	{
		fprintf(stderr, "Couldn't open output pipe\n");
		exit(EX_IOERR);
	}
	unsigned long int got_time = 0;
	fscanf(pf, "%lu\n", &got_time);
	pclose(pf);

	pthread_mutex_lock(&time_output_mutex);
	time_sum += got_time;
	pthread_mutex_unlock(&time_output_mutex);

	return NULL;
}

// copies from /dev/zero to /dev/null
static void *dummy_io_worker(void *nothing)
{
	(void) nothing;

	FILE *zero = fopen("/dev/zero", "rb");
	FILE *null = fopen("/dev/null", "w");
	char *buffer = calloc(block_size, sizeof(char));

	while (1)
	{
		// checks exit condition
		pthread_mutex_lock(&dummy_io_worker_mutex);
		if (1 == dummy_io_worker_stop)
		{
			// exits gracefully
			pthread_mutex_unlock(&dummy_io_worker_mutex);
			free(buffer);
			fclose(zero);
			fclose(null);
			pthread_exit(NULL);
		} else
			pthread_mutex_unlock(&dummy_io_worker_mutex);

		// reads block from /dev/zero and writes it to /dev/null
		fread(buffer, block_size, 1, zero);
		fwrite(buffer, block_size, 1, null);
	}
}

void gen_random(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i)
	{
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}


// writes to file
static void *write_worker(void *nothing)
{
	(void) nothing;

	char *filename = calloc(44, sizeof(char));
	struct stat sts;
	do
	{
		if (filename != NULL)
		{
			free(filename);
			filename = NULL;
			filename = calloc(44, sizeof(char));
		}
		char headname[40];
		gen_random(headname, 40);
		strncpy(filename, headname, 40);
		strcat(filename, ".out");
	} while (stat(filename, &sts) != -1);

	FILE *zero = fopen("/dev/zero", "rb");
	FILE *f = fopen(filename, "w");
	char *buffer = calloc(block_size, sizeof(char));

	while (1)
	{
		// checks exit condition
		pthread_mutex_lock(&write_worker_mutex);
		if (1 == write_worker_stop)
		{
			// exits gracefully
			pthread_mutex_unlock(&write_worker_mutex);
			free(buffer);
			fclose(zero);
			fclose(f);
			remove(filename);
			pthread_exit(0);
		} else
			pthread_mutex_unlock(&write_worker_mutex);

		// reads block from /dev/zero and writes it to file
		fread(buffer, block_size, 1, zero);
		fwrite(buffer, block_size, 1, f);
	}
}

int main(int argc, char **argv)
{
	// install segfault handler
	signal(SIGSEGV, signal_handler);

	int opt = 0;
	unsigned int tries = 10, dummy_io_workers = 0, write_workers = 0, from_threads = 1, to_threads = 100, usecs_divider = 1;

	// parses command line arguments
	while (-1 != (opt = getopt(argc, argv, "t:d:b:w:f:o:m")))
	{
		switch (opt)
		{
			case 't':
				tries = atoi(optarg);
				break;
			case 'd':
				dummy_io_workers = atoi(optarg);
				break;
			case 'b':
				block_size = atoi(optarg);
				break;
			case 'w':
				write_workers = atoi(optarg);
				break;
			case 'f':
				from_threads = atoi(optarg);
				break;
			case 'o':
				to_threads = atoi(optarg);
				break;
			case 'm':
				usecs_divider = 1000;
				break;
		}
	}

	pthread_mutex_init(&time_output_mutex, NULL);
	for (unsigned int current_threads = from_threads; current_threads <= to_threads; current_threads++)
	{
		time_sum = 0;

		// starts dummy IO workers if needed
		pthread_t dummy_io_worker_id[dummy_io_workers];
		if (0 < dummy_io_workers)
		{
			dummy_io_worker_stop = 0;
			pthread_mutex_init(&dummy_io_worker_mutex, NULL);
			unsigned int i;
			for (i = 0; i < dummy_io_workers; i++)
				pthread_create(&dummy_io_worker_id[i], NULL, dummy_io_worker, NULL);
		}

		// starts write IO workers if needed
		pthread_t write_worker_id[write_workers];
		if (0 < write_workers)
		{
			write_worker_stop = 0;
			pthread_mutex_init(&write_worker_mutex, NULL);
			unsigned int i;
			for (i = 0; i < write_workers; i++)
				pthread_create(&write_worker_id[i], NULL, write_worker, NULL);
		}

		// cycle to repeat spawn to get average spawn time
		for (unsigned int ctry = 0; ctry <= tries; ctry++)
		{
			pthread_t ids[current_threads];
			
			unsigned int i;
			for (i = 0; i < current_threads; i++)
				pthread_create(&ids[i], NULL, spawner_worker, NULL);

			for (i = 0; i < current_threads; i++)
				pthread_join(ids[i], NULL);
		}

		// cleans gracefully after dummy IO workers
		if (0 < dummy_io_workers)
		{
			pthread_mutex_lock(&dummy_io_worker_mutex);
			dummy_io_worker_stop = 1;
			pthread_mutex_unlock(&dummy_io_worker_mutex);

			unsigned int i = 0;
			for (i = 0; i < dummy_io_workers; i++)
				pthread_join(dummy_io_worker_id[i], NULL);
			pthread_mutex_destroy(&dummy_io_worker_mutex);
		}

		// cleans gracefully after write IO workers
		if (0 < write_workers)
		{
			pthread_mutex_lock(&write_worker_mutex);
			write_worker_stop = 1;
			pthread_mutex_unlock(&write_worker_mutex);

			unsigned int i = 0;
			for (i = 0; i < write_workers; i++)
				pthread_join(write_worker_id[i], NULL);
			pthread_mutex_destroy(&write_worker_mutex);
		}

		fprintf(stdout, "%u\t%1.3lf\n", current_threads, (double) time_sum / (tries * current_threads * usecs_divider));
	}
	pthread_mutex_destroy(&time_output_mutex);

	return EX_OK;
}

