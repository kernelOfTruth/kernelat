/*
    kernelat-spawner — small tool to measure userspace responsiveness, spawner
    Copyright © 2012–2013 Oleksandr Natalenko aka post-factum <oleksandr@natalenko.name>

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

#ifndef KA_TYPES_H
#define KA_TYPES_H

#include <pthread.h>

typedef struct spawner_worker_opdata
{
	unsigned long int spawn_time;
} spawner_worker_opdata_t;

typedef struct io_worker_opdata
{
	FILE *zero;
	FILE *file;
	char *filename;
	char *buffer;
	unsigned int exit;
	pthread_mutex_t mutex;
} io_worker_opdata_t;

#endif

