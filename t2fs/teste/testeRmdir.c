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

    printf("rmdir: %d\n",rmdir2("/Dir1/Dir2"));
	return 0;
}
