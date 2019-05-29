#include <stdio.h>
#include "../include/apidisk.h"

#define SECTOR_SIZE 256

int main(){
	unsigned char sectorBuffer[SECTOR_SIZE];
	int j;
	if(read_sector(0, sectorBuffer) == 0)
		printf("foi\n");
	/*
	for(j = 0; j < SECTOR_SIZE; j++){
		printf("%x ",sectorBuffer[j]);
	} //imprime os bytes invertidos*/
	//printf("%s", sectorBuffer);
	short* versaoDisco = (short*)sectorBuffer;
	printf("Versao do disco: %x Decimal:%d\n", *versaoDisco,*versaoDisco);
	short* tamSetor = (short*)(sectorBuffer+2);
	printf("Tamanho do setor: %x Decimal:%d\n", *tamSetor,*tamSetor);
	short* BInicialTabPart = (short*)(sectorBuffer+4);
	printf("Byte inicial da tabela de particoes: %x Decimal:%d\n", *BInicialTabPart,*BInicialTabPart);
	short* Nparticoes = (short*)(sectorBuffer+6);
	printf("Numero de particoes no disco: %x Decimal:%d\n", *Nparticoes,*Nparticoes);
	int* BlocoInicioPart1 = (int*)(sectorBuffer+8);
	printf("bloco inicial da particao 1: %x Decimal:%d\n", *BlocoInicioPart1,*BlocoInicioPart1);
	int* BlocoFimPart1 = (int*)(sectorBuffer+12);
	printf("bloco final da particao 1: %x Decimal:%d\n", *BlocoFimPart1,*BlocoFimPart1);

	
	
	
    
}
