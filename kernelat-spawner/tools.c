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
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <string.h>
#include "mm.h"

void signal_handler(int sig)
{
	void *array[10];
	size_t size;

	size = backtrace(array, 10);
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(EX_SOFTWARE);
}

void gen_random(char *s, const int len)
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len - 1; ++i)
	{
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len - 1] = 0;
}

char *get_unique_filename(void)
{
	char *filename = mm_alloc_char(44);
	struct stat sts;
	while (1)
	{
		char headname[40];
		gen_random(headname, 40);
		strncpy(filename, headname, 40);
		strcat(filename, ".out");

		if (stat(filename, &sts) == -1)
			break;
		else
		{
			mm_free_char(&filename);
			filename = mm_alloc_char(44);
		}
		
	}
	return filename;
}

// gets file size
long fsize(FILE *file)
{
	long current_pos = ftell(file);
	fseek(file, 0, SEEK_END);
	long res = ftell(file);
	fseek(file, 0, current_pos);
	return res;
}

