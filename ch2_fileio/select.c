/**
 * int select(int nfds, fd_set *readfds, fd_set *writefds,
 * 	      fd_set *exceptfds, struct timeval *timeout);
 * 每次调用前需要重新初始化maxfd, set, timeout
 */ 

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

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

static void select_thread_signal_callback(int sig)
{
	if (sig == SIGUSR1) {
		prog_exit = 1;
	}
}

static void *start_tcp_server(void *arg)
{
	int srv_fd, fds[MAX_CLIENT], i, j;

	if ((srv_fd = create_server_fd()) < 0)
		return NULL;
	printf("srv_fd:%d\n", srv_fd);

	for (i = 0; i < MAX_CLIENT; i++)
		fds[i] = -1;

	signal(SIGUSR1, select_thread_signal_callback);
	while (!prog_exit) {
		int max_fd = srv_fd;
		fd_set rset, eset;
		struct sockaddr client;
		socklen_t socklen;
		char buf[1024];
		int ret;
		struct timeval tv;

		for (i = 0; i < MAX_CLIENT; i++)
			max_fd = (max_fd < fds[i] ? fds[i] : max_fd);
		max_fd += 1;

		FD_ZERO(&rset);
		FD_SET(srv_fd, &rset);
		for (i = 0; i < MAX_CLIENT; i++) {
			if (!(fds[i] < 0))
				FD_SET(fds[i], &rset);
		}

		FD_ZERO(&eset);
		FD_SET(srv_fd, &eset);
		for (i = 0; i < MAX_CLIENT; i++) {
			if (!(fds[i] < 0))
				FD_SET(fds[i], &eset);
		}
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		ret = select(max_fd, &rset, NULL, NULL, NULL);
		if (ret < 0) {
			perror("select");
			continue;
		}
		for (i = 0; i < max_fd; i++) {
			if (FD_ISSET(i, &rset)) {
				if (i == srv_fd) {
					/* new connection */
					for (j = 0; j < MAX_CLIENT; j++) {
						if (fds[j] < 0)
						break;
					}
					if (j == MAX_CLIENT)
						continue;
					socklen = (socklen_t)sizeof(sizeof(struct sockaddr));
					fds[j] = accept(srv_fd, &client, &socklen);
				} else {
					/* data comming */
					ret = read(i, buf, sizeof(buf));
					if (ret <= 0) {
						//... 
					} else {
						printf("fd%d:%s\n", i, buf);
						write(i, "hello", strlen("hello") + 1);
					}
				}
			}
			if (FD_ISSET(i, &eset)) {
				printf("eset:fd%d\n", i);
				for (j = 0; j < MAX_CLIENT; j++) {
					if (fds[j] == i && i != srv_fd) {
						close(i);
						fds[j] = -1;
					}
				}
			}
		}
	}

	close(srv_fd);
	for (i = 0; i < MAX_CLIENT; i++) {
		if ( !(fds[i] < 0))
			close(fds[i]);
	}
	printf("end perfertly!\n");

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

static pthread_t select_thread;

static void sighandler(int sig)
{
	printf("sig:%d\n", sig);
	pthread_kill(select_thread, SIGUSR1);
}

int main()
{
	pthread_t ths[11];
	struct sigaction act;

	/*
	   加了就无法中断select
	   不加select过程中死掉服务器端口不能及时释放
	*/
	signal(SIGINT, sighandler);
	
	pthread_create(&ths[0], NULL, start_tcp_server, NULL);
	select_thread = ths[0];
	sleep(1);
	for (int i = 1; i < 11; i++)
		pthread_create(&ths[i], NULL, create_client, (void *)(long)i);

	for (int i = 0; i < 11; i++)
		pthread_join(ths[i], NULL);
}

/*
srv_fd:3
eset:fd3
eset:fd3
fd7:I'm client 5!
eset:fd7
client 5 recv hello
eset:fd3
fd8:I'm client 6!
eset:fd8
eset:fd3
fd8:I'm client 10!
eset:fd8
eset:fd3
eset:fd3
fd8:I'm client 7!
eset:fd8
client 7 recv hello
client 10 recv hello
client 6 recv hello
eset:fd3
eset:fd3
fd7:I'm client 3!
eset:fd7
client 3 recv hello
eset:fd3
eset:fd3
fd7:I'm client 2!
eset:fd7
client 2 recv hello
eset:fd3
eset:fd3
fd7:I'm client 1!
eset:fd7
client 1 recv hello
eset:fd3
fd6:I'm client 4!
eset:fd6
eset:fd3
eset:fd3
fd6:I'm client 9!
eset:fd6
client 9 recv hello
client 4 recv hello
eset:fd3
eset:fd3
fd6:I'm client 8!
eset:fd6
client 8 recv hello
*/