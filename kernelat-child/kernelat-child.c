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
#include <zmq.h>

int main(int argc, char **argv)
{
	struct timeval time_inside;
	gettimeofday(&time_inside, NULL);

	char *tmpfile = NULL;
	int opt = 0;
	while (-1 != (opt = getopt(argc, argv, "t:")))
	{
		switch (opt)
		{
			case 't':
				tmpfile = optarg;
				break;
		}
	}

	if (tmpfile == NULL)
	{
		fprintf(stderr, "You need to specify communication socket\n");
		exit(EX_SOFTWARE);
	}

	void *zmq_context = zmq_init(1);
	if (zmq_context == NULL)
	{
		fprintf(stderr, "Unable to initialize zmq context\n");
		exit(EX_SOFTWARE);
	}
	void *zmq_sock = zmq_socket(zmq_context, ZMQ_REQ);
	if (zmq_sock == NULL)
	{
		fprintf(stderr, "Unable to create zmq socket\n");
		exit(EX_SOFTWARE);
	}
	int rc = zmq_connect(zmq_sock, tmpfile);
	if (rc != 0)
	{
		fprintf(stderr, "Unable to connect to zmq socket\n");
		exit(EX_SOFTWARE);
	}

	zmq_msg_t msg;
	zmq_msg_init_size(&msg, sizeof(struct timeval));
	memcpy(zmq_msg_data(&msg), &time_inside, sizeof(struct timeval));
#if ZMQ_VERSION < ZMQ_MAKE_VERSION(3, 0, 0)
	rc = zmq_send(zmq_sock, &msg, 0);
#else
	rc = zmq_msg_send(&msg, zmq_sock, 0);
#endif
	if (rc == -1)
	{
		fprintf(stderr, "Unable to send message: ");
		switch (errno)
		{
			case EAGAIN:
				fprintf(stderr, "EAGAIN\n");
				break;
			case ENOTSUP:
				fprintf(stderr, "ENOTSUP\n");
				break;
			case EFSM:
				fprintf(stderr, "EFSM\n");
				break;
			case ETERM:
				fprintf(stderr, "ETERM\n");
				break;
			case ENOTSOCK:
				fprintf(stderr, "ENOTSOCK\n");
				break;
			case EINTR:
				fprintf(stderr, "EINTR\n");
				break;
			case EFAULT:
				fprintf(stderr, "EFAULT\n");
				break;
			default:
				fprintf(stderr, "EUNKNOWN\n");
				break;
		}
		exit(EX_SOFTWARE);
	}
	rc = zmq_msg_close(&msg);
	if (rc != 0)
	{
		fprintf(stderr, "Unable to close message\n");
		exit(EX_SOFTWARE);
	}
	rc = zmq_close(zmq_sock);
	if (rc != 0)
	{
		fprintf(stderr, "Unable to close socket\n");
		exit(EX_SOFTWARE);
	}
	rc = zmq_term(zmq_context);
	if (rc != 0)
	{
		fprintf(stderr, "Unable to close context\n");
		exit(EX_SOFTWARE);
	}

	return EX_OK;
}

