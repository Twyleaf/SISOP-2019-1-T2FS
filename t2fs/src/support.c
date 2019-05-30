#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/t2fs.h"

int t2fsInitiated = 0
short versaoDisco
short tamSetor
short BInicialTabPart
short Nparticoes
int BlocoInicioPart1
int BlocoFimPart

int initT2FS(){
	unsigned char sectorBuffer[SECTOR_SIZE];
	if(read_sector(0, sectorBuffer) != 0)
		return 0;
	versaoDisco = *(short*)sectorBuffer;
	tamSetor = *(short*)(sectorBuffer+2);
	BInicialTabPart = *(short*)(sectorBuffer+4);
	Nparticoes = *(short*)(sectorBuffer+6);
	BlocoInicioPart1 = *(int*)(sectorBuffer+8);
	BlocoFimPart1 = *(int*)(sectorBuffer+12);
	t2fsInitiated=1;
	return 1;
}