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
#include <signal.h>
#include <execinfo.h>
#include <libpww.h>
#include "kernelat-spawner.h"
#include "ka_types.h"
#include "mm.h"
#include "tools.h"

// common vars used by workers
unsigned long int write_worker_stop, block_size = 4096;
pthread_mutex_t write_worker_mutex;

// worker that spawns child to get spawn time
static void spawner_worker(void *data)
{
	char *command = mm_alloc_char(CHILD_COMMAND_LENGTH);
	struct timeval spawn_time;
	memset(&spawn_time, 0, sizeof(struct timeval));

	spawner_worker_opdata_t *d = data;

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

	d->spawn_time = got_time;

	mm_free_char(command);

	return;
}

// copies from /dev/zero to /dev/null
static void dummy_io_worker(void *data)
{
	dummy_io_worker_opdata_t *d = data;

	while (1)
	{
		pthread_mutex_lock(&d->mutex);
		unsigned int exit = d->exit;
		pthread_mutex_unlock(&d->mutex);
		if (exit == 1)
			break;
		else
		{
			fread(d->buffer, block_size, 1, d->zero);
			fwrite(d->buffer, block_size, 1, d->null);
		}
	}

	return;
}

// writes to file
static void *write_worker(void *nothing)
{
	(void) nothing;

	FILE *zero = fopen("/dev/zero", "rb");
	char *filename = get_unique_filename();
	FILE *f = fopen(filename, "w");
	char *buffer = mm_alloc_char(block_size);

	while (1)
	{
		// checks exit condition
		pthread_mutex_lock(&write_worker_mutex);
		if (1 == write_worker_stop)
		{
			// exits gracefully
			pthread_mutex_unlock(&write_worker_mutex);
			mm_free_char(buffer);
			fclose(zero);
			fclose(f);
			remove(filename);
			mm_free_char(filename);
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

	// workers data
	worker_data_t *workers_data[to_threads];
	spawner_worker_opdata_t workers_opdata[to_threads];
	worker_data_t *dummy_io_workers_data[dummy_io_workers];
	dummy_io_worker_opdata_t dummy_io_workers_opdata[dummy_io_workers];

	// prefork spawner workers
	for (unsigned int i = 0; i < to_threads; i++)
		workers_data[i] = pww_start_worker();

	// prefork dummy IO workers
	for (unsigned int i = 0; i < dummy_io_workers; i++)
	{
		dummy_io_workers_opdata[i].zero = fopen("/dev/zero", "r");
		dummy_io_workers_opdata[i].null = fopen("/dev/null", "w");
		dummy_io_workers_opdata[i].buffer = mm_alloc_char(block_size);
		pthread_mutex_init(&dummy_io_workers_opdata[i].mutex, NULL);
		dummy_io_workers_data[i] = pww_start_worker();
	}

	for (unsigned int current_threads = from_threads; current_threads <= to_threads; current_threads++)
	{
		unsigned long int time_sum = 0;

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
			// start dummy IO workers
			for (unsigned int i = 0; i < dummy_io_workers; i++)
			{
				pthread_mutex_lock(&dummy_io_workers_opdata[i].mutex);
				dummy_io_workers_opdata[i].exit = 0;
				pthread_mutex_unlock(&dummy_io_workers_opdata[i].mutex);
				pww_submit_task(dummy_io_workers_data[i], &dummy_io_workers_opdata[i], dummy_io_worker);
			}

			// start spawner workers
			for (unsigned int i = 0; i < current_threads; i++)
				pww_submit_task(workers_data[i], &workers_opdata[i], spawner_worker);

			// join spawner workers
			for (unsigned int i = 0; i < current_threads; i++)
			{
				pww_join_task(workers_data[i]);
				time_sum += workers_opdata[i].spawn_time;
			}

			// join dummy IO workers
			for (unsigned int i = 0; i < dummy_io_workers; i++)
			{
				pthread_mutex_lock(&dummy_io_workers_opdata[i].mutex);
				dummy_io_workers_opdata[i].exit = 1;
				pthread_mutex_unlock(&dummy_io_workers_opdata[i].mutex);
				pww_join_task(dummy_io_workers_data[i]);
			}
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
		fprintf(stderr, "Completed: %1.3lf%%\n", 100 * (double) (current_threads - from_threads + 1) / (to_threads - from_threads + 1));
	}

	for (unsigned int i = 0; i < to_threads; i++)
		pww_exit_task(workers_data[i]);

	for (unsigned int i = 0; i < dummy_io_workers; i++)
	{
		pww_exit_task(dummy_io_workers_data[i]);
		pthread_mutex_destroy(&dummy_io_workers_opdata[i].mutex);
		fclose(dummy_io_workers_opdata[i].zero);
		fclose(dummy_io_workers_opdata[i].null);
		mm_free_char(dummy_io_workers_opdata[i].buffer);
	}

	exit(EX_OK);
}

