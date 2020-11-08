/**
 * int poll(struct pollfd *fds, nfds_t nfds, int timeout);
 * 不需要每次调用前需要重新初始化fds, nfds, timeout
 */
#ifndef __USE_GNU
#define __USE_GNU	1
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#define SRV_IP			"127.0.0.1"
#define SRV_PORT		12345
#define MAX_CLIENT		1


static int prog_exit = 0;

static int create_server_fd()
{
        int srvfd;
	struct sockaddr_in srv;
	int ret;

	srvfd = socket(AF_INET, SOCK_STREAM, 0);
	if (srvfd < 0) {
		perror("socket");
		return -1;				
	}
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = inet_addr(SRV_IP);//inet_ntoa
	//srv.sin_addr = htonl(INADDR_ANY);//ntohl
	srv.sin_port = htons(SRV_PORT);
	ret = bind(srvfd, (struct sockaddr *)&srv, sizeof(struct sockaddr));
	if (ret < 0) {
		perror("bind");
		close(srvfd);
		return -1;
	}
	ret = listen(srvfd, 1);
	if (ret < 0) {
		perror("listen");
		return -1;
	}

	return srvfd;
}

static void *start_tcp_server(void *arg)
{
	struct pollfd fds[MAX_CLIENT + 1];
	int srv_fd, i, j;

	if ((srv_fd = create_server_fd()) < 0)
		return NULL;
	printf("srv_fd:%d\n", srv_fd);

	for (i = 0; i < MAX_CLIENT + 1; i++) {
                fds[i].fd = -1;
                fds[i].events = POLLIN;
        }
        fds[MAX_CLIENT].fd = srv_fd;
        
	while (!prog_exit) {
		struct sockaddr client;
		socklen_t socklen;
		char buf[1024];
		int ret;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		ret = poll(fds, MAX_CLIENT + 1, -1);
		if (ret < 0) {
			perror("poll");
			continue;
		}
		printf("event coming! fds[0].revents:%d fds[1].revents:%d\n", fds[0].revents, fds[1].revents);
		if (fds[MAX_CLIENT].revents & POLLIN) {
			/* new connection */
			for (i = 0; i < MAX_CLIENT; i++) {
				if (fds[i].fd < 0)
					break;
			}
			if (i == MAX_CLIENT) {
				fds[MAX_CLIENT].events = 0;
				continue;
			}
			socklen = (socklen_t)sizeof(sizeof(struct sockaddr));
			fds[i].fd = accept(srv_fd, &client, &socklen);
			int flag = fcntl(fds[i].fd, F_GETFL, 0);
			printf("flag:%04o\n", flag);
			//fcntl(fds[i].fd, F_SETFL, flag | O_NONBLOCK);
			fds[i].events = POLLIN;
			printf("accept:fd%d\n", fds[i].fd);
		}

		for (i = 0; i < MAX_CLIENT; i++) {
			if (fds[i].revents & POLLIN){
				/* data comming */
				ret = read(fds[i].fd, buf, sizeof(buf));
				if (ret <= 0) {
						//... 
				} else {
					printf("fd%d:%s\n", i, buf);
					write(i, "hello", strlen("hello") + 1);
				}
			}
			
			if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				close(fds[i].fd);
				fds[i].fd = -1;
				printf("fd%d, revents:%d\n", fds[i].fd, fds[i].revents);
			}
		}
	}

	close(srv_fd);
	for (i = 0; i < MAX_CLIENT; i++) {
		if ( !(fds[i].fd < 0))
			close(fds[i].fd);
	}

	return NULL;
}

static void* create_client(void *arg)
{
	int fd;
	struct sockaddr_in addr;
	char buf[1024];
	int id = (long)arg;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return NULL;
	}
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SRV_IP);
	addr.sin_port = htons(SRV_PORT);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		return NULL;
	}
	snprintf(buf, sizeof(buf), "I'm client %d!", id);
	write(fd, buf, strlen(buf) + 1);
	read(fd, buf, sizeof(buf));
	printf("client %d recv %s\n", id, buf);
	close(fd);
}

static void sighandler(int sig)
{
	printf("sig:%d\n", sig);
	prog_exit = 1;
}

int main()
{
	pthread_t ths[11];
	struct sigaction act;

	signal(SIGINT, sighandler);
	
	pthread_create(&ths[0], NULL, start_tcp_server, NULL);
	sleep(1);
	for (int i = 1; i < 11; i++)
		pthread_create(&ths[i], NULL, create_client, (void *)(long)i);
	
	for (int i = 0; i < 11; i++)
		pthread_join(ths[i], NULL);
}

/*
没有实现检测客户端断开
*/