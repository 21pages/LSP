/**
 * void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
 *	addr: NULL is good
 *	length: > 0
 *	prot:	(需保证fd打开时也具有相应权限)
 * 		PROT_READ 页面可读
 * 		PROT_WRITE 页面可写
 *		PROT_EXEC 页面可执行
 *		PROT_NONE 无法访问
 *	flags:
 * 		MAP_SHARED: shared mapping. write will real write
 * 		MAP_PRIVATE: private mapping. write will not write real file
 * 		MAP_FIXED: addr is fixed, not adviced
 * 	fd:	which file
 * 	offset:	the start of map in file, offset must be a multiple of the page size as  returned by sysconf(_SC_PAGE_SIZE).
 * 	ret:	succ: mapping address; fail: MAP_FAILED
 *
 * int munmap(void *addr, size_t length);
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>

static void mmap_test(int flags, int file_old_size)
{
    int fd = open("1.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }

    /* 文件大小不能为0, 否则mmap不能找到具体的页地址, 写时出现bus error */
    //write(fd, "01234\n", 6);
    ftruncate(fd, file_old_size);

    struct stat statbuf;
    fstat(fd, &statbuf);
    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "not regular file!\n");
        return;
    }

    /* 即使映射长度大于实际长度, 也只能读写实际长度 */
    //long page_size = sysconf(_SC_PAGE_SIZE);
    char *addr = mmap(NULL, file_old_size, PROT_READ | PROT_WRITE, flags, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return;
    }
    sprintf(addr, "%s", "hello world");
    munmap(addr, file_old_size);

    close(fd);

    fd = open("1.txt", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return;
    }

    fstat(fd, &statbuf);
    addr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        return;
    }
    printf("file content:");
    for (int i = 0; i < statbuf.st_size; i++)
        printf("%c", addr[i]);
    printf("\n");
    munmap(addr, statbuf.st_size);
    close(fd);
}


void main()
{
    printf("shared:\n");
    mmap_test(MAP_SHARED, 8);
    mmap_test(MAP_SHARED, 128);
    printf("private:\n");
    mmap_test(MAP_PRIVATE, 8);
    mmap_test(MAP_PRIVATE, 128);
}

/*
shared:
file content:hello wo
file content:hello world
private:
file content:
file content:
*/
