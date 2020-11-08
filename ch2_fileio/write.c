/**
 * proto: write(fd, buf, cnt)
 * ret: -1 EINTR (EAGAIN when O_NONBLOCK)
 *      [0, cnt) EINTR, (EAGAIN when O_NONBLOCK)
 * 必须做返回值检查, 第二次write断开的socket会产生SIGPIPE导致进程终止
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

static void write_all()
{
    int fd, len ,ret;
    char *buf = "helloworld", *p = buf;

    fd = open("1.txt", O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        perror("open");
        return;
    }
    len = strlen(buf) + 1;
    while(len != 0 && (ret = write(fd, p, len)) != 0) {
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            perror("write");
            break;
        }
        len -= ret;
        p += ret;
    }
}

void main()
{
    write_all();
}