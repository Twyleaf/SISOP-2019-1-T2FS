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
	unsigned char blockBuffer[blockSize];
	readBlock(1,blockBuffer);
	DirData dirData;
	dirData =*(DirData*)(blockBuffer+sizeof(unsigned int));
	int block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	//block1DataSize+filesInDir
	DirRecord dirRecord ;
	dirRecord = *(DirRecord*)(blockBuffer+block1DataSize);
	
	printf("Nome do arquivo no diretório: %s",dirRecord.name);
	/*
	int i;
	for(i=0;i<1000;i++){
		printf("bloco: %d",*/
	return 0;
}
