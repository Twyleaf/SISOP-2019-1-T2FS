#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../include/support.h"
#include "../include/apidisk.h"
#include "../include/bitmap.h"

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
		return -1;
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
	if(readFSInfo() != SUCCEEDED){
		return -1;
	}
	T2FSInitiated=1;
	//blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;
	//blockSize=SECTOR_SIZE*sectorsPerBlock;
	return 1;
}

int readFSInfo(){
	unsigned char sectorBuffer[SECTOR_SIZE];
	if(read_sector(firstSectorPartition1,sectorBuffer) !=0){
		return -1;
	}
	T2FSInfo info = *(T2FSInfo*)(sectorBuffer);
	blockSectionStart = info.blockSectionStart;
	blockSize = (int)info.blockSize;
	printf("[ReadFSInfo] tamanho do bloco: %d\n",blockSize);
	return SUCCEEDED;

}

int formatFSData(int sectorsPerBlock){
	unsigned char sectorBuffer[SECTOR_SIZE];
	int sectorByte;
	for(sectorByte=0;sectorByte<SECTOR_SIZE;sectorByte++){
		sectorBuffer[sectorByte]=UCHAR_MAX;
	}
	blockSize=SECTOR_SIZE*sectorsPerBlock;
	int bytesForBitmap = getBytesForBitmap();
	int sectorsForBitmap = ceil(bytesForBitmap/(float)SECTOR_SIZE);
	int currentSector;
	//printf("Setores usados para o bitmap %d\n",sectorsForBitmap);
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++)
	{
		printf("[formatFSData]Setor %d sendo escrito\n",firstSectorPartition1+1+currentSector);
		if(write_sector(firstSectorPartition1+1+currentSector,sectorBuffer)!=0){//Escrever os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;//Setor inicial para os blocos do sistema de arquivos.

	printf("[formatFSData] tamanho do bloco: %d\n",blockSize);

	//Adicionar dados do sistema de arquivos (no primeiro setor, antes do bitmap)
	T2FSInfo FSInfo;
	FSInfo.blockSectionStart = (unsigned int)blockSectionStart;
	FSInfo.blockSize = (unsigned int)blockSize;
	memcpy(sectorBuffer, (unsigned char*)&FSInfo, sizeof(FSInfo));//Escreve dados de T2FS no disco

	
	if(write_sector(firstSectorPartition1,sectorBuffer)!=0){//FSInfo precisa ser menor que a partição
		return -1;
		//printf("Erro ao escrever os setores por bloco");
	}
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
		printf("[getFileBlock] Erro, path não absoluto\n");
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
				printf("[getFileBlock] Erro, bloco do arquivo filho não achado\n");
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
	if(readBlock(blockToRead,blockBuffer)!=0){//Faz a leitura do primeiro bloco
		return -1;
	}	
	DirData* dirData=(DirData*)(blockBuffer+blockPointerSize);// Salva dados do diretório
	int filesInDir=dirData->entryCount;
	do{//	Itera sobre todos os blocos do diretório
		nextBlockPointer = *(unsigned int*)(blockBuffer);//Calcula ponteiro para próximo bloco
		if(nextBlockPointer!=UINT_MAX){//Coloca próximo bloco para ser lido, se existir
			hasNextBlock=1;
			blockToRead = nextBlockPointer;
		}
		while((dirEntryIndex<filesInDir)&&(dirOffset<=blockSize-dirEntrySize)){//Enquanto não ler todas as entradas e não exceder tamanho do bloco
			record = *(DirRecord*)(blockBuffer+dirOffset);//Lê entrada
			if(strcmp(dirName,record.name)==0){//Se nome é o sendo procurado, retorna número do bloco do arquivo.
				return record.dataPointer;
			}
			dirEntryIndex++;//			Incrementa índice de número de entradas (lidas)
			if(dirEntryIndex>=filesInDir){//Encerra se não houver mais arquivos para ler
				return -2;
			}
			dirOffset+=dirEntrySize;//Adiciona ao offset(espaço entre começo do buffer até entrada a ser lida) o tamanho da entrada já lida.
		}
		dirOffset = blockPointerSize+ dirEntrySize;//Reseta offset para o começo das entradas no bloco
		if(hasNextBlock==1){//		Lê próximo bloco, se houver
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
	int sectorsPerBlock = getSectorsPerBlock(blockSize);
	int sectorPos =blockSectionStart + (blockNumber * sectorsPerBlock);
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

int writeBlock(int blockNumber, unsigned char* data){
	printf("[writeBlock] Escrevendo no bloco %d\n",blockNumber);
	int sectorsPerBlock = getSectorsPerBlock(blockSize);
	int sectorPos = blockSectionStart+(blockNumber * sectorsPerBlock);
	int i, j;
	unsigned char sectorBuffer[SECTOR_SIZE];
	for(i = 0; i < sectorsPerBlock; i++){
		for(j = 0; j < SECTOR_SIZE; j++){
			sectorBuffer[j]=data[j + i * SECTOR_SIZE];
		}
		/*
		DirRecord dirRecord;
		memcpy(&dirRecord,sectorBuffer+44,sizeof(DirRecord));
		printf("[writeBlock]Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",
			dirRecord.name,dirRecord.fileType,dirRecord.dataPointer);
			*/
		printf("[writeBlock]Setor %d sendo escrito\n",sectorPos + i);
		if(write_sector(sectorPos + i, sectorBuffer) != 0){
			return -1;
		}
		
	}
	return 0;
}

int getFileNameAndPath(char *pathname, char *path, char *name){
	//char path[MAX_FILE_NAME_SIZE+1];
	//char name[32];
	int pathIndex=0;
	int lastNameStart;
	while(pathname[pathIndex]!='\0'){
		path[pathIndex]=pathname[pathIndex];
		if(pathname[pathIndex]=='/'){
			lastNameStart = pathIndex+1;
		}
		pathIndex++;
	}
	if(lastNameStart==pathIndex)
		return -1;
	if(lastNameStart==1){
		path[lastNameStart]='\0';
	}else{
		path[lastNameStart-1]='\0';
	}
	int lastNameIndex=lastNameStart;
	int nameIndex=0;
	while(pathname[lastNameIndex]!='\0'){
		name[nameIndex]=pathname[lastNameIndex];
		nameIndex++;
		lastNameIndex++;
	}
	name[nameIndex]='\0';
	printf("[getFileNameAndPath] Arquivo de pathname %s, path %s e nome %s\n",pathname, path,name);
	return 0;
}

int allocateBlock(){
	int sectorsPerBlock = getSectorsPerBlock(blockSize);
	int bytesForBitmap = getBytesForBitmap(sectorsPerBlock);//Excessão
	int sectorsForBitmap = ceil(bytesForBitmap/(float)SECTOR_SIZE);
	printf("[allocateBlock] Setores para bitmap: %d\n",sectorsForBitmap);
	unsigned char bitmapBuffer[SECTOR_SIZE*sectorsForBitmap];
	int currentSector;
	//printf("Setores usados para o bitmap %d\n",sectorsForBitmap);
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++)
	{
		if(read_sector(firstSectorPartition1+1+currentSector,(bitmapBuffer+(SECTOR_SIZE*currentSector)))!=0){//Ler os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	return setAndReturnBit(bitmapBuffer, 0, bytesForBitmap);
}

int writeDirData(int firstBlockNumber, DirData newDirData){
	unsigned char sectorBuffer[SECTOR_SIZE];
	int sectorToUse = getFirstSectorOfBlock(firstBlockNumber);
	if(read_sector(sectorToUse,sectorBuffer)!=0)
		return -1;
	int dirDataOffset = sizeof(int);//Ponteiro para próximo bloco no começo
	memcpy(sectorBuffer+dirDataOffset, &newDirData, sizeof(DirData));//Escreve dados de T2FS no disco
	printf("[writeDirData]Setor %d sendo escrito\n",sectorToUse);
	if(write_sector(sectorToUse,sectorBuffer)!=0)
		return -1;
	return 0;
}

int insertEntryInDir(int dirFirstBlockNumber,DirRecord newDirEntry){//TO DO: INCREMENTAR COUNT DE DIRETORIOS
	unsigned char sectorBuffer[SECTOR_SIZE];
	int sectorToUse = getFirstSectorOfBlock(dirFirstBlockNumber);
	if(read_sector(sectorToUse,sectorBuffer)!=0){
		printf("[insertEntryInDir]Erro ao ler setor do arquivo de diretório\nSetor: %d\n",sectorToUse);
		return -1;
	}
	DirData targetDirData = *(DirData*)(sectorBuffer+blockPointerSize);
	int filesInDir = targetDirData.entryCount;
	float block1DataSize = sizeof(unsigned int)+sizeof(DirData);
	float maxEntryCountBlock1= floor(block1DataSize/(float)sizeof(DirRecord));
	float maxEntryCountRestBlocks = floor((float)sizeof(unsigned int)/(float)sizeof(DirRecord));
	int blockToWrite=-1;
	int blockToWriteByteOffset=-1;
	if(maxEntryCountBlock1 >filesInDir){//se houver espaço no bloco 1
		//escrever no bloco 1
		blockToWrite=dirFirstBlockNumber;
		blockToWriteByteOffset=block1DataSize+(filesInDir*sizeof(DirRecord));
		printf("[insertEntryInDir] Entrada vai ser escrita no primeiro bloco, com offset %d\n",blockToWriteByteOffset);
	}else{
		float blocksNeededAfterFirst = ((float)filesInDir-maxEntryCountBlock1)/maxEntryCountRestBlocks;
		int lastBlock =getLastBlockInFile(dirFirstBlockNumber);
		blockToWrite=lastBlock;
		if(ceilf(blocksNeededAfterFirst) == blocksNeededAfterFirst){//Todos os blocos estão cheios, é preciso alocar um novo
			blockToWriteByteOffset = blockPointerSize;
			int nextBlockAddress =allocateBlock();
			writeNextBlockPointer(lastBlock,nextBlockAddress);
		}else{//Diretório usa mais de um bloco.
			int entriesInBlock = filesInDir;
			entriesInBlock-=maxEntryCountBlock1;
			while(entriesInBlock>maxEntryCountRestBlocks){
				entriesInBlock-=maxEntryCountRestBlocks;//Calcula quantas entradas sobram para o último bloco,
			}// contando que todos os blocos anteriores estão completamente cheios.
			
			blockToWriteByteOffset = blockPointerSize+entriesInBlock*sizeof(DirRecord);
		}
	}
	printf("[insertEntryInDir]Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",newDirEntry.name,newDirEntry.fileType,newDirEntry.dataPointer);
	if(writeRecordInBlock(newDirEntry,blockToWrite,blockToWriteByteOffset)==-1){
		printf("[insertEntryInDir]Erro na adição da nova entrada na estrutura de dados do diretório\n");
		return -1;
	}
	return 0;
}

int getFirstSectorOfBlock(int blockNumber){
	printf("Primeiro setor do bloco %d sendo lido\n",blockNumber);
	int sectorsPerBlock =  getSectorsPerBlock(blockSize);
	return blockSectionStart+(blockNumber*sectorsPerBlock);
}

void writeNextBlockPointer(int blockNumber,unsigned int pointer){
	int sector = getFirstSectorOfBlock(blockNumber);
	unsigned char sectorBuffer[SECTOR_SIZE];
	read_sector(sector,sectorBuffer);
	memcpy(sectorBuffer, (unsigned int*)&pointer, sizeof(unsigned int));//Escreve ponteiro
	printf("[writeNextBlockPointer]Setor %d sendo escrito\n",sector);
	write_sector(sector,sectorBuffer);
	
}

int writeRecordInBlock(DirRecord newDirEntry, int blockToWriteEntry, int dirToInsertOffset){
	unsigned char blockBuffer[blockSize];
	printf("[writeRecordInBlock]Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",newDirEntry.name,newDirEntry.fileType,newDirEntry.dataPointer);
	if(readBlock(blockToWriteEntry, blockBuffer)!=0)
		return -1;
	printf("[writeRecordInBlock]Record do diretório de nome %s sendo escrito no bloco %d com offset %d\n",
		newDirEntry.name,blockToWriteEntry,dirToInsertOffset);
	memcpy(blockBuffer+dirToInsertOffset, (const unsigned char*)&newDirEntry, sizeof(newDirEntry));
	if(writeBlock(blockToWriteEntry, blockBuffer)!=0)
		return -1;
	return SUCCEEDED;
}

int getLastBlockInFile(unsigned int fileFirstBlockNumber){
	unsigned int currentFileBlock=fileFirstBlockNumber;
	unsigned int nextBlock=fileFirstBlockNumber;
	unsigned char sectorBuffer[SECTOR_SIZE];
	int currentFileSector;
	while(nextBlock!= UINT_MAX){
		currentFileBlock = nextBlock;
		currentFileSector= getFirstSectorOfBlock(currentFileBlock);
		if(read_sector(currentFileSector,sectorBuffer)!=0)
			return -1;
		nextBlock = *(unsigned int*)(sectorBuffer);
	}
	return currentFileBlock;
}

int getSectorsPerBlock(int blockSizeBytes){
	printf("[getSectorsPerBlock] Tamanho do bloco:%d\n",blockSizeBytes);
	return blockSizeBytes/(SECTOR_SIZE);
}

int getBytesForBitmap(){
	int sectorsPerBlock = getSectorsPerBlock(blockSize);
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1+1;
	float blocksInPartition= floor(sectorsInPartition/sectorsPerBlock);
	int bytesForBitmap = ceil(blocksInPartition/8.0);
	printf("[getBytesForBitmap] Bytes para bitmap: %d\n", bytesForBitmap);
	return bytesForBitmap;
}

