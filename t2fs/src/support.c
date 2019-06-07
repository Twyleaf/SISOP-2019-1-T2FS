#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
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
int rootDirBlockNumber;

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
	rootDirBlockNumber =0;
	T2FSInitiated=1;
	return 1;
}

int formatFSData(int sectorsPerBlock){
	unsigned char sectorBuffer[SECTOR_SIZE];
	memcpy(sectorBuffer, (char*)&sectorsPerBlock, 2);
	//printf("Foi");
	if(write_sector(firstSectorPartition1,sectorBuffer)!=0){//Escrever o numero de setores por bloco do sistema de arquivos
		return -1;
		//printf("Erro ao escrever os setores por bloco");
	}
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1+1;
	int blocksInPartition=sectorsInPartition/sectorsPerBlock;
	int bytesForBitmap = ceil(blocksInPartition/8.0);
	int sectorByte;
	for(sectorByte=0;sectorByte<SECTOR_SIZE;sectorByte++){
		sectorBuffer[sectorByte]=UCHAR_MAX;
	}
	int sectorsForBitmap = ceil(bytesForBitmap/(float)SECTOR_SIZE);
	int currentSector;
	//printf("Setores usados para o bitmap %d\n",sectorsForBitmap);
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++)
	{
		if(write_sector(firstSectorPartition1+1+currentSector,sectorBuffer)!=0){//Escrever os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;//Setor inicial para os blocos do sistema de arquivos.
	
	return 0;
	
	
}

void printPartition1DataSection(){
	int sector, byte;
	unsigned char sectorBuffer[SECTOR_SIZE];
	for(sector = firstSectorPartition1;sector<blockSectionStart;sector++){
		read_sector(sector,sectorBuffer);
		for(byte=0; byte<SECTOR_SIZE;byte++){
			printf("%x ",sectorBuffer[byte]);
		}
		printf("\n");
	}
	
	
}

int getFileBlock(char *filename){
	char fileCurrentChar = filename[0];
	char dirName[32];
	if(fileCurrentChar!='/'){
		return -1;
	}
	int dirNameStart=1;
	int filenameIndex=0;
	int wordIndex;
	int dirNameIndex=0;
	int fileBlockIndex=rootDirBlockNumber;
	while(fileCurrentChar!= '\0'){
		filenameIndex++;
		fileCurrentChar=filename[filenameIndex];
		if(fileCurrentChar=='/'){
			for(wordIndex=dirNameStart;wordIndex < filenameIndex;wordIndex++){
				dirName[dirNameIndex] = filename[wordIndex];
				dirNameIndex++;
			}
			dirName[dirNameIndex]='\0';
			fileBlockIndex = goToFileFromParentDir(dirName,fileBlockIndex);
			if(fileBlockIndex<0){
				return -1;
			}
		}
	}
	return fileBlockIndex;
}

int goToFileFromParentDir(char* dirName,int parentDirBlockNumber){
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1+1;
	int blocksInPartition=sectorsInPartition/sectorsPerBlock;
	if(parentDirBlockNumber<blocksInPartition){
		return -1;
	}
	int filesInDir=getFileCountFromDir(parentDirBlockNumber);
	int dirOffset = blockPointerSize+ dirDataSize;
	int dirEntryIndex = 0;
	unsigned char blockBuffer[SECTOR_SIZE*sectorsPerBlock];
	int blockToRead = parentDirBlockNumber;
	int hasNextBlock=0;
	dirRecord record;
	unsigned int nextBlockPointer;
	//for(dirEntryIndex=0;dirEntryIndex<filesInDir;dirEntryIndex++)
	do{
		if(readBlock(blockToRead,blockBuffer)!=0){
			return -1;
		}	
		nextBlockPointer = *(unsigned int*)(blockBuffer);
		if(nextBlockPointer!=UINT_MAX){
			hasNextBlock=1;
		}
		while((dirEntryIndex<filesInDir)&&(dirOffset<=sectorsPerBlock*SECTOR_SIZE-dirEntrySize)){
			record = *(t2fs_record*)(blockBuffer+dirOffset);
			if(strcmp(dirName,record.name)==0){
				return record.
			}
			dirEntryIndex++;
			if(dirEntryIndex>=filesInDir){
				return -2;
			}
			dirOffset+=dirEntrySize;
		}
		dirOffset = blockPointerSize+ dirDataSize;
		
	}while(hasNextBlock==1)
	
}


int getDataFromBuffer(char *outputBuffer, int bufferDataStart, int dataSize, char* dataBuffer, int dataBufferSize) {
	if (dataSize > diskSize) {
		return -1;
	}
	int i = 0;
	for (i=0; i < dataSize; i++) {
		buffer[i] = diskBuffer[bufferDataStart + i];
	}
	return 0;
}

int readBlock(int blockNumber, char* data){
	int sectorPos = blockNumber * SECTORS_PER_BLOCK;
	int i, j;
	char sectorBuffer[SECTOR_SIZE];
	for(i = 0; i < sectorsPerBlock; i++){
		if(read_sector(sectorPos + i, sectorBuffer) != 0){
			return -1;
		}
		for(j = 0; j < SECTOR_SIZE; j++){
			data[j + i * SECTOR_SIZE] = sectorBuffer[j];
		}
	}
	return 0;
}