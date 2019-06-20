#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "../include/support.h"
#include "../include/apidisk.h"
#include "../include/bitmap.h"

//=========================================NOVO=====================================
#define VERBOSE_DEBUG
//para desativar os prints de debug, basta comentar esta linha
//=====================================================================================

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


/*
*	Cria em memória um buffer para armazenar um setor do disco
*	Retorna um ponteiro para este buffer
*/
unsigned char* createSectorBuffer(){
	return (unsigned char*)malloc(SECTOR_SIZE);
}


/*
*	Cria em memória um buffer para armazenar um bloco inteiro
*	Retorna um ponteiro para este buffer
*/
unsigned char* createBlockBuffer(){
	return (unsigned char*)malloc(sizeof(unsigned char)*blockSize);
}


/*
*	Cria um buffer em memória para armazenar o bitmap inteiro
*	Retorna um ponteiro para este buffer
*/
unsigned char* createBitmapBuffer(){
	int bitmapSizeInBytes = ceil(getBytesForBitmap()/(float)SECTOR_SIZE)*SECTOR_SIZE;
	unsigned char* bitmap = (unsigned char*)malloc(sizeof(unsigned char)*bitmapSizeInBytes);
	return bitmap;
}


/*
*	Dado um ponteiro para uma área de memória contendo um buffer criado com uma das funções acima, libera ela.
*/
void destroyBuffer(unsigned char* buffer){
	free(buffer);
}


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

	/*Inicialização do array de arquivos abertos */
	int currentIndex = 0;
	OpenFileData emptyFile;
	emptyFile.isValid = false;
	for(currentIndex = 0; currentIndex < 10; currentIndex++) open_files[currentIndex] = emptyFile;
	
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
	//printf("[ReadFSInfo] tamanho do bloco: %d\n",blockSize);
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
		#ifdef VERBOSE_DEBUG
			printf("[formatFSData]Setor %d sendo escrito\n",firstSectorPartition1+1+currentSector);
		#endif
		if(write_sector(firstSectorPartition1+1+currentSector,sectorBuffer)!=0){//Escrever os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	blockSectionStart = sectorsForBitmap + firstSectorPartition1 + 1;//Setor inicial para os blocos do sistema de arquivos.

	//printf("[formatFSData] tamanho do bloco: %d\n",blockSize);

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
		#ifdef VERBOSE_DEBUG
		printf("[getFileBlock] Erro, path não absoluto\n");
		#endif
		return -1;
	}
	int dirNameStart=1;
	int filenameIndex=0;
	int wordIndex;
	int dirNameIndex=0;
	int fileBlockIndex=rootDirBlockNumber;
	while(filename[filenameIndex]!= '\0'){
		filenameIndex++;
		fileCurrentChar=filename[filenameIndex];
		if((filename[filenameIndex]=='/')||(filename[filenameIndex+1]=='\0')){
			if(filename[filenameIndex+1]=='\0')
				filenameIndex++;
			for(wordIndex=dirNameStart;wordIndex < filenameIndex;wordIndex++){
				dirName[dirNameIndex] = filename[wordIndex];
				dirNameIndex++;
			}
			dirName[dirNameIndex]='\0';
			
			if(getFileType(fileBlockIndex)!=0x02){//Se o diretório anterior não era realmente um arquivo de diretório
				#ifdef VERBOSE_DEBUG
				printf("[getFileBlock] Erro: arquivo em path não diretório\n");
				#endif
				return -1;
			}
			fileBlockIndex = goToFileFromParentDir(dirName,fileBlockIndex);
			
			#ifdef VERBOSE_DEBUG
			printf("[getFileBlock] Bloco do arquivo durante iteração: %d\n",fileBlockIndex);
			#endif
			if(fileBlockIndex<0){
				#ifdef VERBOSE_DEBUG
				printf("[getFileBlock] Erro, bloco do arquivo filho não achado\n");
				#endif
				return -1;
			}
			dirNameIndex=0;
			dirNameStart = filenameIndex+1;
		}
	}
	return fileBlockIndex;
}

int goToFileFromParentDir(char* dirName,int parentDirBlockNumber){
	int dirOffset = blockPointerSize+ sizeof(DirData);
	int dirEntryIndex = 0;
	unsigned char* blockBuffer = malloc(sizeof(unsigned char)*blockSize); 
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
			if(record.isValid){//se entrada é válida
				printf("[goToFileFromParentDir]Comparação de nome %s VS %s Na entrada com offset %d\n",dirName,record.name,dirOffset);
				if(strcmp(dirName,record.name)==0){//Se nome é o sendo procurado, retorna número do bloco do arquivo.
					return record.dataPointer;
				}
				dirEntryIndex++;//			Incrementa índice de número de entradas (lidas)
			}
			if(dirEntryIndex>=filesInDir){//Encerra se não houver mais arquivos para ler
				return -2;
			}
			dirOffset+=dirEntrySize;//Adiciona ao offset(espaço entre começo do buffer até entrada a ser lida) o tamanho da entrada já lida.
		}
		dirOffset = blockPointerSize;//Reseta offset para o começo das entradas no bloco
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
	unsigned char* bitmapBuffer = (unsigned char*)malloc(sizeof(unsigned char)*SECTOR_SIZE*sectorsForBitmap);
	int currentSector;
	//printf("Setores usados para o bitmap %d\n",sectorsForBitmap);
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++){
		if(read_sector(firstSectorPartition1+1+currentSector,(bitmapBuffer+(SECTOR_SIZE*currentSector)))!=0){//Ler os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	int blockAddress= setAndReturnBit(bitmapBuffer, 0, bytesForBitmap);
	for(currentSector=0;currentSector<sectorsForBitmap;currentSector++){
		if(write_sector(firstSectorPartition1+1+currentSector,(bitmapBuffer+(SECTOR_SIZE*currentSector)))!=0){//Ler os bits do bitmap.
			return -1;
			//printf("Erro ao escrever o bitmap");
		}
	}
	return blockAddress;
}

int allocateRootDirBlock(){
	unsigned char bitmapFirstSectorBuffer[SECTOR_SIZE];
	//printf("Setores usados para o bitmap %d\n",sectorsForBitmap);

	if(read_sector(firstSectorPartition1+1,bitmapFirstSectorBuffer)!=0){//Ler os bits do bitmap.
		return -1;
	}
	clearBit(bitmapFirstSectorBuffer, 0);
	if(write_sector(firstSectorPartition1+1,bitmapFirstSectorBuffer)!=0){//escrever os bits do bitmap.
		return -1;
	}
	return 0;
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


/*
*	Dados uma DirData representando a versão atualizada do diretório e o número do primeiro setor do diretório,
*	atualiza as informações do diretório em disco
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
int updateDirectory(DirData directory, int dirFirstBlockNumber){
	/*Sobrescrevemos as informações do diretório no bloco dirFirstBlockNumber com a estrutura directory */
	#ifdef VERBOSE_DEBUG
		printf("[updateDirectory]Bloco %d sendo escrito\n",dirFirstBlockNumber);
	#endif


	/*Lemos o bloco para um buffer em memória */
	unsigned char* blockBuffer = createBlockBuffer();
	if(readBlock(dirFirstBlockNumber, blockBuffer) != 0){
		destroyBuffer(blockBuffer);
		return -1;
	}

	/*Sobrscrevemos as informações */
	int nextBlockPointerOffset = sizeof(unsigned int);
	memcpy(&blockBuffer[nextBlockPointerOffset], &directory, sizeof(DirData));

	/*Escrevemos de volta no disco */
	if(writeBlock(dirFirstBlockNumber, blockBuffer) != 0){
		destroyBuffer(blockBuffer);
		return -1;
	}

	return 0;
}


/*
*	Dados o primeiro bloco de um diretório e um ponteiro para uma entrada de diretório,
*	Adiciona esta entrada ao diretório
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
int insertEntryInDir(int directoryFirstBlockNumber,DirRecord newDirEntry){
	/*
	*	Itera pelos blocos do diretório procurando alguma DirRecord com isValid = false
	*	Se encontrar, apenas sobrescreve ela com newDirEntry
	*	Se não encontrar, escreve newDirEntry logo após a última entrada do diretório, 
	*	incrementa o contador de entradas e atualiza as informações do diretório em disco
	*/

	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir]Iniciando\n");
	#endif

	/*O primeiro bloco de um diretório tem um ponteiro para o próximo e a estrutura de informações do diretório
	* Apenas após estes dados que as entradas de fato começam, então calculamos esse offset para poder
	* calcular quantas entradas de diretório cabem no primeiro bloco*/
	int firstBlockFirstEntryOffset = sizeof(unsigned int)+sizeof(DirData);
	int firstBlockEntryRegionSize = blockSize - firstBlockFirstEntryOffset;
	int maxEntryCountFirstBlock= floor(firstBlockEntryRegionSize/(float)sizeof(DirRecord));

	/*Quanto aos outros blocos, basta fazer o mesmo cálculo considerando apenas o ponteiro para o próximo
	* bloco como offset*/
	int otherBlocksFirstEntryOffset = sizeof(unsigned int);
	int otherBlocksEntryRegionSize = blockSize - otherBlocksFirstEntryOffset;
	int maxEntryCountOtherBlocks = floor(otherBlocksEntryRegionSize/(float)sizeof(DirRecord));
	
	/*Lemos o primeiro bloco do diretório para recuperar as informações correspondentes a ele 
	* e o ponteiro para o próximo bloco*/
	unsigned char* blockBuffer = createBlockBuffer();
	if(readBlock(directoryFirstBlockNumber,blockBuffer)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir]Erro ao ler o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
		#endif
		destroyBuffer(blockBuffer); //sempre que a função for retornar, temos que garantir que a área que alocamos para o buffer será liberada
		return -1;
	}
	unsigned int nextBlockPointer = *(unsigned int*)blockBuffer;	//ponteiro para o próximo bloco, se tiver
	DirData directoryInfo = *(DirData*)(&blockBuffer[blockPointerSize]);//struct contendo informações sobre o diretório
	int numEntriesInDirectory = directoryInfo.entryCount;//número de entradas neste diretório
	
	int directorySizeInBlocks = ceil((sizeof(DirData)+numEntriesInDirectory*sizeof(DirRecord))/blockSize);

	/*Iteramos pelas entradas no primeiro bloco procurando por uma entrada inválida*/
	int currentEntryIndex = 0;
	int entrySize = sizeof(DirRecord);
	DirRecord currentEntry;
	int numEntriesInFirstBlock;
	int currentEntryOffset;
	/*Se o número total de entradas neste diretório for menor que o número de entradas que cabem em um bloco,
	* significa que o número de entradas no primeiro bloco será o número total de entradas, visto que tem espaço sobrando
	  Caso contrário, o número de entradas no primeiro bloco será necessariamente o máximo que cabe nele*/
	if(maxEntryCountFirstBlock > numEntriesInDirectory) numEntriesInFirstBlock = numEntriesInDirectory;
	else numEntriesInFirstBlock = maxEntryCountFirstBlock;

	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir]Iterando sobre as entradas do primeiro bloco\n");
	#endif

	for(currentEntryIndex = 0; currentEntryIndex < numEntriesInFirstBlock; currentEntryIndex++){
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*currentEntryIndex;
		currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
		if(!currentEntry.isValid){
			/*Encontramos uma entrada cujo arquivo foi deletado, então podemos sobrescrever ela com a nova*/
			memcpy(&blockBuffer[currentEntryOffset], &newDirEntry, sizeof(DirRecord));
			
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Entrada vai ser escrita no primeiro bloco, com offset %d\n",currentEntryOffset);
			#endif

			/*Pronto, agora o retorno da função só depende da execução de writeBlock */
			if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
				#ifdef VERBOSE_DEBUG
					printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",directoryFirstBlockNumber);
				#endif
				destroyBuffer(blockBuffer);
				return -1;
			} else{
				destroyBuffer(blockBuffer);
				return 0;
			}
		}
	}

	/*Se a função chegou até aqui, significa que nenhuma das entradas do primeiro bloco estava inválida,
	então nos resta checar se tem espaço sobrando no primeiro bloco*/
	if(maxEntryCountFirstBlock >numEntriesInDirectory){
		/*Tem espaço sobrando no primeiro bloco, então vamos escrever no fim dele e atualizar a
		* contagem de entradas*/
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*numEntriesInDirectory;
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Entrada vai ser escrita no primeiro bloco, com offset %d\n",currentEntryOffset);
		#endif
		memcpy(&blockBuffer[currentEntryOffset], &newDirEntry, sizeof(DirRecord));
		if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",directoryFirstBlockNumber);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}
		directoryInfo.entryCount = directoryInfo.entryCount+1;

		/*Pronto, agora é só atualizar o diretório e a função já pode retornar*/
		if(updateDirectory(directoryInfo, directoryFirstBlockNumber)!= 0){
			destroyBuffer(blockBuffer);
			return -1;
		} else{
			destroyBuffer(blockBuffer);
			return 0;
		}
	}
	
	
	/*Se a função chegou até aqui, significa que não foi possivel escrever no primeiro bloco, 
	* então vamos ter que checar se este diretório ocupa mais blocos.
	* Se não ocupar mais blocos, aloca um novo bloco para o diretório e escreve a nova entrada lá
	* Se ocupar mais blocos, itera sobre eles tentando encontrar uma entrada inválida para sobrescrever ou espaço sobrando para escrever
	* Se for até o final e não conseguir escrever, aloca um novo bloco para o diretório e escreve a nova entrada lá
	*/
	
	if(directorySizeInBlocks == 1){
		/*Se o diretório ocupar apenas um bloco, vamos alocar um bloco novo e escrever a entrada lá*/
		unsigned int newBlockNumber = allocateBlock();
		
	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir]Novo bloco sera alocado para o diretorio\n");
	#endif

		if(newBlockNumber == -1){ /*Houve um erro na função de alocação*/
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar alocar novo bloco\n");
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Entrada vai ser escrita em um novo bloco, de numero %d\n",newBlockNumber);
		#endif

		/* blockBuffer aqui armazena o conteúdo do primeiro bloco, então
		*  escrevemos nele o ponteiro para o novo bloco e então escrevemos ele no disco
		*/
		memcpy(blockBuffer, &newBlockNumber, sizeof(unsigned int)); 													
		if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",directoryFirstBlockNumber);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/* Agora vamos ler o novo bloco, escrever a nova entrada nele 
		*  e então escrever ele no disco
		*/
		if(readBlock(newBlockNumber, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar ler o bloco %d\n",newBlockNumber);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}
		memcpy(&blockBuffer[otherBlocksFirstEntryOffset], &newDirEntry, sizeof(DirRecord));
		if(writeBlock(newBlockNumber, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",newBlockNumber);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/*Agora precisamos incrementar o contador de entradas do diretório, pois criamos uma nova*/
		directoryInfo.entryCount = directoryInfo.entryCount+1;

		/*Pronto, agora é só atualizar o diretório e a função já pode retornar*/
		if(updateDirectory(directoryInfo, directoryFirstBlockNumber)!= 0){
			destroyBuffer(blockBuffer);
			return -1;
		} else{
			destroyBuffer(blockBuffer);
			return 0;
		}
	}


	/*Se a função chegou até aqui, significa que o diretório ocupa mais de um bloco, então vamos iterar
	* Sobre todos os blocos ocupados por este diretório até achar uma entrada inválida ou espaço sobrando
	* Se não encontrarmos, alocamos um novo bloco e escrevemos a nova entrada lá*/
	
	//lembrando que nextBlockPointer já nos diz o número do segundo bloco deste diretório 
	//(recuperamos esta informação lá no início da função)

	/*Número de entradas do diretório sobre as quais ainda falta iterar,
	* usamos esse valor para checar se tem espaço sobrando em algum bloco
	* e para calcular quantas entradas tem um dado bloco*/
	int entriesRemaining = numEntriesInDirectory - maxEntryCountFirstBlock;

	/*Número de entradas em um dado bloco, usamos esse valor para iterar
	* sobre as entradas de cada bloco */
	int numEntriesInCurrentBlock; 
	unsigned int currentBlockNumber = 1; //começa em 1 pois já analisamos o primeiro bloco

	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir]Iniciando a iterar sobre os outros blocos do diretorio\n");
	#endif

	for(currentBlockNumber = 1; currentBlockNumber < directorySizeInBlocks; currentBlockNumber++){
		
		if(readBlock(nextBlockPointer, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar ler o bloco %d\n",nextBlockPointer);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/*O primeiro passo é iterar pelas entradas de diretório que estão neste bloco
		* procurando por alguma inválida para sobrescrever*/

		/*Primeiramente calculamos quantas entradas existem no bloco atual.
		* Se o número de entradas sobre as quais falta iterar for menor que o máximo que este bloco
		* suporta, então o número de entradas neste bloco é exatamente o número de entradas sobre as quais falta iterar
		* Caso contrário, este bloco necessariamente terá o número máximo de entradas */
		if(maxEntryCountOtherBlocks > entriesRemaining) numEntriesInCurrentBlock = entriesRemaining;
		else numEntriesInCurrentBlock = maxEntryCountOtherBlocks;

		/*Agora iteramos sobre todas as entradas deste bloco procurando uma inválida para sobrescrever*/
		for(currentEntryIndex = 0; currentEntryIndex<numEntriesInCurrentBlock; currentEntryIndex++){
			
			currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*currentEntryIndex;
			
			currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
			if(!currentEntry.isValid){
				/*Encontramos uma entrada cujo arquivo foi deletado, então podemos sobrescrever ela com a nova*/
				memcpy(&blockBuffer[currentEntryOffset], &newDirEntry, sizeof(DirRecord));
				
				#ifdef VERBOSE_DEBUG
					printf("[insertEntryInDir] Entrada vai ser escrita no %d-esimo bloco, com offset %d\n",currentBlockNumber,currentEntryOffset);
				#endif

				/*Pronto, agora o retorno da função só depende da execução de writeBlock */
				if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",directoryFirstBlockNumber);
					#endif
					destroyBuffer(blockBuffer);
					return -1;
				} else{
					destroyBuffer(blockBuffer);
					return 0;
				}
			}
		}

		/*Se a função chegou aqui, significa que nenhuma das entradas do bloco atual está inválida, então
		* nos resta checar se tem espaço sobrando neste bloco */
		if(maxEntryCountOtherBlocks > entriesRemaining){
			/*Tem espaço sobrando, então vamos escrever no final deste bloco e incrementar o contador de entradas*/
			currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*entriesRemaining;
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Entrada vai ser escrita no %d-esimo bloco, com offset %d\n",currentBlockNumber,currentEntryOffset);
			#endif

			memcpy(&blockBuffer[currentEntryOffset], &newDirEntry, sizeof(DirRecord));

			if(writeBlock(currentBlockNumber, blockBuffer) != 0){
				#ifdef VERBOSE_DEBUG
					printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",currentBlockNumber);
				#endif
				destroyBuffer(blockBuffer);
				return -1;
			}
			directoryInfo.entryCount = directoryInfo.entryCount+1;

			/*Pronto, agora é só atualizar o diretório e a função já pode retornar*/
			if(updateDirectory(directoryInfo, directoryFirstBlockNumber)!= 0){
				destroyBuffer(blockBuffer);
				return -1;
			} else{
				destroyBuffer(blockBuffer);
				return 0;
			}
		}

		/*Se a função chegou aqui, significa que não conseguimos escrever no bloco atual,
		* então pegamos o ponteiro para o próximo, atualizamos o número de entradas faltando
		*  e seguimos iterando (ou saímos do loop, se for o último bloco)*/

		/*Se alcançarmos o último bloco, não queremos perder seu número, pois vamos escrever nele o ponteiro
		para o bloco que será alocado. Então, se chegarmos ao fim, apenas saímos do loop*/
		if(*(unsigned int*)blockBuffer == UINT_MAX) break;

		/*Caso contrário, pegamos os dados necessários e seguimos iterando*/
		entriesRemaining = entriesRemaining - maxEntryCountOtherBlocks; //o bloco atual necessariamente está cheio
		nextBlockPointer = *(unsigned int*)blockBuffer;
	}

	/*Se a função chegou até aqui, significa que não conseguimos escrever em nenhum dos blocos deste diretório,
	* então a única opção é alocar um novo bloco e escrever a nova entrada lá.
	* Felizmente,  temos o número do último bloco em nextBlockPointer e o conteúdo dele em blockBuffer*/
	int lastBlockNumber = nextBlockPointer;

	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir]Um novo bloco sera alocado para a nova entrada\n");
	#endif

	/*Alocamos um novo bloco para o diretório*/
	unsigned int newBlockNumber = allocateBlock();
	
	if(newBlockNumber == -1){ /*Houve um erro na função de alocação*/
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Erro ao tentar alocar novo bloco\n");
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	#ifdef VERBOSE_DEBUG
		printf("[insertEntryInDir] Entrada vai ser escrita em um novo bloco, de numero %d\n",newBlockNumber);
	#endif

	/* blockBuffer aqui armazena o conteúdo do último bloco, então
	*  escrevemos nele o ponteiro para o novo bloco e então escrevemos ele no disco
	*/
	memcpy(blockBuffer, &newBlockNumber, sizeof(unsigned int)); 													
	if(writeBlock(lastBlockNumber, blockBuffer) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",lastBlockNumber);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	/* Agora vamos ler o novo bloco, escrever a nova entrada nele 
	*  e então escrever ele no disco
	*/
	if(readBlock(newBlockNumber, blockBuffer) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Erro ao tentar ler o bloco %d\n",newBlockNumber);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}
	memcpy(&blockBuffer[otherBlocksFirstEntryOffset], &newDirEntry, sizeof(DirRecord));
	if(writeBlock(newBlockNumber, blockBuffer) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[insertEntryInDir] Erro ao tentar escrever no bloco %d\n",newBlockNumber);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	/*Agora precisamos incrementar o contador de entradas do diretório, pois criamos uma nova*/
	directoryInfo.entryCount = directoryInfo.entryCount+1;

	/*Pronto, agora é só atualizar o diretório e a função já pode retornar*/
	if(updateDirectory(directoryInfo, directoryFirstBlockNumber)!= 0){
		destroyBuffer(blockBuffer);
		return -1;
	} else{
		destroyBuffer(blockBuffer);
		return 0;
	}

	/*Se a função chegou até aqui, alguma coisa deu muito errado, então vamos parar o programa e 
	* lançar um código de erro só por precaução*/
	exit(-1);
}

int getFirstSectorOfBlock(int blockNumber){
	//printf("[getFirstSectorOfBlock]Primeiro setor do bloco %d sendo lido\n",blockNumber);
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
	unsigned char* blockBuffer = createBlockBuffer();
	printf("[writeRecordInBlock]Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",newDirEntry.name,newDirEntry.fileType,newDirEntry.dataPointer);
	if(readBlock(blockToWriteEntry, blockBuffer)!=0)
		return -1;
	printf("[writeRecordInBlock]Record do diretório de nome %s sendo escrito no bloco %d com offset %d\n",
		newDirEntry.name,blockToWriteEntry,dirToInsertOffset);
	memcpy(&blockBuffer[dirToInsertOffset], (const unsigned char*)&newDirEntry, sizeof(newDirEntry));
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
	//printf("[getSectorsPerBlock] Tamanho do bloco:%d\n",blockSizeBytes);
	return blockSizeBytes/(SECTOR_SIZE);
}

int getBytesForBitmap(){
	int sectorsPerBlock = getSectorsPerBlock(blockSize);
	int sectorsInPartition=lastSectorPartition1-firstSectorPartition1+1;
	float blocksInPartition= floor(sectorsInPartition/sectorsPerBlock);
	int bytesForBitmap = ceil(blocksInPartition/8.0);
	//printf("[getBytesForBitmap] Bytes para bitmap: %d\n", bytesForBitmap);
	return bytesForBitmap;
}


/*
*	Dados o primeiro bloco de um diretório e um nome de arquivo, percorre o diretório procurando o arquivo.
*	Quando encontrar, modifica o bit de validade de sua entrada, fazendo com que ela possa ser sobrescrita futuramente
*	(Esta função faz parte do processamento de delete2)
*	Retorna 0 se executou com sucesso e -1 caso contrário
*/
int SetDirectoryEntryAsInvalid(unsigned int directoryFirstBlockNumber, char* filename){

	/*O primeiro bloco de um diretório tem um ponteiro para o próximo e a estrutura de informações do diretório
	* Apenas após estes dados que as entradas de fato começam, então calculamos esse offset para poder
	* calcular quantas entradas de diretório cabem no primeiro bloco*/
	int firstBlockFirstEntryOffset = sizeof(unsigned int)+sizeof(DirData);
	int firstBlockEntryRegionSize = blockSize - firstBlockFirstEntryOffset;
	int maxEntryCountFirstBlock= floor(firstBlockEntryRegionSize/(float)sizeof(DirRecord));

	/*Quanto aos outros blocos, basta fazer o mesmo cálculo considerando apenas o ponteiro para o próximo
	* bloco como offset*/
	int otherBlocksFirstEntryOffset = sizeof(unsigned int);
	int otherBlocksEntryRegionSize = blockSize - otherBlocksFirstEntryOffset;
	int maxEntryCountOtherBlocks = floor(otherBlocksEntryRegionSize/(float)sizeof(DirRecord));
	
	/*Lemos o primeiro bloco do diretório para recuperar as informações correspondentes a ele 
	* e o ponteiro para o próximo bloco*/
	unsigned char* blockBuffer = createBlockBuffer();
	if(readBlock(directoryFirstBlockNumber,blockBuffer)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[SetDirectoryEntryAsInvalid]Erro ao ler o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
		#endif
		destroyBuffer(blockBuffer); //sempre que a função for retornar, temos que garantir que a área que alocamos para o buffer será liberada
		return -1;
	}

	unsigned int nextBlockPointer = *(unsigned int*)blockBuffer;	//ponteiro para o próximo bloco, se tiver
	DirData directoryInfo = *(DirData*)(&blockBuffer[blockPointerSize]);//struct contendo informações sobre o diretório
	int numEntriesInDirectory = directoryInfo.entryCount;//número de entradas neste diretório
	int directorySizeInBlocks = ceil((sizeof(DirData)+numEntriesInDirectory*sizeof(DirRecord))/blockSize);
	
	/*Iteramos pelas entradas no primeiro bloco procurando por uma entrada que tenha o nome do arquivo que procuramos*/
	int currentEntryIndex = 0;
	int entrySize = sizeof(DirRecord);
	DirRecord currentEntry;
	int numEntriesInFirstBlock;
	int currentEntryOffset;
	/*Se o número total de entradas neste diretório for menor que o número de entradas que cabem em um bloco,
	* significa que o número de entradas no primeiro bloco será o número total de entradas, visto que tem espaço sobrando
	  Caso contrário, o número de entradas no primeiro bloco será necessariamente o máximo que cabe nele*/
	if(maxEntryCountFirstBlock > numEntriesInDirectory) numEntriesInFirstBlock = numEntriesInDirectory;
	else numEntriesInFirstBlock = maxEntryCountFirstBlock;

	for(currentEntryIndex = 0; currentEntryIndex < numEntriesInFirstBlock; currentEntryIndex++){
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*currentEntryIndex;
		currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
		if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
			/*Encontramos a entrada que estávamos procurando, agora basta desligar seu bit de validade e invalidar seu ponteiro*/
			currentEntry.isValid = false;
			currentEntry.dataPointer = -1;
			/*Sobrescrevemos a entrada no buffer */
			memcpy(&blockBuffer[currentEntryOffset], &currentEntry, sizeof(DirRecord));

			/*Sobrescrevemos o bloco no disco */
			if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
				#ifdef VERBOSE_DEBUG
					printf("[SetDirectoryEntryAsInvalid]Erro ao escrever o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
				#endif
				destroyBuffer(blockBuffer);
				return -1;
			}

			/*Pronto, a função já pode retornar*/
			destroyBuffer(blockBuffer);
			return 0;
		}
	}

	/*Se a função chegou aqui, significa que a entrada que estávamos procurando não está no primeiro bloco,
	* então vamos procurar nos outros, mas primeiro temos que checar se este diretório tem outros blocos */
	if(directorySizeInBlocks == 1){
		/*Se o diretório não tem mais blocos, significa que não conseguimos encontrar o arquivo, o que é um erro */
		#ifdef VERBOSE_DEBUG
			printf("[SetDirectoryEntryAsInvalid]Não foi possível encontrar a entrada\n");
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	/*Se a função chegou aqui, significa que precisamos procurar nos outros blocos do diretório pela entrada */
	//lembrando que nextBlockPointer já nos diz o número do segundo bloco deste diretório 
	//(recuperamos esta informação lá no início da função)

	/*Número de entradas do diretório sobre as quais ainda falta iterar,
	* usamos esse valor para checar se tem espaço sobrando em algum bloco
	* e para calcular quantas entradas tem um dado bloco*/
	int entriesRemaining = numEntriesInDirectory - maxEntryCountFirstBlock;

	/*Número de entradas em um dado bloco, usamos esse valor para iterar
	* sobre as entradas de cada bloco */
	int numEntriesInCurrentBlock; 

	unsigned int currentBlockNumber = 1; //começa em 1 pois já analisamos o primeiro bloco
	for(currentBlockNumber = 1; currentBlockNumber < directorySizeInBlocks; currentBlockNumber++){
		if(readBlock(nextBlockPointer, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar ler o bloco %d\n",nextBlockPointer);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/*O primeiro passo é iterar pelas entradas de diretório que estão neste bloco
		* procurando pela entrada que tem o nome do arquivo que procuramos*/

		/*Primeiramente calculamos quantas entradas existem no bloco atual.
		* Se o número de entradas sobre as quais falta iterar for menor que o máximo que este bloco
		* suporta, então o número de entradas neste bloco é exatamente o número de entradas sobre as quais falta iterar
		* Caso contrário, este bloco necessariamente terá o número máximo de entradas */
		if(maxEntryCountOtherBlocks > entriesRemaining) numEntriesInCurrentBlock = entriesRemaining;
		else numEntriesInCurrentBlock = maxEntryCountOtherBlocks;

		/*Agora iteramos sobre todas as entradas deste bloco procurando uma inválida para sobrescrever*/
		for(currentEntryIndex = 0; currentEntryIndex<numEntriesInCurrentBlock; currentEntryIndex++){
			
			currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*currentEntryIndex;
			
			currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
			if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
				/*Encontramos a entrada que estávamos procurando, agora basta desligar seu bit de validade e invalidar seu ponteiro*/
				currentEntry.isValid = false;
				currentEntry.dataPointer = -1;
				/*Sobrescrevemos a entrada no buffer */
				memcpy(&blockBuffer[currentEntryOffset], &currentEntry, sizeof(DirRecord));

				/*Sobrescrevemos o bloco no disco */
				if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[SetDirectoryEntryAsInvalid]Erro ao escrever o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
					#endif
					destroyBuffer(blockBuffer);
					return -1;
				}

				/*Pronto, a função já pode retornar*/
				destroyBuffer(blockBuffer);
				return 0;
			}
		}

		/*Se a função chegou aqui, significa que não encontramos a entrada que procurávamos no bloco atual,
		* então vamos pegar o número do próximo bloco e continuar iterando
		* (ou sair do loop se estivermos no último) */

		/*Se alcançarmos o último bloco, podemos só sair do loop*/
		if(*(unsigned int*)blockBuffer == UINT_MAX) break;

		/*Caso contrário, pegamos os dados necessários e seguimos iterando*/
		entriesRemaining = entriesRemaining - maxEntryCountOtherBlocks; //o bloco atual necessariamente está cheio
		nextBlockPointer = *(unsigned int*)blockBuffer;
	}

	/*Se a função chegou aqui, significa que ela leu o diretório inteiro e não encontrou a entrada que estava procurando.
	* Sendo assim, encerramos a função e retornamos um código de erro */
	destroyBuffer(blockBuffer);
	#ifdef VERBOSE_DEBUG
		printf("[SetDirectoryEntryAsInvalid]Não foi possivel encontrar a entrada\n");
	#endif
	return -1;

}





/*
*	Dado um buffer em memória de tamanho adequado, lê o bitmap inteiro do disco e o escreve no buffer (para que se possam fazer leituras e atualizações deste)
*	Retorna 0 se conseguiu fazer a leitura inteira do bitmap com sucesso, -1 caso contrário
*/
int readBitmap(unsigned char* bitmap){
	
	unsigned int firstBitmapSector = firstSectorPartition1+1; //o bitmap inicia no segundo setor da partição
	int bitmapSizeInSectors = ceil((float)getBytesForBitmap()/(float)SECTOR_SIZE);
	int currentBitmapSectorOffset = 0;
	int currentBitmapSector = 0;

	for(currentBitmapSectorOffset = 0; currentBitmapSectorOffset <bitmapSizeInSectors; currentBitmapSectorOffset++){
		currentBitmapSector = firstBitmapSector + currentBitmapSectorOffset;
		if(read_sector(currentBitmapSector, &bitmap[SECTOR_SIZE*currentBitmapSectorOffset]) !=0){
			return -1;
		}
	}

	return 0;
}


/*
*	Dado um buffer de tamanho adequado, sobrescreve o bitmap em disco com o conteúdo deste buffer
*	Retorna 0 se conseguiu realizar todas as escritas com sucesso, -1 caso contrário
*/
int writeBitmap(unsigned char* buffer){
	int firstBitmapSector = firstSectorPartition1+1; //o bitmap inicia no segundo setor da partição
	int bitmapSizeInSectors = ceil((float)getBytesForBitmap()/(float)SECTOR_SIZE);
	int currentBitmapSectorOffset = 0;
	int currentBitmapSector = 0;

	for(currentBitmapSectorOffset = 0; currentBitmapSectorOffset <bitmapSizeInSectors; currentBitmapSectorOffset++){
		currentBitmapSector = firstBitmapSector + currentBitmapSectorOffset;
		if(write_sector(currentBitmapSector, &buffer[SECTOR_SIZE*currentBitmapSectorOffset]) !=0)
			return -1;
	}
	return 0;
}

/*
*	Liga (ou seja, escreve 1) o blockNumber-ésimo bit de um bitmap, significando que o bloco de número blockNumber está vazio e pode ser sobrescrito
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
void setBlockUnused(int blockNumber, unsigned char* bitmap){
	setBit(bitmap, blockNumber);
}

/*
*	Desliga (ou seja, escreve 0) o blockNumber-ésimo bit de um bitmap, significando que o bloco de número blockNumber está sendo utilizado por um arquivo e não pode ser sobrescrito
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
void setBlockUsed(int blockNumber, unsigned char* bitmap){
	clearBit(bitmap, blockNumber);
}

/*
*	Dado o número do primeiro bloco de um arquivo, itera por todos os blocos do arquivo, marcando-os no bitmap como vagos
*	e invalidando seus ponteiros, para que mais tarde possam ter seu conteúdo sobrescrito
*	(Esta função faz parte do processamento de delete2)
* 	Retorna 0 se executou com sucesso, -1 caso contrário
*/
int setFileBlocksAsUnused(int firstBlock){
	unsigned int fileCurrentBlockNumber;
	unsigned int fileNextBlockNumber;
	unsigned int invalidPointer = UINT_MAX;
	unsigned int currentBlockFirstSectorNumber;
	unsigned char* sectorBuffer = createSectorBuffer();	//um ponteiro para uma área de memória de tamanho adequado para armazenar um setor inteiro
	unsigned char* bitmap = createBitmapBuffer(); //um ponteiro para uma área de memória de tamanho adequado para armazenar o bitmap inteiro

	if(readBitmap(bitmap) != 0){ //lê o bitmap do disco e o coloca no buffer bitmap
		destroyBuffer(bitmap);
		destroyBuffer(sectorBuffer);
		#ifdef VERBOSE_DEBUG
			printf("[setFileBlocksAsUnused] Erro ao tentar ler o bitmap \n");
		#endif
		return -1;
	}
	
	fileCurrentBlockNumber = firstBlock;

	while(fileCurrentBlockNumber != UINT_MAX){
		/*
		*	Configura o bit correspondente ao bloco de número fileCurrentBlockNumber no bitmap como não utilizado,
		*	lê o primeiro setor do bloco de número fileCurrentBlockNumber,
		*	salva o ponteiro para o próximo bloco em fileNextBlockNumber,
		*	sobrescreve este ponteiro no disco com UINT_MAX e
		*	Repete até que o ponteiro lido seja UINT_MAX, significando fim de arquivo
		*/
		
		/*Configura o bitmap marcando este bloco como vago*/
		setBlockUnused(fileCurrentBlockNumber, bitmap);

		/*Descobre o número do primeiro setor do bloco atual */
		currentBlockFirstSectorNumber = getFirstSectorOfBlock(fileCurrentBlockNumber);

		/*Lê o primeiro setor do bloco atual */
		if(read_sector(currentBlockFirstSectorNumber, sectorBuffer) != 0){
			destroyBuffer(bitmap);
			destroyBuffer(sectorBuffer);
			#ifdef VERBOSE_DEBUG
				printf("[setFileBlocksAsUnused] Erro ao tentar ler o setor %d\n",currentBlockFirstSectorNumber);
			#endif
			return -1;
		}

		fileNextBlockNumber = *(unsigned int*)sectorBuffer; //o ponteiro para o próximo bloco está nos primeiros bytes do setor, 
															//então basta recuperar o valor apontado por ele como se ele fosse um ponteiro para unsigned int
		/*Invalidamos o ponteiro do bloco atual*/
		memcpy(sectorBuffer, &invalidPointer, sizeof(unsigned int));
		
		/*Escrevemos o primeiro setor do bloco atual no disco */
		if(write_sector(currentBlockFirstSectorNumber, sectorBuffer) != 0){
			destroyBuffer(bitmap);
			destroyBuffer(sectorBuffer);
			#ifdef VERBOSE_DEBUG
				printf("[setFileBlocksAsUnused] Erro ao tentar escrever no setor %d\n",currentBlockFirstSectorNumber);
			#endif
			return -1;
		}

		/*Passamos para o próximo bloco*/
		fileCurrentBlockNumber = fileNextBlockNumber;

	}

	if(writeBitmap(bitmap) != 0){	//sobrescreve o bitmap em disco com o conteúdo do buffer bitmap
		destroyBuffer(bitmap);
		destroyBuffer(sectorBuffer);
		#ifdef VERBOSE_DEBUG
			printf("[setFileBlocksAsUnused] Erro ao tentar escrever no bitmap \n");
		#endif
		return -1;
	}

	destroyBuffer(bitmap);	//libera as áreas dos buffers, já que não serão mais necessárias
	destroyBuffer(sectorBuffer);
	return 0;
}

int fileExistsInDir(char* fileName, int parentDirFirstBlock){
	int fileBlock = goToFileFromParentDir(fileName, parentDirFirstBlock);
	if(fileBlock<0){
		return 0;
	}else return 1;
}

int isPathnameAlphanumeric(char* pathname, int maxPathSize){
	int pathnameIndex=0;
	while(pathname[pathnameIndex]!= '\0'){
		if((isalnum(pathname[pathnameIndex])==0)&&(pathname[pathnameIndex]!='/'))
			return -1;
		pathnameIndex++;
		if(pathnameIndex>maxPathSize)
			return -1;
	}
	return 0;
}

int getFileType(int firstBlockNumber){
	unsigned char sectorBuffer[SECTOR_SIZE];
	int sectorToUse = getFirstSectorOfBlock(firstBlockNumber);
	if(read_sector(sectorToUse,sectorBuffer)!=0)
		return -1;
	DirData dirDataToRead = *(DirData*)(sectorBuffer+blockPointerSize);
	printf("[getFileType] Tipo do arquivo de bloco %d: %d\n",firstBlockNumber,dirDataToRead.fileType);
	return dirDataToRead.fileType;
}


/*
*	Dados o número do primeiro bloco do diretório, um nome de arquivo e um buffer para uma entrada de diretório,
*	lê do disco a entrada correspondente e a escreve no buffer.
*	Se o arquivo não existir nesse diretório, retorna -1
*	Se executou com sucesso, retorna 0
*/
int getDirectoryEntry(unsigned int directoryFirstBlockNumber, char* filename, DirRecord* dirRecordBuffer){
	
	/*O primeiro bloco de um diretório tem um ponteiro para o próximo e a estrutura de informações do diretório
	* Apenas após estes dados que as entradas de fato começam, então calculamos esse offset para poder
	* calcular quantas entradas de diretório cabem no primeiro bloco*/
	int firstBlockFirstEntryOffset = sizeof(unsigned int)+sizeof(DirData);
	int firstBlockEntryRegionSize = blockSize - firstBlockFirstEntryOffset;
	int maxEntryCountFirstBlock= floor(firstBlockEntryRegionSize/(float)sizeof(DirRecord));

	/*Quanto aos outros blocos, basta fazer o mesmo cálculo considerando apenas o ponteiro para o próximo
	* bloco como offset*/
	int otherBlocksFirstEntryOffset = sizeof(unsigned int);
	int otherBlocksEntryRegionSize = blockSize - otherBlocksFirstEntryOffset;
	int maxEntryCountOtherBlocks = floor(otherBlocksEntryRegionSize/(float)sizeof(DirRecord));
	
	/*Lemos o primeiro bloco do diretório para recuperar as informações correspondentes a ele 
	* e o ponteiro para o próximo bloco*/
	unsigned char* blockBuffer = createBlockBuffer();
	if(readBlock(directoryFirstBlockNumber,blockBuffer)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[SetDirectoryEntryAsInvalid]Erro ao ler o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
		#endif
		destroyBuffer(blockBuffer); //sempre que a função for retornar, temos que garantir que a área que alocamos para o buffer será liberada
		return -1;
	}

	unsigned int nextBlockPointer = *(unsigned int*)blockBuffer;	//ponteiro para o próximo bloco, se tiver
	DirData directoryInfo = *(DirData*)(&blockBuffer[blockPointerSize]);//struct contendo informações sobre o diretório
	int numEntriesInDirectory = directoryInfo.entryCount;//número de entradas neste diretório
	int directorySizeInBlocks = ceil((sizeof(DirData)+numEntriesInDirectory*sizeof(DirRecord))/blockSize);
	
	/*Iteramos pelas entradas no primeiro bloco procurando por uma entrada que tenha o nome do arquivo que procuramos*/
	int currentEntryIndex = 0;
	int entrySize = sizeof(DirRecord);
	DirRecord currentEntry;
	int numEntriesInFirstBlock;
	int currentEntryOffset;
	/*Se o número total de entradas neste diretório for menor que o número de entradas que cabem em um bloco,
	* significa que o número de entradas no primeiro bloco será o número total de entradas, visto que tem espaço sobrando
	  Caso contrário, o número de entradas no primeiro bloco será necessariamente o máximo que cabe nele*/
	if(maxEntryCountFirstBlock > numEntriesInDirectory) numEntriesInFirstBlock = numEntriesInDirectory;
	else numEntriesInFirstBlock = maxEntryCountFirstBlock;

	for(currentEntryIndex = 0; currentEntryIndex < numEntriesInFirstBlock; currentEntryIndex++){
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*currentEntryIndex;
		currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
		if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
			/*Encontramos a entrada que estávamos procurando, agora basta copiar ela para o buffer e retornar*/
			memcpy(dirRecordBuffer, &currentEntry, sizeof(DirRecord));
			#ifdef VERBOSE_DEBUG
					printf("[getDirectoryEntry]Encontrou a entrada do arquivo %s\n",filename);
			#endif
			destroyBuffer(blockBuffer);
			return 0;
		}
	}

	/*Se a função chegou aqui, significa que a entrada que estávamos procurando não está no primeiro bloco,
	* então vamos procurar nos outros, mas primeiro temos que checar se este diretório tem outros blocos */
	if(directorySizeInBlocks == 1){
		/*Se o diretório não tem mais blocos, significa que não conseguimos encontrar o arquivo, o que é um erro */
		#ifdef VERBOSE_DEBUG
			printf("[getDirectoryEntry]Não foi possível encontrar a entrada\n");
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	/*Se a função chegou aqui, significa que precisamos procurar nos outros blocos do diretório pela entrada */
	//lembrando que nextBlockPointer já nos diz o número do segundo bloco deste diretório 
	//(recuperamos esta informação lá no início da função)

	/*Número de entradas do diretório sobre as quais ainda falta iterar,
	* usamos esse valor para checar se tem espaço sobrando em algum bloco
	* e para calcular quantas entradas tem um dado bloco*/
	int entriesRemaining = numEntriesInDirectory - maxEntryCountFirstBlock;

	/*Número de entradas em um dado bloco, usamos esse valor para iterar
	* sobre as entradas de cada bloco */
	int numEntriesInCurrentBlock; 

	unsigned int currentBlockNumber = 1; //começa em 1 pois já analisamos o primeiro bloco
	for(currentBlockNumber = 1; currentBlockNumber < directorySizeInBlocks; currentBlockNumber++){
		if(readBlock(nextBlockPointer, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[insertEntryInDir] Erro ao tentar ler o bloco %d\n",nextBlockPointer);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/*O primeiro passo é iterar pelas entradas de diretório que estão neste bloco
		* procurando pela entrada que tem o nome do arquivo que procuramos*/

		/*Primeiramente calculamos quantas entradas existem no bloco atual.
		* Se o número de entradas sobre as quais falta iterar for menor que o máximo que este bloco
		* suporta, então o número de entradas neste bloco é exatamente o número de entradas sobre as quais falta iterar
		* Caso contrário, este bloco necessariamente terá o número máximo de entradas */
		if(maxEntryCountOtherBlocks > entriesRemaining) numEntriesInCurrentBlock = entriesRemaining;
		else numEntriesInCurrentBlock = maxEntryCountOtherBlocks;

		/*Agora iteramos sobre todas as entradas deste bloco procurando uma inválida para sobrescrever*/
		for(currentEntryIndex = 0; currentEntryIndex<numEntriesInCurrentBlock; currentEntryIndex++){
			
			currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*currentEntryIndex;
			
			currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
			if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
				/*Encontramos a entrada que estávamos procurando, agora basta copiar ela para o buffer e retornar*/
				memcpy(dirRecordBuffer, &currentEntry, sizeof(DirRecord));
				
				#ifdef VERBOSE_DEBUG
						printf("[getDirectoryEntry]Encontrou a entrada do arquivo %s\n",filename);
				#endif
				destroyBuffer(blockBuffer);
				return 0;
			}
		}

		/*Se a função chegou aqui, significa que não encontramos a entrada que procurávamos no bloco atual,
		* então vamos pegar o número do próximo bloco e continuar iterando
		* (ou sair do loop se estivermos no último) */

		/*Se alcançarmos o último bloco, podemos só sair do loop*/
		if(*(unsigned int*)blockBuffer == UINT_MAX) break;

		/*Caso contrário, pegamos os dados necessários e seguimos iterando*/
		entriesRemaining = entriesRemaining - maxEntryCountOtherBlocks; //o bloco atual necessariamente está cheio
		nextBlockPointer = *(unsigned int*)blockBuffer;
	}

	/*Se a função chegou aqui, significa que ela leu o diretório inteiro e não encontrou a entrada que estava procurando.
	* Sendo assim, encerramos a função e retornamos um código de erro */
	destroyBuffer(blockBuffer);
	#ifdef VERBOSE_DEBUG
		printf("[getDirectoryEntry]Não foi possivel encontrar a entrada\n");
	#endif
	return -1;
}

int getOpenFileData(int handle,OpenFileData *openFileData){
	if((handle<0)||(handle>10)){
		return -1;
	}
	memcpy(openFileData,&open_files[handle],sizeof(OpenFileData));
	return 1;
}

int getCurrentPointerPosition(unsigned int currentPointer,unsigned int firstBlockOfFile,BlockAndByteOffset *blockAndByteOffset){
	/*O primeiro bloco de um arquivo tem um ponteiro para o próximo e a estrutura de informações do arquivo
	* Apenas após estes dados que os dados de fato começam, então calculamos esse offset para poder
	* calcular quantos bytes de dados cabem no primeiro bloco*/
	int firstBlockDataOffset = sizeof(unsigned int)+sizeof(DirData);
	int firstBlockDataRegionSize = blockSize - firstBlockDataOffset;
	unsigned char* sectorBuffer = createSectorBuffer();
	/*Quanto aos outros blocos, basta fazer o mesmo cálculo considerando apenas o ponteiro para o próximo
	* bloco como offset*/
	int otherBlocksDataOffset = sizeof(unsigned int);
	int otherBlocksDataRegionSize = blockSize - otherBlocksDataOffset;
	
	//Ponteiro a ser usado na iteração de blocos
	unsigned int nextBlockPointer;
	int currentBlockDataSize=firstBlockDataRegionSize;//Tamanho da area de blocos do bloco sendo iterado
	unsigned int currentPointerIterator=currentPointer;
	int currentBlock=firstBlockOfFile;
	/*Queremos achar o bloco no qual o CurrentPointer estaria, junto com seu offset, então temos que iterar sobre os blocos
	* para descobrirmos o número do bloco do currentPointer.
	* O offset precisa ser descoberto também, subtraíndo um offset "total" (currentPointerIterator) até que sobre um offset que "caiba" no bloco sendo iterado.
	* A iteração acaba quando esse offset conseguir caber em um bloco, retornando esse offset+offset de dados do bloco, mais o bloco em si.
	*/
	while(currentPointerIterator>currentBlockDataSize){
		#ifdef VERBOSE_DEBUG
			printf("[getCurrentPointerPosition]CurrentPointer aponta para depois do bloco %d\n",currentBlock);
		#endif
		if(read_sector(currentBlock,sectorBuffer)!=0){
			return -1;
		}
		nextBlockPointer = *(unsigned int*)(sectorBuffer);
		if(nextBlockPointer==UINT_MAX){
			return -1;//ERRO: Current pointer aponta para um local não referenciável
		}
		currentBlock=nextBlockPointer;
		currentPointerIterator-=currentBlockDataSize;
		currentBlockDataSize=otherBlocksDataRegionSize;
	}
	destroyBuffer(sectorBuffer);
	BlockAndByteOffset offsetToCopy;//offset a ser usado com memcpy ao original
	offsetToCopy.block = currentBlock;// Setamos o bloco do offset como sendo o bloco no qual 
	if(currentBlockDataSize=firstBlockDataRegionSize){
		offsetToCopy.byte = currentPointerIterator+firstBlockDataOffset;
	}else{
		offsetToCopy.byte = currentPointerIterator+otherBlocksDataOffset;
	}
	memcpy(blockAndByteOffset,&offsetToCopy,sizeof(offsetToCopy));//Efetua a escrita na struct passada por referência
	return 1;

}

int getNewFileSize(unsigned int fileOriginalSize, unsigned int dataBeingWrittenSize, unsigned int currentPointer){
	/*Calcula o quão longe o ponteiro atual está do fim do arquivo, já que escrita poderia ser no meio
	 *Ajustar o tamanho do file na struct dele, sendo igual a TamanhoAnterior+(size- distanciaDoFinal).
	 *O tamanho novo não pode ser menor, se for retornar tamanho original
	*/
	int bytesBetweenPointerAndEnd= (signed)fileOriginalSize - (signed)currentPointer;
	if(bytesBetweenPointerAndEnd<0){//ERRO: currentPointer aponta para fora do arquivo
		return -1;
	}
	if(bytesBetweenPointerAndEnd >= dataBeingWrittenSize){
		return fileOriginalSize;
	}else{	
		int fileNewSize = fileOriginalSize+(dataBeingWrittenSize-bytesBetweenPointerAndEnd);
		return fileNewSize;
	}
}

int writeData(char *buffer, int bufferSize, int blockToWriteOn, int blockToWriteOffset){
	/*efetua a escrita, iterando sobre os blocos, alocando blocos novos (se o proximo ponteiro for válido, usar esse ponteiro)
	* Escreve a primeira leva de dados no bloco dado, depois, se sobrar dados, iniciar iteração
	* Enquanto os dados do buffer não acabarem, escrever no bloco com offset definido, achar ou alocar próximo bloco, ir até ele
	*/
	unsigned char* blockBuffer = createBlockBuffer();
	int totalBytesToWrite =bufferSize;
	int bytesToWriteInBlock;
	int currentBlockToWriteOn=blockToWriteOn;
	unsigned int nextBlockPointer;
	int blockDataSize;
	//unsigned int allocatedBlockNumber;
	do{
		readBlock(currentBlockToWriteOn,blockBuffer);
		nextBlockPointer = *(unsigned int*)(blockBuffer);
		blockDataSize=blockSize-blockToWriteOffset;
		if(blockDataSize < totalBytesToWrite){//bytes para escrever no bloco deve ser o mínimo entre o
			bytesToWriteInBlock =blockDataSize;// tamanho do espaço de dados do bloco e o tamanho que precisa escrever do buffer
		}else{
			bytesToWriteInBlock =totalBytesToWrite;
		}
		#ifdef VERBOSE_DEBUG
			printf("[writeData] Bytes para escrever no bloco: %d\n",bytesToWriteInBlock);
		#endif
		//exit(-1); 
		totalBytesToWrite-=bytesToWriteInBlock;
		if(totalBytesToWrite>0){
			if(nextBlockPointer==UINT_MAX){
				nextBlockPointer= allocateBlock();
				memcpy(blockBuffer,(unsigned char*)&nextBlockPointer,sizeof(unsigned int));
			}
			currentBlockToWriteOn=nextBlockPointer;
		}
		memcpy(&blockBuffer[blockToWriteOffset],(unsigned char*)buffer,bytesToWriteInBlock);//botar if
		if(writeBlock(currentBlockToWriteOn,blockBuffer)<0)
			return -1;
		blockToWriteOffset = sizeof(unsigned int);
	}while(totalBytesToWrite>0);
	destroyBuffer(blockBuffer);
}

int writeCurrentFileData(int handle,OpenFileData openFileData){
	if((handle<0)||(handle>10)){
		return -1;
	}
	memcpy(&open_files[handle],&openFileData,sizeof(OpenFileData));
	if(SetSizeOfFileInDir(openFileData.parentDirBlock,openFileData.fileRecord.name,openFileData.fileRecord.fileSize)==-1)
		return -1;
	unsigned char sectorBuffer[SECTOR_SIZE];
	int fileFirstSector = getFirstSectorOfBlock(openFileData.fileRecord.dataPointer);
	if(read_sector(fileFirstSector,sectorBuffer)!=0)
		return -1;
	DirData dirDataToUpdate = *(DirData*)(sectorBuffer+blockPointerSize);
	dirDataToUpdate.fileSize = openFileData.fileRecord.fileSize;
	memcpy(sectorBuffer+sizeof(unsigned int),&dirDataToUpdate,sizeof(dirDataToUpdate));
	if(write_sector(fileFirstSector,sectorBuffer)!=0)
		return -1;
	return 1;
}


/*
*	Dados o primeiro bloco de um diretório e um nome de arquivo, percorre o diretório procurando o arquivo.
*	Quando encontrar, modifica o int de tamanho sua entrada
*	(Esta função faz parte do processamento de write2)
*	Retorna 0 se executou com sucesso e -1 caso contrário
*/
int SetSizeOfFileInDir(unsigned int directoryFirstBlockNumber, char* filename, unsigned int newFileSize){

	/*O primeiro bloco de um diretório tem um ponteiro para o próximo e a estrutura de informações do diretório
	* Apenas após estes dados que as entradas de fato começam, então calculamos esse offset para poder
	* calcular quantas entradas de diretório cabem no primeiro bloco*/
	int firstBlockFirstEntryOffset = sizeof(unsigned int)+sizeof(DirData);
	int firstBlockEntryRegionSize = blockSize - firstBlockFirstEntryOffset;
	int maxEntryCountFirstBlock= floor(firstBlockEntryRegionSize/(float)sizeof(DirRecord));

	/*Quanto aos outros blocos, basta fazer o mesmo cálculo considerando apenas o ponteiro para o próximo
	* bloco como offset*/
	int otherBlocksFirstEntryOffset = sizeof(unsigned int);
	int otherBlocksEntryRegionSize = blockSize - otherBlocksFirstEntryOffset;
	int maxEntryCountOtherBlocks = floor(otherBlocksEntryRegionSize/(float)sizeof(DirRecord));
	
	/*Lemos o primeiro bloco do diretório para recuperar as informações correspondentes a ele 
	* e o ponteiro para o próximo bloco*/
	unsigned char* blockBuffer = createBlockBuffer();
	if(readBlock(directoryFirstBlockNumber,blockBuffer)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[SetSizeOfFileInDir]Erro ao ler o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
		#endif
		destroyBuffer(blockBuffer); //sempre que a função for retornar, temos que garantir que a área que alocamos para o buffer será liberada
		return -1;
	}
	#ifdef VERBOSE_DEBUG
		printf("[SetSizeOfFileInDir]primeiro bloco do arquivo de diretório: %d\n",directoryFirstBlockNumber);
	#endif
	unsigned int nextBlockPointer = *(unsigned int*)blockBuffer;	//ponteiro para o próximo bloco, se tiver
	DirData directoryInfo = *(DirData*)(&blockBuffer[blockPointerSize]);//struct contendo informações sobre o diretório
	int numEntriesInDirectory = directoryInfo.entryCount;//número de entradas neste diretório
	int directorySizeInBlocks = ceil((sizeof(DirData)+numEntriesInDirectory*sizeof(DirRecord))/blockSize);
	
	/*Iteramos pelas entradas no primeiro bloco procurando por uma entrada que tenha o nome do arquivo que procuramos*/
	int currentEntryIndex = 0;
	int entrySize = sizeof(DirRecord);
	DirRecord currentEntry;
	int numEntriesInFirstBlock;
	int currentEntryOffset;
	/*Se o número total de entradas neste diretório for menor que o número de entradas que cabem em um bloco,
	* significa que o número de entradas no primeiro bloco será o número total de entradas, visto que tem espaço sobrando
	  Caso contrário, o número de entradas no primeiro bloco será necessariamente o máximo que cabe nele*/
	if(maxEntryCountFirstBlock > numEntriesInDirectory) numEntriesInFirstBlock = numEntriesInDirectory;
	else numEntriesInFirstBlock = maxEntryCountFirstBlock;
	#ifdef VERBOSE_DEBUG
		printf("[SetSizeOfFileInDir]Número de entradas no bloco 1: %d\n ",numEntriesInFirstBlock);
	#endif
	for(currentEntryIndex = 0; currentEntryIndex < numEntriesInFirstBlock; currentEntryIndex++){
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*currentEntryIndex;
		currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
		#ifdef VERBOSE_DEBUG
			printf("[SetSizeOfFileInDir]Comparação de nomes no bloco 1: %s e %s.\n ",filename,currentEntry.name);
		#endif
		if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
			/*Encontramos a entrada que estávamos procurando, agora basta atualizar seu tamanho*/
			currentEntry.fileSize = newFileSize;
			/*Sobrescrevemos a entrada no buffer */
			memcpy(&blockBuffer[currentEntryOffset], &currentEntry, sizeof(DirRecord));

			/*Sobrescrevemos o bloco no disco */
			if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
				#ifdef VERBOSE_DEBUG
					printf("[SetSizeOfFileInDir]Erro ao escrever o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
				#endif
				destroyBuffer(blockBuffer);
				return -1;
			}

			/*Pronto, a função já pode retornar*/
			destroyBuffer(blockBuffer);
			return 0;
		}
	}

	/*Se a função chegou aqui, significa que a entrada que estávamos procurando não está no primeiro bloco,
	* então vamos procurar nos outros, mas primeiro temos que checar se este diretório tem outros blocos */
	if(directorySizeInBlocks == 1){
		/*Se o diretório não tem mais blocos, significa que não conseguimos encontrar o arquivo, o que é um erro */
		#ifdef VERBOSE_DEBUG
			printf("[SetSizeOfFileInDir]Não foi possível encontrar a entrada\n");
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	/*Se a função chegou aqui, significa que precisamos procurar nos outros blocos do diretório pela entrada */
	//lembrando que nextBlockPointer já nos diz o número do segundo bloco deste diretório 
	//(recuperamos esta informação lá no início da função)

	/*Número de entradas do diretório sobre as quais ainda falta iterar,
	* usamos esse valor para checar se tem espaço sobrando em algum bloco
	* e para calcular quantas entradas tem um dado bloco*/
	int entriesRemaining = numEntriesInDirectory - maxEntryCountFirstBlock;

	/*Número de entradas em um dado bloco, usamos esse valor para iterar
	* sobre as entradas de cada bloco */
	int numEntriesInCurrentBlock; 

	unsigned int currentBlockNumber = 1; //começa em 1 pois já analisamos o primeiro bloco
	for(currentBlockNumber = 1; currentBlockNumber < directorySizeInBlocks; currentBlockNumber++){
		if(readBlock(nextBlockPointer, blockBuffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[SetSizeOfFileInDir] Erro ao tentar ler o bloco %d\n",nextBlockPointer);
			#endif
			destroyBuffer(blockBuffer);
			return -1;
		}

		/*O primeiro passo é iterar pelas entradas de diretório que estão neste bloco
		* procurando pela entrada que tem o nome do arquivo que procuramos*/

		/*Primeiramente calculamos quantas entradas existem no bloco atual.
		* Se o número de entradas sobre as quais falta iterar for menor que o máximo que este bloco
		* suporta, então o número de entradas neste bloco é exatamente o número de entradas sobre as quais falta iterar
		* Caso contrário, este bloco necessariamente terá o número máximo de entradas */
		if(maxEntryCountOtherBlocks > entriesRemaining) numEntriesInCurrentBlock = entriesRemaining;
		else numEntriesInCurrentBlock = maxEntryCountOtherBlocks;

		/*Agora iteramos sobre todas as entradas deste bloco procurando uma inválida para sobrescrever*/
		for(currentEntryIndex = 0; currentEntryIndex<numEntriesInCurrentBlock; currentEntryIndex++){
			
			currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*currentEntryIndex;
			
			currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
			if(strcmp(filename,currentEntry.name)==0 && currentEntry.isValid){
				/*Encontramos a entrada que estávamos procurando, agora basta atualizar seu tamanho*/
				currentEntry.fileSize = newFileSize;
				/*Sobrescrevemos a entrada no buffer */
				memcpy(&blockBuffer[currentEntryOffset], &currentEntry, sizeof(DirRecord));

				/*Sobrescrevemos o bloco no disco */
				if(writeBlock(directoryFirstBlockNumber, blockBuffer) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[SetSizeOfFileInDir]Erro ao escrever o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
					#endif
					destroyBuffer(blockBuffer);
					return -1;
				}

				/*Pronto, a função já pode retornar*/
				destroyBuffer(blockBuffer);
				return 0;
			}
		}

		/*Se a função chegou aqui, significa que não encontramos a entrada que procurávamos no bloco atual,
		* então vamos pegar o número do próximo bloco e continuar iterando
		* (ou sair do loop se estivermos no último) */

		/*Se alcançarmos o último bloco, podemos só sair do loop*/
		if(*(unsigned int*)blockBuffer == UINT_MAX) break;

		/*Caso contrário, pegamos os dados necessários e seguimos iterando*/
		entriesRemaining = entriesRemaining - maxEntryCountOtherBlocks; //o bloco atual necessariamente está cheio
		nextBlockPointer = *(unsigned int*)blockBuffer;
	}

	/*Se a função chegou aqui, significa que ela leu o diretório inteiro e não encontrou a entrada que estava procurando.
	* Sendo assim, encerramos a função e retornamos um código de erro */
	destroyBuffer(blockBuffer);
	#ifdef VERBOSE_DEBUG
		printf("[SetSizeOfFileInDir]Não foi possivel encontrar a entrada\n Nome da entrada: %s\n",filename);
	#endif
	return -1;

}



/*
*	Dados um número de bloco, um offset, um cutoff e um buffer, copia o conteúdo do bloco inteiro a partir desse offset e até o cutoff (não incluindo)
*	para o buffer
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
int readFromBlockWithOffsetAndCutoff(unsigned int blockNumber, unsigned int offset, unsigned int cutoff, unsigned char* buffer){
	unsigned char* blockBuffer = createBlockBuffer();
	unsigned int bytesToRead = (blockSize - offset) - (blockSize - cutoff);
	
	if(offset < 0 || cutoff >= blockSize){
		#ifdef VERBOSE_DEBUG
			printf("[readFromBlockWithOffsetAndCutoff] Valor de offset (%d) ou cutoff (%d) invalido\n", offset, cutoff);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	if(readBlock(blockBuffer, blockNumber)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[readFromBlockWithOffsetAndCutoff] Erro ao tentar ler o bloco %d\n", blockNumber);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}

	memcpy(buffer, &blockBuffer[offset], bytesToRead);

	destroyBuffer(blockBuffer);
	return 0;
}

/*
*	Dados um número de bloco e um buffer, copia o ponteiro para o próximo bloco que está no início deste bloco para o buffer
*	Retorna 0 se executou com sucesso, -1 caso contrário
*/
int getPointerToNextBlock(unsigned int blockNumber, unsigned int* buffer){
	unsigned char* sectorBuffer = createSectorBuffer();
	int firstSectorNumber = getFirstSectorOfBlock(blockNumber);

	if(read_sector(firstSectorNumber, sectorBuffer)!=0){
		#ifdef VERBOSE_DEBUG
			printf("[getPointerToNextBlock] Erro ao tentar ler o setor %d\n", firstSectorNumber);
		#endif
		destroyBuffer(sectorBuffer);
		return -1;
	}

	memcpy(buffer, sectorBuffer, sizeof(unsigned int));

	destroyBuffer(sectorBuffer);
	return 0;
}

