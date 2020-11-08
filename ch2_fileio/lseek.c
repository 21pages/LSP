/**
 * lseek(fd, oft, pos)
 * oft:+-0
 * pos:SEEK_SET, SEEK_CUR, SEEK_END
 * ret:current position
 * 
 * pread(fd, buf, cnt, pos)
 * pwrite(fd, buf,cnt ,pos)
 * pread, pwrite 不修改文件当前读写指针
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void print_pos(int fd)
{
    off_t pos;

    pos = lseek(fd, 0, SEEK_CUR);
    printf("pos:%ld\n", pos);
}

static void test1()
{
    int fd;
    char *buf1 = "0123456789";
    char buf2[32];

    fd = open("1.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    write(fd, buf1, strlen(buf1) + 1);
    print_pos(fd);
    
    lseek(fd, 5, SEEK_SET);
    print_pos(fd);

    pread(fd, buf2, sizeof(buf2), 0);
    print_pos(fd);

    pwrite(fd, "hello", strlen("hello"), 0);
    print_pos(fd);
}

int main()
{
    test1();
}
/*
pos:11
pos:5
pos:5
pos:5
*/