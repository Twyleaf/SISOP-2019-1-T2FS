#include <stdio.h>
#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"

#define SECTOR_SIZE 256

int main(){
	int Ret = format2(2);
	//printPartition1DataSection();
	printf("\nRetorno: %d\n",Ret);
	printf("%d\n",getFileBlock("/"));
	printf("Teste mkdir: %d\n", mkdir2("/teste"));
	/*
	int i;
	for(i=0;i<1000;i++){
		printf("bloco: %d",*/
	return 0;
}
