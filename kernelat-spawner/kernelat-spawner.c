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
#include <zmq.h>
#include "kernelat-spawner.h"
#include "ka_types.h"
#include "mm.h"
#include "tools.h"

// common vars used by workers
unsigned long int block_size = 4096;
void *zmq_context;

// worker that spawns child to get spawn time
static void spawner_worker(void *data)
{
	char *command = mm_alloc_char(CHILD_COMMAND_LENGTH);
	struct timeval prespawn_time;
	memset(&prespawn_time, 0, sizeof(struct timeval));
	spawner_worker_opdata_t *d = data;

	void *zmq_sock = zmq_socket(zmq_context, ZMQ_REP);
	char *tmpfile = mm_alloc_char(50);
	strcpy(tmpfile, "ipc://");
	strcat(tmpfile, get_unique_filename());
	zmq_bind(zmq_sock, tmpfile);

	sprintf(command, "./kernelat-child -t %s", tmpfile);
	gettimeofday(&prespawn_time, NULL);
	system(command);
	mm_free_char(command);

	zmq_msg_t msg;
	zmq_msg_init(&msg);
	zmq_recv(zmq_sock, &msg, 0);
	struct timeval postspawn_time;
	memset(&postspawn_time, 0, sizeof(struct timeval));
	memcpy(&postspawn_time, zmq_msg_data(&msg), zmq_msg_size(&msg));
	zmq_msg_close(&msg);
	zmq_close(zmq_sock);

	unsigned long int spawn_time = (postspawn_time.tv_sec * 1000000 + postspawn_time.tv_usec) - (prespawn_time.tv_sec * 1000000 + prespawn_time.tv_usec);
	d->spawn_time = spawn_time;

	return;
}

// copies from /dev/zero to some file
static void io_worker(void *data)
{
	io_worker_opdata_t *d = data;

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
			fwrite(d->buffer, block_size, 1, d->file);
		}
	}

	return;
}

int main(int argc, char **argv)
{
	// install segfault handler
	signal(SIGSEGV, signal_handler);

	int opt = 0;
	unsigned int tries = 10, dummy_io_workers = 0, real_io_workers = 0, from_threads = 1, to_threads = 100, usecs_divider = 1;

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
				real_io_workers = atoi(optarg);
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
	io_worker_opdata_t dummy_io_workers_opdata[dummy_io_workers];
	worker_data_t *real_io_workers_data[real_io_workers];
	io_worker_opdata_t real_io_workers_opdata[real_io_workers];

	// start 0mq server
	zmq_context = zmq_init(1);
	

	// prefork spawner workers
	for (unsigned int i = 0; i < to_threads; i++)
		workers_data[i] = pww_start_worker();

	// prefork dummy IO workers
	for (unsigned int i = 0; i < dummy_io_workers; i++)
	{
		dummy_io_workers_opdata[i].zero = fopen("/dev/zero", "r");
		dummy_io_workers_opdata[i].file = fopen("/dev/null", "w");
		dummy_io_workers_opdata[i].buffer = mm_alloc_char(block_size);
		dummy_io_workers_opdata[i].exit = 0;
		pthread_mutex_init(&dummy_io_workers_opdata[i].mutex, NULL);
		dummy_io_workers_data[i] = pww_start_worker();
	}

	// prefork real IO workers
	for (unsigned int i = 0; i < real_io_workers; i++)
	{
		real_io_workers_opdata[i].zero = fopen("/dev/zero", "r");
		real_io_workers_opdata[i].filename = get_unique_filename();
		real_io_workers_opdata[i].file = fopen(real_io_workers_opdata[i].filename, "w");
		real_io_workers_opdata[i].buffer = mm_alloc_char(block_size);
		real_io_workers_opdata[i].exit = 0;
		pthread_mutex_init(&real_io_workers_opdata[i].mutex, NULL);
		real_io_workers_data[i] = pww_start_worker();
	}

	for (unsigned int current_threads = from_threads; current_threads <= to_threads; current_threads++)
	{
		unsigned long int time_sum = 0;

		// cycle to repeat spawn to get average spawn time
		for (unsigned int ctry = 1; ctry <= tries; ctry++)
		{
			// start dummy IO workers
			for (unsigned int i = 0; i < dummy_io_workers; i++)
				pww_submit_task(dummy_io_workers_data[i], &dummy_io_workers_opdata[i], io_worker);

			// start real IO workers
			for (unsigned int i = 0; i < real_io_workers; i++)
				pww_submit_task(real_io_workers_data[i], &real_io_workers_opdata[i], io_worker);

			// start spawner workers
			for (unsigned int i = 0; i < current_threads; i++)
				pww_submit_task(workers_data[i], &workers_opdata[i], spawner_worker);

			// join spawner workers
			for (unsigned int i = 0; i < current_threads; i++)
			{
				pww_join_task(workers_data[i]);
				time_sum += workers_opdata[i].spawn_time;
			}

			// join real IO workers
			for (unsigned int i = 0; i < real_io_workers; i++)
			{
				pthread_mutex_lock(&real_io_workers_opdata[i].mutex);
				real_io_workers_opdata[i].exit = 1;
				pthread_mutex_unlock(&real_io_workers_opdata[i].mutex);
				pww_join_task(real_io_workers_data[i]);
				rewind(real_io_workers_opdata[i].file);
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

		fprintf(stdout, "%u\t%1.3lf\n", current_threads, (double) time_sum / (tries * current_threads * usecs_divider));
		fprintf(stderr, "Completed: %1.3lf%%\n", 100 * (double) (current_threads - from_threads + 1) / (to_threads - from_threads + 1));
	}

	for (unsigned int i = 0; i < to_threads; i++)
		pww_exit_task(workers_data[i]);

	for (unsigned int i = 0; i < real_io_workers; i++)
	{
		pww_exit_task(real_io_workers_data[i]);
		pthread_mutex_destroy(&real_io_workers_opdata[i].mutex);
		fclose(real_io_workers_opdata[i].zero);
		fclose(real_io_workers_opdata[i].file);
		mm_free_char(real_io_workers_opdata[i].buffer);
		remove(real_io_workers_opdata[i].filename);
		mm_free_char(real_io_workers_opdata[i].filename);
	}

	for (unsigned int i = 0; i < dummy_io_workers; i++)
	{
		pww_exit_task(dummy_io_workers_data[i]);
		pthread_mutex_destroy(&dummy_io_workers_opdata[i].mutex);
		fclose(dummy_io_workers_opdata[i].zero);
		fclose(dummy_io_workers_opdata[i].file);
		mm_free_char(dummy_io_workers_opdata[i].buffer);
	}

	// stop 0mq server
	zmq_term(zmq_context);

	exit(EX_OK);
}

