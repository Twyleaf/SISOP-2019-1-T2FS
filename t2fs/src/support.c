#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/apidisk.h"
#include "../include/t2fs.h"

int T2FSInitiated = 0;
short discVersion;
short sectorSize;
short partitionTableFirstByte;
short partitionCount;
int firstSectorPartition1;
int lastSectorPartition1;

int initT2FS(){
	unsigned char sectorBuffer[SECTOR_SIZE];
	if(read_sector(0, sectorBuffer) != 0){
		return 0;
	}
	discVersion= *(short*)sectorBuffer;
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
	if(write_sector(firstSectorPartition1,sectorBuffer)!=0){
		return -1;
	}
	blocksInPartition=blockSize/SECTOR_SIZE;
	bytesForBitmap = ceil(blocksInPartition/8);
	
	
}