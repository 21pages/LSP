/**
 * int fseek(FILE *stream, long offset, int whence);
 * 	SEEK_SET, SEEK_CUR, SEEK_END
 * long ftell(FILE *stream);
 * void rewind(FILE *stream); = fseek(fp, 0, SEEK_SET)
 * int fgetpos(FILE *stream, fpos_t *pos); = ftell
 * int fsetpos(FILE *stream, const fpos_t *pos);
 */

#include <stdio.h>

void main()
{
	FILE *fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}
	fseek(fp, 10, SEEK_SET);
	printf("pos:%ld\n", ftell(fp));
	fpos_t pos;
	fgetpos(fp, &pos);
	printf("pos:%ld\n", (long)pos.__pos);
	pos.__pos = 5;
	fsetpos(fp, &pos);
	printf("pos:%ld\n", ftell(fp));
	rewind(fp);
	printf("pos:%ld\n", ftell(fp));
}
/*
pos:10
pos:10
pos:5
pos:0
*/