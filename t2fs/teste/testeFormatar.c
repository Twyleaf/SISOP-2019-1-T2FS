#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"

#define SECTOR_SIZE 256

int main(){
	int Ret = format2(2);
	//printPartition1DataSection();
	printf("\nRetorno: %d\n",Ret);
	printf("%d\n",getFileBlock("/"));
	printf("Teste mkdir: %d\n", mkdir2("/NomeArq"));
	unsigned char blockBuffer[blockSize];
	readBlock(0,blockBuffer);
	DirData dirData;
	dirData =*(DirData*)(blockBuffer+sizeof(unsigned int));
	int block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	//block1DataSize+filesInDir
	DirRecord dirRecord;
	memcpy(&dirRecord,blockBuffer+44,sizeof(DirRecord));
	/*
	DirRecord testeRecord;
	testeRecord.name="TesteRecord";
	testeRecord.fileType=2;
	testeRecord.filePointer=123;
	memcpy(blockBuffer+44,sizeof(DirRecord));*/

	
	//dirRecord = *(DirRecord*)(blockBuffer+44);
	
	printf("Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",dirRecord.name,dirRecord.fileType,dirRecord.dataPointer);
	/*
	int i;
	for(i=0;i<1000;i++){
		printf("bloco: %d",*/
	return 0;
}
