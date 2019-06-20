#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"


int main(){
	printf("\nRetorno format2: %d\n",format2(2));
	FILE2 handle1 = create2("/NomeArq");
	printf("Teste create2: %d\n", handle1);
	char stringTest[1000] = "testando";
	int size=1000;
	printf("Teste write2: %d\n", write2(handle1,stringTest,size));
	unsigned char* blockBuffer = createBlockBuffer();
	readBlock(0,blockBuffer);
	DirData dirData;
	dirData =*(DirData*)(blockBuffer+sizeof(unsigned int));
	int block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	DirRecord dirRecord;
	memcpy(&dirRecord,&blockBuffer[block1DataSize],sizeof(DirRecord));
	
	printf("Nome do arquivo no diretorio: %s, tipo: %d, ponteiro: %d, valido: %d, tamanho: %d\n",dirRecord.name,dirRecord.fileType,dirRecord.dataPointer,dirRecord.isValid,dirRecord.fileSize);
	//printf("String no bloco: %s\n",&blockBuffer[block1DataSize]);
	//Arquivo no bloco:
	
	readBlock(1,blockBuffer);
	dirData =*(DirData*)(blockBuffer+sizeof(unsigned int));
	block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	printf("String no bloco: %s\n",&blockBuffer[block1DataSize]);


	
	destroyBuffer(blockBuffer);
	return 0;
}
