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

// common vars used by workers
unsigned int dummy_io_worker_stop, block_size = 4096;
pthread_mutex_t dummy_io_worker_mutex;

// worker that spawns child to get spawn time
static void *spawner_worker(void *nothing)
{
	char *command = malloc(256 * sizeof(char));
	struct timeval spawn_time;

	gettimeofday(&spawn_time, NULL);	
	sprintf(command, "../kernelat-child/kernelat-child -s %ld -u %ld", spawn_time.tv_sec, spawn_time.tv_usec);
	system(command);
}

// copies from /dev/zero to /dev/null
static void *dummy_io_worker(void *nothing)
{
	FILE *zero = fopen("/dev/zero", "rb");
	FILE *null = fopen("/dev/null", "w");
	char *buffer = malloc(16384 * sizeof(char));

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

int main(int argc, char **argv)
{
	int opt = 0;
	unsigned int spawner_threads = 1, tries = 10, dummy_io_workers = 0;

	// parses command line arguments
	while (-1 != (opt = getopt(argc, argv, "c:t:d:b:")))
	{
		switch (opt)
		{
			case 'c':
				spawner_threads = atoi(optarg);
				break;
			case 't':
				tries = atoi(optarg);
				break;
			case 'd':
				dummy_io_workers = atoi(optarg);
				break;
			case 'b':
				block_size = atoi(optarg);
				break;
		}
	}

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

	// cycle to repeat spawn to get average spawn time
	while (0 < tries--)
	{
		pthread_t ids[spawner_threads];
		
		unsigned int i;
		for (i = 0; i < spawner_threads; i++)
			pthread_create(&ids[i], NULL, spawner_worker, NULL);

		for (i = 0; i < spawner_threads; i++)
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

	return EX_OK;
}

