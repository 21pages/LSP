/*
 * proto:	open(name, flag, mode)
 * flag:	O_RDONLY, O_WRONLY, O_RDWR
 * 		read:O_NONBLOCK
 * 		write:O_APPEND, O_CREAT, O_EXCL, O_TRUNC
 * 		O_DIRECT, O_DIRECTORY, O_LARGEFILE, O_NOCTTY, O_NOFOLLOW, O_ASYNC, O_SYNC
 * mode:	useful when O_CREAT
 * 		rwxrwxrwx, UGO, 0644
 * 		O_IRWXU, O_IRUSR, O_IWUSR, O_IXUSR,
 * 		O_IRWXG, O_IRGRP, O_IWGRP, O_IXGRP,
 * 		O_IRWXO, O_IROTH, O_IWOTH, O_IXOTH,
 * 		
 * return: fd >= 0; fd == -1;
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

static int unix_open(const char *name, int flags, ...)
{
	int fd;

	if (flags & O_CREAT) {
		va_list ap;
		int mode;

		va_start(ap, flags);
		mode = va_arg(ap, int);
		va_end(ap);

		fd = open(name, flags, mode);
	} else {
		fd =open(name, flags);
	}

	return fd;
}

static int test1()
{
	int fd;

	fd = open("1.txt", O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (!(fd < 0)) {
		write(fd, "hello", sizeof("hello"));
		close(fd);
	}

	fd = open("1.txt", O_RDONLY);
	if (!(fd < 0)) {
		char buf[64];
		if (read(fd, buf, sizeof(buf)) > 0)
			printf("%s", buf);
	}
}

static void test2()
{
	int fd = unix_open("1.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (!(fd < 0)) {
		write(fd, "world", sizeof("world"));
		close(fd);
	}
}

void main()
{
	test1();
	test2();
}
