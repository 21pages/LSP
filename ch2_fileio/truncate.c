/**
 * trancate(path, size)
 * ftrancate(fd, size)
 * 截大/截小文件
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static void print_filesize(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (!(fd < 0)) {
        int size = lseek(fd, 0, SEEK_END);
        printf("filesize:%d\n", size);
        close(fd);
    }
}

void main()
{
    const char *file = "1.txt";
    truncate(file, 50);
    print_filesize(file);
    truncate(file,100);
    print_filesize(file);        
}
/*
filesize:50
filesize:100
*/