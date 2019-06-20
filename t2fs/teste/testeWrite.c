#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"


int main(){
	printf("\nRetorno: %d\n",format2(2));
	FILE2 handle = create2("/arquivo");
	printf("\nprimeiro bloco %d", open_files[(int)handle].fileRecord.dataPointer);
	printf("\n\n=============================================================\n");
	printf("\n\n=============================================================\n");
	unsigned char* cu = (unsigned char*)malloc(sizeof(unsigned char)*10);
	cu[1] = 1;
	cu[2] = 2;
	cu[3] = 3;
	read2(handle, cu, 10);

	printf("\n\n==========================================================\n");

	int i = 0;
	for(i = 0; i<10; i++){
		printf("%d", cu[i]);
	}

	return 0;
}
