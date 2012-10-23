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

#include <stdio.h>
#include <stdlib.h>
#include "mm.h"
#include "ka_types.h"

char *mm_alloc_char(int size)
{
	if (size < 1)
	{
		fprintf(stderr, "mm_alloc_char: size is too small\n");
		return NULL;
	}

	char *data = calloc(size, sizeof(char));
	if (!data)
	{
		fprintf(stderr, "mm_alloc_char: unable to allocate %d\n", size);
		return NULL;
	}

	return data;
}

void mm_free_char(char *data)
{
	if (!data)
	{
		fprintf(stderr, "mm_free_char: memory is not allocated\n");
		return;
	}

	free(data);
	data = NULL;
}

spawner_worker_data_t *mm_alloc_spawner_worker_data_t(int size)
{
	if (size < 1)
	{
		fprintf(stderr, "mm_alloc_spawner_worker_data_t: size is too small\n");
		return NULL;
	}

	spawner_worker_data_t *data = calloc(size, sizeof(spawner_worker_data_t));
	if (!data)
	{
		fprintf(stderr, "mm_alloc_spawner_worker_data_t: unable to allocate %d\n", size);
		return NULL;
	}

	return data;
}

void mm_free_spawner_worker_data_t(spawner_worker_data_t *data)
{
	if (!data)
	{
		fprintf(stderr, "mm_free_spawner_worker_data_t: memory is not allocated\n");
		return;
	}

	free(data);
	data = NULL;
}

