/*
 * proto:	read(fd, buf, count)
 * 阻塞读:	0:没数据; 1:检查EINTR		
 * 非阻塞读: 0: socket连接断开; -1: 检查EINTR, EAGAIN
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>


static void test1(int block_flag)
{
	int fd = open("1.txt", O_RDONLY | block_flag);
	if (fd < 0)
		return;
	char buf[8];
	int ret = read(fd, buf, sizeof(buf));
	printf("ret = %d\n", ret);
	if (ret == 8) {
		/* normal */
	} else if( ret > 0 && ret < 8) {
		/* EOF */
	} else if (ret == 0) {
		/* EOF */
	} else if (ret < 0) {
		/* 1.EINTR */
		if (EINTR == errno) {
			//中断重读
		}
		/* 2.EAGAIN */
		if (block_flag & O_NONBLOCK) {
			//非阻塞, 无数据重读
		}
		/* 3.other errno */
		if (EINTR != errno && EAGAIN != errno) {
			//某种更严重的错误
		}
	}
}

static void read_all()
{
	int fd, ret;
	char buf[16] , *p = buf;
	size_t len = sizeof(buf);

	fd = open("/dev/random", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return;
	}
	/* buf not full && not EOF */
	while(len != 0 && (ret = read(fd, p, len)) != 0) {
		printf("len:%ld\n", len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("read");
			break;
		}
		len -= ret;
		p += ret;
	}
	printf("readsize:%ld\n", p - buf);
	printf("%s\n", buf);
}

void main()
{
	test1(0);
	test1(O_NONBLOCK);
	read_all();
}

