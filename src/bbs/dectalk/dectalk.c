#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#include "c_cmmn.h"
#include "config.h"
#include "tools.h"

#define IDLE		0
#define WAIT		1
#define IDENTIFY	2

static int service_socket(void);
static void dec_talk(char *s);

FILE *dectalk_fp;
int sock_fd = ERROR;

char input[4000];
char *p = input;

extern void test_host(char*);

int
main(int argc, char *argv[])
{
	int fdlimit = getdtablesize();
	int port = DECTALK_PORT;
	char *host = DECTALK_BIND_ADDR;
	int sock = socket_listen(host, &port);
	int cnt;
	int state = IDLE;
	struct timeval t;
	char out[256];

#ifndef DEBUG
	daemon(1, 1);
#endif

	if((dectalk_fp = fopen(DECTALK_DEVICE, "w")) == NULL)
		return 1;

	while(TRUE) {
		fd_set ready;

		FD_ZERO(&ready);
		FD_SET(sock, &ready);

		if(sock_fd != ERROR)
			FD_SET(sock_fd, &ready);

		t.tv_usec = 0;
		switch(state) {
		case IDENTIFY:
			t.tv_sec = 45;
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, &t);
			break;
		case WAIT:
			t.tv_sec = 1;
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, &t);
			break;
		case IDLE:
			t.tv_sec = 0;
			cnt = select(fdlimit, &ready, (fd_set*)0, (fd_set*)0, NULL);
			break;
		}

		if(cnt < 0) {
			perror("select");
			return 1;
		}

		if(cnt == 0) {
			switch(state) {
			case IDENTIFY:
				dec_talk("N0ARY auxiliary");
				state = IDLE;
				break;

			case WAIT:
				dec_talk(input);
				p = input;
				*p = 0;
				state = IDENTIFY;
				break;
			}
			continue;
		}

		if(FD_ISSET(sock, &ready))
			if(sock_fd == ERROR)
				if((sock_fd = socket_accept(sock)) < 0) {
					perror("sola: accept");
					return 2;
				}

		if(FD_ISSET(sock_fd, &ready)) {
			state = service_socket();
		}
	}

	return 0;
}


static int
service_socket()
{
	int len = read(sock_fd, p, 256);

	if(len < 0) {
		perror("read");
		exit(2);
	}

	if(len == 0) {
		close(sock_fd);
		sock_fd = ERROR;

		if((int)p != (int)input) {
			dec_talk(input);
			p = input;
			*p = 0;
		}
		return IDENTIFY;
	}

	p += len;
	*p = 0;
	return WAIT;
}

#if 0
setup_socket(port)
int *port;
{
	struct sockaddr_in server;
	int length = sizeof(server);
	int sock = ERROR;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = *port;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		perror("opening stream socket");
	else {

		if(bind(sock, (struct sockaddr *)&server, length) < 0) {
			perror("binding stream socket");
			sock = ERROR;
		} else {

			if(getsockname(sock, (struct sockaddr *)&server, &length) < 0) {
				perror("getting socket name");
				sock = ERROR;
			} else

				listen(sock, 5);
		}
	}

	*port = server.sin_port;
	return sock;
}
#endif

static void
dec_talk(char *s)
{
	char *p = &s[strlen(s)-1];
	while(isspace(*p) || *p == ',' || *p == '.' || *p == '?')
		*p-- = 0;

	fprintf(dectalk_fp, "%s.\n", s);
}
