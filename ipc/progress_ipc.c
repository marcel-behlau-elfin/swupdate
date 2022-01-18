/*
 * Author: Christian Storm
 * Copyright (C) 2017, Siemens AG
 *
 * SPDX-License-Identifier:     LGPL-2.1-or-later
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <progress_ipc.h>

#ifdef CONFIG_SOCKET_PROGRESS_PATH
char *SOCKET_PROGRESS_PATH = (char*)CONFIG_SOCKET_PROGRESS_PATH;
#else
char *SOCKET_PROGRESS_PATH = NULL;
#endif

#define SOCKET_PROGRESS_DEFAULT  "swupdateprog"

char *get_prog_socket(void) {
	if (!SOCKET_PROGRESS_PATH || !strlen(SOCKET_PROGRESS_PATH)) {
		const char *tmpdir = getenv("TMPDIR");
		if (!tmpdir)
			tmpdir = "/tmp";

		if (asprintf(&SOCKET_PROGRESS_PATH, "%s/%s", tmpdir, SOCKET_PROGRESS_DEFAULT) == -1)
			return (char *)"/tmp/"SOCKET_PROGRESS_DEFAULT;
	}

	return SOCKET_PROGRESS_PATH;
}

static int _progress_ipc_connect(const char *socketpath, bool reconnect)
{
	fprintf(stdout, "%s called\n", __func__);
	struct sockaddr_un servaddr;
	int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sun_family = AF_LOCAL;
	strncpy(servaddr.sun_path, socketpath, sizeof(servaddr.sun_path) - 1);

	/*
	 * Check to get a valid socket
	 */
	if (fd < 0)
		return -1;

	do {
	      fprintf(stdout, "Open socket: %s\n", socketpath);
		if (connect(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == 0) {
			break;
		}
		if (!reconnect) {
			fprintf(stderr, "cannot communicate with SWUpdate via %s\n", socketpath);
			close(fd);
			return -1;
		}

		usleep(10000);
	} while (true);

	fprintf(stdout, "Connected to SWUpdate via %s\n", socketpath);
	return fd;
}

int progress_ipc_connect_with_path(const char *socketpath, bool reconnect) {
	return _progress_ipc_connect(socketpath, reconnect);
}

int progress_ipc_connect(bool reconnect)
{
	return _progress_ipc_connect(get_prog_socket(), reconnect);
}

int progress_ipc_receive(int *connfd, struct progress_msg *msg) {
	int ret = read(*connfd, msg, sizeof(*msg));

	if (ret == -1 && (errno == EAGAIN || errno == EINTR))
		return 0;

	if (ret != sizeof(*msg)) {
		fprintf(stdout, "Connection closing..\n");
		close(*connfd);
		*connfd = -1;
		return -1;
	}

	return ret;
}
