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
	/*printf("Teste mkdir: %d\n", mkdir2("/NomeArq"));
	unsigned char* blockBuffer = createBlockBuffer();
	readBlock(0,blockBuffer);
	DirData dirData;
	dirData =*(DirData*)(blockBuffer+sizeof(unsigned int));
	int block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	DirRecord dirRecord;
	memcpy(&dirRecord,&blockBuffer[44],sizeof(DirRecord));
	
	printf("Nome do arquivo no diretorio: %s, tipo: %d, ponteiro: %d\n",dirRecord.name,dirRecord.fileType,dirRecord.dataPointer);

	printf("Testando delete2\n");
	printf("bloco do arquivo NomeArq: %d\n",getFileBlock("/NomeArq"));
	printf("Retorno delete2: %d\n", delete2("/NomeArq"));
	printf("bloco do arquivo NomeArq: %d\n\n",getFileBlock("/NomeArq"));*/
	return 0;
}
