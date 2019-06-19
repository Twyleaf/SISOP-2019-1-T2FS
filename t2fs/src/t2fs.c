
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

//=========================================NOVO=====================================
#define VERBOSE_DEBUG
//para desativar os prints de debug, basta comentar esta linha
//=====================================================================================

/*-----------------------------------------------------------------------------
Função:	Informa a identificação dos desenvolvedores do T2FS.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size) {
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
		if(delete2(filename)>0){
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


	/*CONSIDERANDO QUE O CAMINHO RETORNADO POR getFileNameAndPath() TERMINA EM / */
	/*Trocamos a barra no fim do caminho para o arquivo por um caractere de fim de string,
	assim gerando o caminho para o diretório onde está o arquivo */
	int pathIndex = 0;
	while(path[pathIndex] != '\0'){
		pathIndex++;
	}
	path[pathIndex-1] = '\0';

	/*Usamos GetFileBlock para conseguir o primeiro bloco do diretório */
	int directoryFirstBlockNumber = getFileBlock(path);

	/*Usamos  SetDirectoryEntryAsInvalid para invalidar a entrada de diretório correspondente a este arquivo*/
	if(SetDirectoryEntryAsInvalid(directoryFirstBlockNumber, name) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[delete2] Erro ao invalidar a entrada de diretorio do arquivo %s\n",filename);
		#endif
		return -1;
	}

	/*Usamos a  setFileBlocksAsUnused para invalidar os ponteiros dos blocos deste arquivo e também
	* configurar o bitmap para informar que estes blocos estão vazios*/
	int fileFirstBlockNumber = getFileBlock(filename); //descobre o número do primeiro bloco do arquivo
	if(setFileBlocksAsUnused(fileFirstBlockNumber) != 0){
		#ifdef VERBOSE_DEBUG
			printf("[delete2] Erro ao configurar blocos do arquivo %s como não utilizados\n",filename);
		#endif
		return -1;
	}
		
	/*Se a função chegou até aqui, o arquivo já está deletado e a função pode retornar */
	return 0;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um arquivo existente no disco.
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um arquivo.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a leitura de uma certa quantidade
		de bytes (size) de um arquivo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para realizar a escrita de uma certa quantidade
		de bytes (size) de  um arquivo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo. Remove do arquivo 
		todos os bytes a partir da posição atual do contador de posição
		(current pointer), inclusive, até o seu final.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Altera o contador de posição (current pointer) do arquivo.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset) {
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
	printf("[mkdir2] Bloco %d Alocando para o arquivo\n",firstBlockNumber);

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
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para alterar o CP (current path)
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para obter o caminho do diretório corrente.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função que abre um diretório existente no disco.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para ler as entradas de um diretório.
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para fechar um diretório.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle) {
	return -1;
}

/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com
		o nome dado por linkname (relativo ou absoluto) para um 
		arquivo ou diretório fornecido por filename.
-----------------------------------------------------------------------------*/
int ln2 (char *linkname, char *filename) {
	return -1;
}


