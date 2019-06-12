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
int blockSize;
int rootDirBlockNumber;
int dirEntrySize;
int blockPointerSize;

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
	dirEntrySize = sizeof(DirRecord);
	blockPointerSize = sizeof(unsigned int);
	T2FSInitiated=1;
	//blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;
	//blockSize=SECTOR_SIZE*sectorsPerBlock;
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
	blockSize=SECTOR_SIZE*sectorsPerBlock;//TO DO: Botar em estrutura do FS
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
	/*
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1+1;
	int blocksInPartition=sectorsInPartition/sectorsPerBlock;
	if(parentDirBlockNumber<blocksInPartition){
		return -1;
	}*/
	int dirOffset = blockPointerSize+ sizeof(DirData);
	int dirEntryIndex = 0;
	unsigned char blockBuffer[blockSize];
	int blockToRead = parentDirBlockNumber;
	int hasNextBlock=0;
	DirRecord record;
	unsigned int nextBlockPointer;
	if(readBlock(blockToRead,blockBuffer)!=0){
		return -1;
	}	
	DirData* dirData=(DirData*)(blockBuffer+4);
	int filesInDir=dirData->entryCount;
	do{
		nextBlockPointer = *(unsigned int*)(blockBuffer);
		if(nextBlockPointer!=UINT_MAX){
			hasNextBlock=1;
			blockToRead = nextBlockPointer;
		}
		while((dirEntryIndex<filesInDir)&&(dirOffset<=blockSize-dirEntrySize)){
			record = *(DirRecord*)(blockBuffer+dirOffset);
			if(strcmp(dirName,record.name)==0){
				return record.dataPointer;
			}
			dirEntryIndex++;
			if(dirEntryIndex>=filesInDir){
				return -2;
			}
			dirOffset+=dirEntrySize;
		}
		dirOffset = blockPointerSize+ dirEntrySize;
		if(hasNextBlock==1){
			if(readBlock(blockToRead,blockBuffer)!=0){
				return -1;
			}	
		}
	}while(hasNextBlock==1);
	return -1;
	
}


int getDataFromBuffer(char *outputBuffer, int bufferDataStart, int dataSize, char* dataBuffer, int dataBufferSize) {
	if (dataSize > dataBufferSize) {
		return -1;
	}
	int i = 0;
	for (i=0; i < dataSize; i++) {
		outputBuffer[i] = dataBuffer[bufferDataStart + i];
	}
	return 0;
}

int readBlock(int blockNumber, unsigned char* data){
	int sectorsPerBlock = blockSize/SECTOR_SIZE;
	int sectorPos = blockNumber * sectorsPerBlock;
	int i, j;
	unsigned char sectorBuffer[SECTOR_SIZE];
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