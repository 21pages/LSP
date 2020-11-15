/**
 * int epoll_create(int size);
 * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
 * 	EPOLL_CTL_ADD	添加fd监听
 * 	EPOLL_CTL_MOD	删除fd监听
 * 	EPOLL_CTL_DEL	改变fd监听
 * 	
 * 	typedef union epoll_data {
 *		void        *ptr;
 *		int          fd;	//通常设置为fd
 *              uint32_t     u32;
 *              uint64_t     u64;
 *	} epoll_data_t; 
 *
 *	struct epoll_event {
 *		uint32_t     events;      // Epoll events
 *		epoll_data_t data;        // User data variable
 *	};
 *
 * 	events:	EPOLLIN:	可读
 * 		EPOLLOUT:	可写
 * 		EPOLLET:	Edge Triggered, 边沿触发(事件), 默认水平触发(状态)
 * 		EPOLLPRI:	高优先级的带外数据可读
 * 		EPOLLONESHOT:	文件产生一次事件后自动不再监听
 * 		EPOLLRDHUP, EPOLLHUP, EPOLLERR:	这三个事件被自动监听
 *	返回值:	成功返回0, 失败返回-1, errno为以下值:
 *			EBADF:	fd无效
 *			EEXIST:	op为EPOLL_CTL_ADD时, fd已与当前的epfd关联
 *			EINVAL: epfd或op无效
 *			ENOENT:	op是EPOLL_CTL_MOD或EPOLL_CTL_DEL时, fd没有与epfd关联
 *			ENOMEM:	没有内存
 *			EPERM:	fd不支持epoll
 *	int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>
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
#define MAX_CLIENT		10


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
	int opt=1;
	setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	ret = bind(srvfd, (struct sockaddr *)&srv, sizeof(srv));
	if (ret < 0) {
		perror("bind");
		close(srvfd);
		return -1;
	}
	ret = listen(srvfd, 100);
	if (ret < 0) {
		perror("listen");
		return -1;
	}

	return srvfd;
}

static void epoll_thread_signal_callback(int sig)
{
	if (sig == SIGUSR1) {
		prog_exit = 1;
	}
}


static void* start_tcp_server(void *arg)
{
	int epfd = -1, srvfd = -1, clifds[MAX_CLIENT];
	struct epoll_event epevt, epevts[MAX_CLIENT + 1];
	int cliCnt = 0, clifd;
	char buf[1024];
	int i;

	// for (i = 0; i < MAX_CLIENT; i++)
	// 	clifds[i] = -1;
	epfd = epoll_create(10); //a num > 0
	if (epfd < 0) {
		perror("epoll_create");
		goto _exit;
	}

	srvfd = create_server_fd();
	if (srvfd < 0)
		goto _exit;
	
	epevt.data.fd = srvfd;
	epevt.events = EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, srvfd, &epevt) < 0) {
		perror("epoll_ctl");
		goto _exit;
	}

	signal(SIGUSR1, epoll_thread_signal_callback);
	while(prog_exit == 0) {
		int fd, wait_cnt;
		uint32_t events;
		wait_cnt = epoll_wait(epfd, epevts, MAX_CLIENT + 1, -1);
		if (wait_cnt < 0) {
			perror("epoll_wait");
			break;
		}
		for (i = 0; i < wait_cnt; i++) {
			fd = epevts[i].data.fd;
			events =  epevts[i].events;
			printf("i:%d, fd:%d, events:%d\n", i, fd, events);
			if (fd == srvfd) {
				if (events & EPOLLIN) {
					socklen_t len;
					struct sockaddr addr;
					clifd = accept(srvfd, &addr, &len);
					if (clifd > 0 && cliCnt < MAX_CLIENT) {
						epevt.events = EPOLLIN;
						epevt.data.fd = clifd;
						epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &epevt);
						cliCnt++;
						printf("server add client fd:%d\n", clifd);
					} else {
						printf("server can not add client fd:%d\n", clifd);
						close(clifd);
					}	
				}
			} else {
				if (epevts[i].events & EPOLLIN) {
					int read_len = read(fd, buf, sizeof(buf));
					if (read_len <= 0) {
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &epevt);
						close(fd);
						cliCnt--;
						printf("read_len = %d, del fd%d\n", read_len, fd);
					} else {
						printf("read from fd%d:%s\n", fd, buf);
						snprintf(buf, sizeof(buf), "hello fd%d", fd);
						write(fd, buf, strlen(buf) + 1);
					}
				}
			}
		}
	}
	
_exit:
	// for (i = 0; i < MAX_CLIENT; i++) {
	// 	if (clifds[i] < 0)
	// 		close(clifds[i]);
	// }
	if (srvfd > 0)
		close(srvfd);
	if (epfd > 0)
		close(epfd);
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

static pthread_t epoll_thread;

static void sighandler(int sig)
{
	printf("sig:%d\n", sig);
	pthread_kill(epoll_thread, SIGUSR1);
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
	epoll_thread = ths[0];
	sleep(1);
	for (int i = 1; i < 11; i++)
		pthread_create(&ths[i], NULL, create_client, (void *)(long)i);

	for (int i = 0; i < 11; i++)
		pthread_join(ths[i], NULL);
}

/*
i:0, fd:4, events:1
server add client fd:6
i:0, fd:4, events:1
server add client fd:8
i:0, fd:8, events:1
read from fd8:I'm client 8!
client 8 recv hello fd8
i:0, fd:4, events:1
server add client fd:11
i:0, fd:4, events:1
server add client fd:12
i:1, fd:11, events:1
read from fd11:I'm client 9!
i:0, fd:12, events:1
read from fd12:I'm client 10!
i:0, fd:8, events:1
read_len = 0, del fd8
client 10 recv hello fd12
i:0, fd:12, events:1
read_len = 0, del fd12
client 9 recv hello fd11
i:0, fd:11, events:1
read_len = 0, del fd11
i:0, fd:4, events:1
server add client fd:8
i:0, fd:8, events:1
read from fd8:I'm client 6!
client 6 recv hello fd8
i:0, fd:6, events:1
read from fd6:I'm client 7!
client 7 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
i:0, fd:8, events:1
read_len = 0, del fd8
i:0, fd:4, events:1
server add client fd:6
i:0, fd:6, events:1
read from fd6:I'm client 5!
client 5 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
i:0, fd:4, events:1
server add client fd:6
i:0, fd:6, events:1
read from fd6:I'm client 4!
client 4 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
i:0, fd:4, events:1
server add client fd:6
i:0, fd:6, events:1
read from fd6:I'm client 3!
client 3 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
i:0, fd:4, events:1
server add client fd:6
i:0, fd:6, events:1
read from fd6:I'm client 2!
client 2 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
i:0, fd:4, events:1
server add client fd:6
i:0, fd:6, events:1
read from fd6:I'm client 1!
client 1 recv hello fd6
i:0, fd:6, events:1
read_len = 0, del fd6
^Csig:2
epoll_wait: Interrupted system call
*/