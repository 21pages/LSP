/**
 * void clearerr(FILE *stream);
 * int feof(FILE *stream);
 * int ferror(FILE *stream);
 * int fileno(FILE *stream);
 */

#include <stdio.h>
#include <string.h>

void main()
{
	FILE *fp = fopen("1.txt", "r");
	if (!fp) {
		perror("fopen");
		return;
	}
	char buf[4];
	clearerr(fp);
	while (fread(buf, sizeof(buf), 1, fp) >= 0) {
		printf("%s, feof:%d\n", buf, feof(fp));
		if (feof(fp)) {
			break;
		}
		memset(buf, 0, sizeof(buf));
	}
}