/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2019  Black Sphere Technologies Ltd.
 * Written by UweBonnes bon@elektron.ikp.physik.tu-darmstadt.de
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file sends the stlink trace data.
 */

#if defined(_WIN32) || defined(__CYGWIN__)
#   include <winsock2.h>
#   include <windows.h>
#   include <ws2tcpip.h>
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <sys/select.h>
#   include <fcntl.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "general.h"

static int trace_if_serv, trace_if_conn;
#define DEFAULT_PORT 2332
#define NUM_TRACE_SERVER 4
int trace_if_init(void)
{
#if defined(_WIN32) || defined(__CYGWIN__)
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	struct sockaddr_in addr;
	int opt;
	int port = DEFAULT_PORT - 1;

	do {
		port ++;
		if (port > DEFAULT_PORT + NUM_TRACE_SERVER)
			return - 1;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		trace_if_serv = socket(PF_INET, SOCK_STREAM, 0);
		if (trace_if_serv == -1)
			continue;

		opt = 1;
		if (setsockopt(trace_if_serv, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt)) == -1) {
			close(trace_if_serv);
			continue;
		}
		if (setsockopt(trace_if_serv, IPPROTO_TCP, TCP_NODELAY, (void*)&opt, sizeof(opt)) == -1) {
			close(trace_if_serv);
			continue;
		}
		if (bind(trace_if_serv, (void*)&addr, sizeof(addr)) == -1) {
			close(trace_if_serv);
			continue;
		}
		if (listen(trace_if_serv, 1) == -1) {
			close(trace_if_serv);
			continue;
		}
		break;
	} while(1);
#if defined(_WIN32) || defined(__CYGWIN__)
	unsigned long opt = 1;
	ioctlsocket(trace_if_serv, FIONBIO, &opt);
#else
	int	flags = fcntl(trace_if_serv, F_GETFL);
	fcntl(trace_if_serv, F_SETFL, flags | O_NONBLOCK);
#endif
	DEBUG("Listening on TCP: %4d\n", port);
	return 0;
}

int trace_if_accept(void)
{
	trace_if_conn = accept(trace_if_serv, NULL, NULL);
	if (trace_if_conn == -1) {
		if (errno != EWOULDBLOCK) {
			DEBUG("error when accepting connection: %s", strerror(errno));
		}
		return -1;
	} else {
		DEBUG("Got connection\n");
	}
	return 0;
}

int trace_if_putbuf(unsigned char *buf, int size)
{
	if (send(trace_if_conn, buf,size, MSG_NOSIGNAL) == -1)
		return -1;
	return 0;
}
