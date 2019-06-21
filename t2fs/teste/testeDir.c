#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"

#define SECTOR_SIZE 256

int main(){
	printf("\nRetorno: %d\n",format2(2));
	printf("mkdir: %d\n", mkdir2("/Dir1"));
    printf("mkdir: %d\n", mkdir2("/Dir1/Dir2"));
    
    create2("/Dir1/arq1");
    create2("/Dir1/arq11");
    create2("/Dir1/Dir2/arq2");

	DIR2 handle1 = opendir2("/Dir1");
    DIR2 handle2 = opendir2("/Dir1/Dir2");
    
    DIRENT2 entrada1;
    readdir2(handle1, &entrada1);

    printf("\n\n\n====================================\n");
    printf("Nome arquivo 1: %s\n", entrada1.name);
    printf("Tipo arquivo 1: %d\n", entrada1.fileType);
    printf("Tamanho arquivo 1: %d\n", entrada1.fileSize);

    DIRENT2 entrada2;
    readdir2(handle2, &entrada2);

    printf("\n\n\n====================================\n");
    printf("Nome arquivo 2: %s\n", entrada2.name);
    printf("Tipo arquivo 2: %d\n", entrada2.fileType);
    printf("Tamanho arquivo 2: %d\n", entrada2.fileSize);

	return 0;
}
