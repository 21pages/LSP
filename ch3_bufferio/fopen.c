/**
 * FILE *fopen(const char *pathname, const char *mode)
 * r:	O_RDONLY
 * r+:	O_RDWR
 * w:	O_WRONLY | O_CREAT | O_TRUNC
 * w+	O_RDWR | O_CREAT | O_TRUNC
 * a:	O_WRONLY | O_CREAT | O_APPEND
 * a+:	O_RDWR | O_CREAT | O_APPEND
 * all 0666
 * 
 * byte stream
 * int fgetc(FILE *stream);
 * int fputc(int c, FILE *stream);
 * 
 * line stream
 * char *fgets(char *s, int size, FILE *stream);
 * int fputs(const char *s, FILE *stream);
 * 
 * binary stream
 * size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 * size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
 */
#include <stdio.h>
#include <string.h>

static void byte_stream_test()
{
	FILE *fp = fopen("1.txt", "w");
	if (!fp) {
		perror("fopen");
		return;
	}

	const char *p = __func__;
	while(*p != '\0') {
		if (fputc(*p++, fp) == EOF) {
			perror("fputc");
			return;
		}
	}

	/* if not close , read nothing*/
	fclose(fp);
	fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}

	int c;
	while ((c = fgetc(fp)) != EOF) {
		if (fputc(c, stdout) == EOF) {
			perror("fputc");
			return;
		}
	}

	fclose(fp);
}

static void line_stream_test()
{
	FILE *fp = fopen("1.txt", "w");
	if (!fp) {
		perror("fopen");
		return;
	}

	/* fputs: NULL will not be stored */
	for (int i = 0; i < 10; i++) {
		const char *p = __func__;
		if (fputs(p, fp) == EOF) {
			perror("fputs");
			return;
		}
		if (i == 4)
			fwrite("a NuLL", strlen("a NuLL") + 1, 1, fp);
		if (i == 5)
			fputs("\n", fp);
	}
	
	fclose(fp);
	fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}

	char buf[1024];
	/* fgets: newline or NULL will be stored */
	/* stop only newline , not NULL*/
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		printf("fgets new line:\n");
		printf("%s", buf);
	}

	fclose(fp);
}

static void binary_stream_test()
{
	struct Man {
		char name[32];
		char age;
	};
	FILE *fp = fopen("1.txt", "w");
	if (!fp) {
		perror("fopen");
		return;
	}

	/* fread, fwrite return item, not size, if sizeof(buf) > datasize, fread return divided value */
	struct Man m = {"zhangsan", 24};
	if (fwrite(&m, sizeof(m), 1, fp) != 1) {
		perror("fwrite");
		return;
	}

	fclose(fp);
	fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}

	memset(&m, 0x00, sizeof(m));
	if (fread(&m, sizeof(m), 1, fp) != 1) {
		perror("fread");
		return;
	}
	printf("m.name:%s\n", m.name);
	printf("m.age:%d\n", m.age);

	fclose(fp);
}

void main()
{
	byte_stream_test();
	printf("\n----------\n");
	line_stream_test();
	printf("\n----------\n");
	binary_stream_test();			
}

/*
byte_stream_test
----------
fgets new line:
line_stream_testline_stream_testline_stream_testline_stream_testline_stream_testa NuLLfgets new line:
line_stream_testline_stream_testline_stream_testline_stream_test
----------
m.name:zhangsan
m.age:24
*/