#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"

#define SECTOR_SIZE 256

int main(){
	printf("\nRetorno: %d\n",format2(2));
	printf("%d\n",getFileBlock("/"));
	printf("%d\n",getFileType(0));
	printf("mkdir: %d\n", mkdir2("/Dir1"));
    printf("mkdir: %d\n", mkdir2("/Dir1/Dir2"));
    printf("mkdir: %d\n", mkdir2("/Dir1/Dir2/Dir3"));

	printf("open2 Dir1: %d\n",open2("/Dir1"));
	printf("read2\n");
	read2 (0, NULL, 0);
	printf("\n==================================\n");
    printf("open2 Dir2: %d\n",open2("/Dir1/Dir2"));
	printf("read2\n");
	read2 (1, NULL, 0);
	return 0;
}
