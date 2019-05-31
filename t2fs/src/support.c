#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include "../include/support.h"
#include "../include/apidisk.h"

int T2FSInitiated = 0;
short diskVersion;
short sectorSize;
short partitionTableFirstByte;
short partitionCount;
int firstSectorPartition1;
int lastSectorPartition1;
int blockSectionStart;

int initT2FS(){
	unsigned char sectorBuffer[SECTOR_SIZE];
	if(read_sector(0, sectorBuffer) != 0){
		return 0;
	}
	diskVersion= *(short*)sectorBuffer;
	sectorSize = *(short*)(sectorBuffer+2);
	partitionTableFirstByte = *(short*)(sectorBuffer+4);
	partitionCount = *(short*)(sectorBuffer+6);
	firstSectorPartition1 = *(int*)(sectorBuffer+8);
	lastSectorPartition1 = *(int*)(sectorBuffer+12);
	T2FSInitiated=1;
	return 1;
}

int formatFSData(int sectorsPerBlock){
	unsigned char sectorBuffer[SECTOR_SIZE];
	memcpy(sectorBuffer, (char*)&sectorsPerBlock, 2);
	if(write_sector(firstSectorPartition1,sectorBuffer)!=0){//Escrever o numero de setores por bloco do sistema de arquivos
		return -1;
	}
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1;
	int blocksInPartition=sectorsInPartition/sectorsPerBlock;
	int bytesForBitmap = ceil(blocksInPartition/8.0);
	int sectorsForBitmap = ceil(bytesForBitmap/(float)SECTOR_SIZE);
	int sectorByte;
	for(sectorByte=0;sectorByte<SECTOR_SIZE;sectorByte++){
		sectorBuffer[sectorByte]=UCHAR_MAX;
	}
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++)
	{
		if(write_sector(firstSectorPartition1+1+currentSector,sectorBuffer)!=0){//Escrever os bits do bitmap.
			return -1;
		}
	}
	blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;//Setor inicial para os blocos do sistema de arquivos.
	//Blocos do bitmap+ bloco do aaaa
	
	return 1;
	
	
}