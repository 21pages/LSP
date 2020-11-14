/**
 * int fflush(FILE *stream);用户缓冲区到内核缓冲区
 * int fsync(int fd); int fdatasync(int fd); 到磁盘
 */
#include <stdio.h>
#include <unistd.h>

void main()
{
	FILE *fp = fopen("1.txt", "w+");
	if (!fp) {
		perror("fopen");
		return;
	}
	fwrite("hello", 6, 1, fp);
	
	#if 0 //乱码
	fflush(fp);
	fsync(fileno(fp));
	#else
	fclose(fp);
	fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}
	#endif	

	char buf[4];
	int ret = fread(buf, sizeof(buf), 1, fp);
	printf("%s\n", buf);
	printf("ret:%d, feof:%d\n", ret, feof(fp));
	
	ret = fread(buf, sizeof(buf), 1, fp);
	printf("%s\n", buf);
	printf("ret:%d, feof:%d\n", ret, feof(fp));

	ret = fread(buf, sizeof(buf), 1, fp);
	printf("%s\n", buf);
	printf("ret:%d, feof:%d\n", ret, feof(fp));

	fclose(fp);
}
/*
hell
ret:1, feof:0
o
ret:0, feof:1
o //need memset buf
ret:0, feof:1
*/