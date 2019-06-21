
/**
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "../include/t2fs.h"
#include "../include/apidisk.h"
#include "../include/support.h"

#define VERBOSE_DEBUG
//para desativar os prints de debug, basta comentar esta linha

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size) {
	if(T2FSInitiated==0){
		initT2FS();
	}
    char components[] = "Amaury Teixeira Cassola 287704\nBruno Ramos Toresan 291332\nDavid Mees Knijnik 264489";
	if(size >= strlen(components)){
		strncpy(name, components, strlen(components)+1);
		return 0;
	}
	printf("Size is not sufficient\n");
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Formata logicamente o disco virtual t2fs_disk.dat para o sistema de
		arquivos T2FS definido usando blocos de dados de tamanho 
		correspondente a um múltiplo de setores dados por sectors_per_block.
-----------------------------------------------------------------------------*/
int format2 (int sectors_per_block) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	//startPartition1, sizePartition1
	if(formatFSData(sectors_per_block)!=0){
		return -1;
		//printf("Erro ao formatar o sistema de arquivos");
	}
	
	unsigned char sectorWithBlockPointerTemplate[SECTOR_SIZE];
	int sectorByte;
	for(sectorByte=0; sectorByte < SECTOR_SIZE; sectorByte++){
		sectorWithBlockPointerTemplate[sectorByte] = UCHAR_MAX;//Valor de ponteiro nulo, 11111111
	}
	int sectorToWrite;
	int currentBlock;
	int blockTotal =  ceil((lastSectorPartition1-firstSectorPartition1+1)/(float)sectors_per_block);
	for(currentBlock=0; currentBlock < blockTotal; currentBlock++){
		sectorToWrite= (currentBlock*sectors_per_block)+ blockSectionStart;
		if(write_sector(sectorToWrite,sectorWithBlockPointerTemplate)!=0){
			return -1;
		}
	}
	allocateRootDirBlock();
	DirData rootDirData;
	strcpy(rootDirData.name,"/\0");
	rootDirData.fileType =0x02;
	rootDirData.entryCount = 0;
	if(writeDirData(rootDirBlockNumber, rootDirData)==-1)
		return -1;
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo arquivo no disco e abrí-lo,
		sendo, nesse último aspecto, equivalente a função open2.
		No entanto, diferentemente da open2, se filename referenciar um 
		arquivo já existente, o mesmo terá seu conteúdo removido e 
		assumirá um tamanho de zero bytes.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename) {
		if(T2FSInitiated==0){
		initT2FS();
	}
	char path[MAX_FILE_NAME_SIZE+1];
	char name[32];
	
	if(isPathnameAlphanumeric(filename,MAX_FILE_NAME_SIZE+1)==-1){
		return -1;
	}
	if(getFileNameAndPath(filename,path, name)==-1){
		return -1;
	}
	int parentDirBlock = getFileBlock(path);
	if(parentDirBlock == -1){
		return -1;
	}
	
	if(getFileType(parentDirBlock)!=0x02){//Se o diretório pai não é um arquivo de diretório
		return -1;
	}

	if(fileExistsInDir(name,parentDirBlock)==1){
		printf("[create2] arquivo já existe em diretorio\n");
		if(delete2(filename)<0){
			return -1;
		}
	}
	int firstBlockNumber = allocateBlock();

	if(firstBlockNumber==-1){
		return -1;
	}
	
	DirData newDirData;
	strcpy(newDirData.name,name);
	newDirData.fileType = 0x01; // Tipo do arquivo: regular (0x01) 
	newDirData.entryCount = 0;
	
	DirRecord newDirEntry;
	strcpy(newDirEntry.name,name);
	newDirEntry.fileType = 0x01; // Tipo do arquivo: regular (0x01) 
	newDirEntry.dataPointer = firstBlockNumber;
	newDirEntry.isValid = true;
	newDirEntry.fileSize = 0;
	if(insertEntryInDir(parentDirBlock,newDirEntry)==-1){
		return -1;
	}
	if(writeDirData(firstBlockNumber,newDirData)==-1){
		return -1;
	}
	
	return open2(filename);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um arquivo do disco. 
-----------------------------------------------------------------------------*/
int delete2 (char *filename) {
	if(T2FSInitiated==0){
		initT2FS();
	}

	#ifdef VERBOSE_DEBUG
	printf("[delete2]Iniciando\n");
	#endif

	/*"Apagar" um arquivo do disco significa:
		1) Marcar a entrada de diretório correspondente como inválida,
		2) Invalidar os ponteiros de seus blocos e
		3) Marcar seus blocos como vagos no bitmap, para que mais tarde sejam sobrescritos
	*Esta função faz exatamente isso*/

	char path[MAX_FILE_NAME_SIZE+1];
	char name[32];

	/*Usamos esta função para conseguir o caminho até o arquivo */
	if(getFileNameAndPath(filename, path, name) != 0)
		return -1;

	#ifdef VERBOSE_DEBUG
	printf("[delete2]Chamando getFileBlock\n");
	#endif

	/*Usamos GetFileBlock para conseguir o primeiro bloco do diretório */
	int directoryFirstBlockNumber = getFileBlock(path);
	if(directoryFirstBlockNumber == -1){
		#ifdef VERBOSE_DEBUG
			printf("[delete2] Erro ao procurar o primeiro bloco do diretório %s\n",path);
		#endif
	}


	#ifdef VERBOSE_DEBUG
	printf("[delete2]Chamando SetDirectoryEntryAsInvalid\n");
	#endif
	/*Usamos  SetDirectoryEntryAsInvalid para invalidar a entrada de diretório correspondente a este arquivo*/
	if(SetDirectoryEntryAsInvalid(directoryFirstBlockNumber, name) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[delete2] Erro ao invalidar a entrada de diretorio do arquivo %s\n",filename);
		#endif
		return -1;
	}

	#ifdef VERBOSE_DEBUG
	printf("[delete2]Chamando setFileBlocksAsUnused\n");
	#endif
	/*Usamos a  setFileBlocksAsUnused para invalidar os ponteiros dos blocos deste arquivo e também
	* configurar o bitmap para informar que estes blocos estão vazios*/
	int fileFirstBlockNumber = getFileBlock(filename); //descobre o número do primeiro bloco do arquivo
	if(setFileBlocksAsUnused(fileFirstBlockNumber) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[delete2] Erro ao configurar blocos do arquivo %s como não utilizados\n",filename);
		#endif
		return -1;
	}
	
	#ifdef VERBOSE_DEBUG
	printf("[delete2]Terminando\n");
	#endif
	/*Se a função chegou até aqui, o arquivo já está deletado e a função pode retornar */
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	/* Esta função carrega do disco a entrada de diretório correspondente ao arquivo, usa ela para popular uma struct OpenFileData
	*	e a adiciona no array de arquivos abertos, retornando o índice do array no qual inseriu*/
	
	#ifdef VERBOSE_DEBUG
	printf("[open2]Iniciando\n");
	#endif

	char path[MAX_FILE_NAME_SIZE+1];
	char name[32];
	OpenFileData newOpenFileData;	/*struct que será mantida no array global */
	DirRecord* dirRecordBuffer = (DirRecord*)malloc(sizeof(DirRecord));		/*struct que será lida do disco */

	/*Usamos esta função para conseguir o caminho até o arquivo */
	if(getFileNameAndPath(filename, path, name) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[open2] Erro ao separar nome e diretório do pathname %s\n",filename);
		#endif
		free(dirRecordBuffer);
		return -1;
	}
		

	#ifdef VERBOSE_DEBUG
	printf("[open2]Chamando getFileBlock\n");
	#endif

	/*Usamos GetFileBlock para conseguir o primeiro bloco do diretório */
	int directoryFirstBlockNumber = getFileBlock(path);
	if(directoryFirstBlockNumber == -1){
		#ifdef VERBOSE_DEBUG
			printf("[open2] Erro ao procurar o primeiro bloco do diretório %s\n",path);
		#endif
		free(dirRecordBuffer);
		return -1;
	}

	if(getDirectoryEntry(directoryFirstBlockNumber, name, dirRecordBuffer) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[open2] Erro ao procurar o primeiro bloco do diretório %s\n",path);
		#endif
		free(dirRecordBuffer);
		return -1;
	}

	newOpenFileData.isValid = true;
	newOpenFileData.pointerToCurrentByte = 0;
	newOpenFileData.parentDirBlock = directoryFirstBlockNumber;
	memcpy(&newOpenFileData.fileRecord, dirRecordBuffer, sizeof(DirRecord));

	/*Iteramos pelo array de arquivos abertos procurando um que seja inválido para sobrescrever
	* Este array foi inicialiado com todos como inválidos */	
	int currentIndex = 0;
	for(currentIndex = 0; currentIndex < 10; currentIndex++){
		if(!open_files[currentIndex].isValid){
			open_files[currentIndex] = newOpenFileData;
			#ifdef VERBOSE_DEBUG
				printf("[open2] Arquivo aberto\n");
			#endif
			free(dirRecordBuffer);
			return (FILE2)currentIndex;
		}
	}

	#ifdef VERBOSE_DEBUG
		printf("[open2] Erro: já existem 10 arquivos abertos\n");
	#endif
	free(dirRecordBuffer);
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle) {
	
	if(T2FSInitiated==0){
		initT2FS();
	}
	
	if((0 <= handle) && (handle <= 9)){
		open_files[handle].isValid = false;
		return 0;
	}
	
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	#ifdef VERBOSE_DEBUG
		printf("[read2] Iniciando \n");
	#endif
	/* Primeiramente, descobrimos a partir de qual &bloco[offset] devemos ler
	*  Então, calculamos quantos blocos precisarão ser lidos, se mais de 1
	*  Por fim, até qual byte do último bloco devemos ler
	*  Se o número de bytes a serem lidos for maior que o tamanho do arquivo a partir do seu pointerToCurrentByte,
	*  Lemos até o fim, colocamos o pointerToCurrentByte no último byte+1 e retornamos o número de bytes efetivamente lidos
	*/
	int bytesActuallyRead = 0;//número de bytes que foram lidos do arquivo

	OpenFileData thisFile = open_files[handle];
	BlockAndByteOffset BnBOffset;
	printf("retorno getpointer %d\n", getCurrentPointerPosition(thisFile.pointerToCurrentByte, thisFile.fileRecord.dataPointer, &BnBOffset));

	int firstBlockToRead = BnBOffset.block;
	int firstBlockOffset = BnBOffset.byte;

	int otherBlocksOffset = sizeof(unsigned int);

	int remainingBytesInFirstBlock = blockSize - firstBlockOffset;//quantos bytes do primeiro bloco são legíveis (ou seja, se encontram após o offset)
	int remainingBytesInOtherBlocks = blockSize - otherBlocksOffset;//todos os blocos depois do primeiro terão apenas os bytes do ponteiro para o próximo como bytes não legíveis

	int remainingFileBytes = thisFile.fileRecord.fileSize - thisFile.pointerToCurrentByte; //quantos bytes o arquivo tem a partir do pointerToCurrentByte
	int remainingBytesToRead;

	if(size > remainingFileBytes){
		remainingBytesToRead = remainingFileBytes;
	} else {
		remainingBytesToRead = size;
	}

	int lastBlockCutoff;//será calculado quando chegarmos ao último bloco
	int otherBlocksCutoff = blockSize;


	if(remainingBytesInFirstBlock >= remainingBytesToRead){
		/*Vamos ler apenas bytes do primeiro bloco, sem acessar os blocos seguintes */
		#ifdef VERBOSE_DEBUG
			printf("[read2] Vai ler alguns bytes do primeiro bloco\n");
		#endif
		lastBlockCutoff = firstBlockOffset + remainingBytesToRead;
		if(readFromBlockWithOffsetAndCutoff(firstBlockToRead, firstBlockOffset, lastBlockCutoff, buffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[read2] Erro ao tentar ler o primeiro bloco %d\n", firstBlockToRead);
			#endif
			return -1;
		}

		bytesActuallyRead += remainingBytesToRead;
		open_files[handle].pointerToCurrentByte += bytesActuallyRead;
		return bytesActuallyRead;
	} else{
		/*Vamos ler bytes de vários blocos, não apenas do primeiro */

		/*Começamos lendo o primeiro*/
		if(readFromBlockWithOffsetAndCutoff(firstBlockToRead, firstBlockOffset, otherBlocksCutoff, buffer) != 0){
			#ifdef VERBOSE_DEBUG
				printf("[read2] Erro ao tentar ler o bloco %d\n", firstBlockToRead);
			#endif
			return -1;
		}
		bytesActuallyRead += remainingBytesInFirstBlock;
		remainingBytesToRead -= bytesActuallyRead;

		unsigned int currentBlockNumber;
		if(getPointerToNextBlock(firstBlockToRead, &currentBlockNumber) != 0){ //pegamos o ponteiro para o próximo bloco
			#ifdef VERBOSE_DEBUG
				printf("[read2] Erro ao tentar ler o ponteiro do bloco %d\n", firstBlockToRead);
			#endif
			return -1;
		}

		while(remainingBytesToRead > 0){
			if(remainingBytesInOtherBlocks >= remainingBytesToRead){
				/*Este é o último bloco que leremos, então precisamos calcular seu cutoff*/
				lastBlockCutoff = otherBlocksOffset + remainingBytesToRead;
				if(readFromBlockWithOffsetAndCutoff(currentBlockNumber, otherBlocksOffset, lastBlockCutoff, &buffer[bytesActuallyRead]) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[read2] Erro ao tentar ler o bloco %d\n", currentBlockNumber);
					#endif
					return -1;
				}

				bytesActuallyRead += (blockSize - otherBlocksOffset) - (blockSize - lastBlockCutoff);
				remainingBytesToRead -= bytesActuallyRead;
				open_files[handle].pointerToCurrentByte += bytesActuallyRead;
				return bytesActuallyRead;

			} else {
				/*Este não é o último bloco que leremos, então leremos ele inteiro */
				if(readFromBlockWithOffsetAndCutoff(currentBlockNumber, otherBlocksOffset, otherBlocksCutoff, &buffer[bytesActuallyRead]) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[read2] Erro ao tentar ler o bloco %d\n", currentBlockNumber);
					#endif
					return -1;
				}

				bytesActuallyRead += (blockSize - otherBlocksOffset) - (blockSize - otherBlocksCutoff);
				remainingBytesToRead -= bytesActuallyRead;

				if(getPointerToNextBlock(currentBlockNumber, &currentBlockNumber) != 0){ //pegamos o ponteiro para o próximo bloco
					#ifdef VERBOSE_DEBUG
						printf("[read2] Erro ao tentar ler o ponteiro do bloco %d\n", firstBlockToRead);
					#endif
					return -1;
				}
			}
		}
	}

	/*Se a função chegar aqui, houve um erro */
	#ifdef VERBOSE_DEBUG
		printf("[read2] ERRO\n");
	#endif
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	//Começa pegando os dados do diretório aberto a partir do handle
	OpenFileData openFileData;
	if(getOpenFileData(handle,&openFileData)<0){
		return -1;
	}
	unsigned int firstBlockOfFile = openFileData.fileRecord.dataPointer;
	unsigned int currentPointer = openFileData.pointerToCurrentByte;
	#ifdef VERBOSE_DEBUG
		printf("[write2]Bloco do arquivo a ser escrito: %d currentPointer: %d\n",firstBlockOfFile,currentPointer);
	#endif
	//
	//Acha a posição a partir da qual a escrita deve começar
	BlockAndByteOffset blockAndByteOffset;
	if(getCurrentPointerPosition(currentPointer,firstBlockOfFile,&blockAndByteOffset)<0){
		return -1;
	}
	//	
	#ifdef VERBOSE_DEBUG
		printf("[write2]Bloco no ponto do arquivo a ser escrito: %d offset: %d\n",blockAndByteOffset.block,blockAndByteOffset.byte);
	#endif
	
	//Calcular o quão longe o ponteiro atual está do fim do arquivo, já que escrita poderia ser no meio
	//Ajustar o tamanho do file na struct dele, sendo igual a TamanhoAnterior+(size- distanciaDoFinal).
	//	O tamanho novo não pode ser menor
	int newSize;
	newSize = getNewFileSize(openFileData.fileRecord.fileSize,size,currentPointer);
	if(newSize<0){
		return -1;
	}
	
	//efetuar de fato a escrita, iterando sobre os blocos, alocando blocos novos (se o proximo ponteiro for válido, usar esse ponteiro)
	if(writeData(buffer,size,blockAndByteOffset.block,blockAndByteOffset.byte)<0){
		return -1;
	}
	
	//atualizar a estrutura de dados do arquivo aberto
	//currentPointer e size
	openFileData.pointerToCurrentByte=currentPointer+size;
	openFileData.fileRecord.fileSize=newSize;
	if(writeCurrentFileData(handle,openFileData)<0){
		return -1;
	}
	
	return 1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo. Remove do arquivo 
		todos os bytes a partir da posição atual do contador de posição
		(current pointer), inclusive, até o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Altera o contador de posição (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um novo diretório.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	char path[MAX_FILE_NAME_SIZE+1];
	char name[32];
	
	if(isPathnameAlphanumeric(pathname,MAX_FILE_NAME_SIZE+1)==-1){
		printf("[mkdir2]Erro: nome de arquivo inválido\n");
		return -1;
	}
	printf("[mkdir2]Começando a ler o path do arquivo\n");
	if(getFileNameAndPath(pathname,path, name)==-1){
		printf("[mkdir2]Erro ao ler o path do arquivo\n");
		return -1;
	}
	//TO DO: CRIAR TESTE SE HÁ ARQUIVO DE MESMO NOME NO DIRETÓRIO
	printf("[mkdir2]lendo o bloco do diretório pai\n");
	int parentDirBlock = getFileBlock(path);
	if(parentDirBlock == -1){
		printf("[mkdir2] Erro ao ler o bloco do diretório pai\nNome do diretório:%s\n",path);
		return -1;
	}
	
	if(getFileType(parentDirBlock)!=0x02){//Se o diretório pai não é um arquivo de diretório
		printf("[mkdir2] Erro: arquivo em path não diretório\n");
		return -1;
	}
	

	if(fileExistsInDir(name,parentDirBlock)==1){
		printf("[mkdir2] Arquivo de nome %s já existe no diretório\n",name);
		return -1;
	}

	int firstBlockNumber = allocateBlock();
	printf("[mkdir2] Bloco %d Alocado para o arquivo\n",firstBlockNumber);

	if(firstBlockNumber==-1){
		printf("[mkdir2] Erro ao alocar um bloco\n");
		return -1;
	}
	
	DirData newDirData;
	strcpy(newDirData.name,name);
	newDirData.fileType = 0x02; // Tipo do arquivo: diretório (0x02) 
	newDirData.entryCount = 0;
	
	DirRecord newDirEntry;
	strcpy(newDirEntry.name,name);
	newDirEntry.fileType = 0x02; // Tipo do arquivo: diretório (0x02) 
	newDirEntry.dataPointer = firstBlockNumber;
	newDirEntry.isValid = true;
	newDirEntry.fileSize = 0;
	printf("[mkdir2]Nome do arquivo no diretório: %s, tipo: %d, ponteiro: %d\n",newDirEntry.name,newDirEntry.fileType,newDirEntry.dataPointer);
	
	if(insertEntryInDir(parentDirBlock,newDirEntry)==-1){
		printf("[mkdir2]Erro ao inserir entrada em diretório\n");
		return -1;
	}
	
	return writeDirData(firstBlockNumber,newDirData);
}

/*-----------------------------------------------------------------------------
Função:	Função usada para remover (apagar) um diretório do disco.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	/*
	*	Remover um diretório significa remover do disco todos os seus arquivos e subdiretórios e, então, o diretório em si.
	*	Para cada arquivo é chamada a delete2 e para cada subdiretório é feita uma chamada
	*	recursiva desta função, assim garantindo que toda a árvore que tem esse diretório como raiz
	*	será removida do disco. Após isso, é chamada delete2 para apagar o diretório.
	*/

	int directoryFirstBlockNumber = getFileBlock(pathname);
	
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
			printf("[rmdir2]Erro ao ler o primeiro bloco do arquivo de diretório\nBloco: %d\n",directoryFirstBlockNumber);
		#endif
		destroyBuffer(blockBuffer); //sempre que a função for retornar, temos que garantir que a área que alocamos para o buffer será liberada
		return -1;
	}
	unsigned int nextBlockPointer = *(unsigned int*)blockBuffer;	//ponteiro para o próximo bloco, se tiver
	DirData directoryInfo = *(DirData*)(&blockBuffer[blockPointerSize]);//struct contendo informações sobre o diretório
	int numEntriesInDirectory = directoryInfo.entryCount;//número de entradas neste diretório
	
	int directorySizeInBlocks = ceil((sizeof(DirData)+numEntriesInDirectory*sizeof(DirRecord))/blockSize);

	/*Iteramos pelas entradas no primeiro bloco deletando-as uma a uma*/
	int currentEntryIndex = 0;
	int entrySize = sizeof(DirRecord);
	DirRecord currentEntry;
	int numEntriesInFirstBlock;
	int currentEntryOffset;
	char currentFilePath[MAX_FILE_NAME_SIZE*10];

	/*Se o número total de entradas neste diretório for menor que o número de entradas que cabem em um bloco,
	* significa que o número de entradas no primeiro bloco será o número total de entradas, visto que tem espaço sobrando
	  Caso contrário, o número de entradas no primeiro bloco será necessariamente o máximo que cabe nele*/
	if(maxEntryCountFirstBlock > numEntriesInDirectory) numEntriesInFirstBlock = numEntriesInDirectory;
	else numEntriesInFirstBlock = maxEntryCountFirstBlock;

	#ifdef VERBOSE_DEBUG
		printf("[rmdir2]Iterando sobre as entradas do primeiro bloco\n");
	#endif

	for(currentEntryIndex = 0; currentEntryIndex < numEntriesInFirstBlock; currentEntryIndex++){
		currentEntryOffset = firstBlockFirstEntryOffset+entrySize*currentEntryIndex;
		currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
		/*Se a entrada for inválida, apenas ignoramos ela*/
		if(currentEntry.isValid){
			/*Primeiramente, definimos o pathname do arquivo da entrada atual
			* Este nome será usado na chamada de delete2 ou rmdir que remove este arquivo */
			strcpy(currentFilePath, pathname);
			strcat(currentFilePath, "/");
			strcat(currentFilePath, currentEntry.name);
			if(currentEntry.fileType = FILE_TYPE_REGULAR){
				if(delete2(currentFilePath) != 0){
					#ifdef VERBOSE_DEBUG
						printf("[rmdir2]Erro ao tentar apagar o arquivo %s\n", currentFilePath);
					#endif
					destroyBuffer(blockBuffer);
					return -1;
				}
			} else if(currentEntry.fileType == FILE_TYPE_DIRECTORY){
				if(rmdir2(currentFilePath) != 0){//chamada recursiva
					#ifdef VERBOSE_DEBUG
						printf("[rmdir2]Erro ao tentar apagar o diretorio %s\n", currentFilePath);
					#endif
					destroyBuffer(blockBuffer);
					return -1;
				}
			}
			memset(currentFilePath,0,strlen(currentFilePath)); //"limpamos" a string para evitar bugs
		}
	}

	/*Agora checamos se o diretório tem mais blocos. Se tiver, vamos iterar sobre eles apagando tudo.
	* Caso contrário, vamos direto chamar delete2 para este diretório*/
	if(directorySizeInBlocks > 1){
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
			printf("[rmdir2]Iniciando a iterar sobre os outros blocos do diretorio\n");
		#endif

		for(currentBlockNumber = 1; currentBlockNumber < directorySizeInBlocks; currentBlockNumber++){
			
			if(readBlock(nextBlockPointer, blockBuffer) != 0){
				#ifdef VERBOSE_DEBUG
					printf("[rmdir2] Erro ao tentar ler o bloco %d\n",nextBlockPointer);
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

			/*Agora iteramos sobre todas as entradas deste bloco apagando cada arquivo*/
			for(currentEntryIndex = 0; currentEntryIndex<numEntriesInCurrentBlock; currentEntryIndex++){
				
				currentEntryOffset = otherBlocksFirstEntryOffset+entrySize*currentEntryIndex;
				
				currentEntry = *(DirRecord*)(&blockBuffer[currentEntryOffset]);
				if(currentEntry.isValid){
					/*Primeiramente, definimos o pathname do arquivo da entrada atual
					* Este nome será usado na chamada de delete2 ou rmdir que remove este arquivo */
					strcpy(currentFilePath, pathname);
					strcat(currentFilePath, "/");
					strcat(currentFilePath, currentEntry.name);
					if(currentEntry.fileType = FILE_TYPE_REGULAR){
						if(delete2(currentFilePath) != 0){
							#ifdef VERBOSE_DEBUG
								printf("[rmdir2]Erro ao tentar apagar o arquivo %s\n", currentFilePath);
							#endif
							destroyBuffer(blockBuffer);
							return -1;
						}
					} else if(currentEntry.fileType == FILE_TYPE_DIRECTORY){
						if(rmdir2(currentFilePath) != 0){//chamada recursiva
							#ifdef VERBOSE_DEBUG
								printf("[rmdir2]Erro ao tentar apagar o diretorio %s\n", currentFilePath);
							#endif
							destroyBuffer(blockBuffer);
							return -1;
						}
					}

					memset(currentFilePath,0,strlen(currentFilePath)); //"limpamos" a string para evitar bugs
				}
			}

			/*Pegamos o ponteiro para o próximo bloco, atualizamos o número de entradas faltando
			*  e seguimos iterando (ou saímos do loop, se for o último bloco)*/

			entriesRemaining = entriesRemaining - maxEntryCountOtherBlocks; //o bloco atual necessariamente está cheio
			nextBlockPointer = *(unsigned int*)blockBuffer;
		}
	}

	if(delete2(pathname) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[rmdir2]Erro ao apagar este diretorio %s\n", pathname);
		#endif
		destroyBuffer(blockBuffer);
		return -1;
	}
	
	/*Se a função chegou até aqui, o diretório e todo seu conteúdo estão apagados,
	* basta liberar o buffer e retornar */

	destroyBuffer(blockBuffer);
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry) {
	/* ERRADO
	if(T2FSInitiated==0){
		initT2FS();
	}
	
	if ((handle >= 0) && (handle < 9))
	{
		strcpy(dentry->name, open_files[handle].fileRecord.name);
		dentry->fileType = open_files[handle].fileRecord.fileType;
		dentry->fileSize = open_files[handle].fileRecord.fileSize;
		return 0;
	}

	return -1;
	*/
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle) {
	
	if(T2FSInitiated==0){
		initT2FS();
	}
	
	if((0 <= handle) && (handle <= 9)){
		open_directories[handle].isValid = false;
		return 0;
	}
	
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um 
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename) {
	if(T2FSInitiated==0){
		initT2FS();
	}
	return -1;
}


