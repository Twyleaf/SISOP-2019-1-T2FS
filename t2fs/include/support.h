

#ifndef __SUPPORTT2FS___
#define __SUPPORTT2FS___

#define SUCCEEDED 0
#define FILE_TYPE_REGULAR 0x01
#define FILE_TYPE_DIRECTORY 0x02

#include <stdbool.h>

typedef struct Dir_Record{
    bool isValid;                       /* NOVO bit de validade da entrada (indica se esta entrada pode se sobrscrita ou não)*/                     
    char    name[32]; 					/* Nome do arquivo*/
    unsigned char    fileType;			/* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    unsigned int   fileSize;                 /* Numero de bytes do arquivo  */
	unsigned int	dataPointer	;		/* Ponteiro para primeiro bloco de dados do arquivo */
} DirRecord;

typedef struct Dir_Data{
    char    name[32]; 					/* Nome do arquivo*/
    unsigned char    fileType;			/* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
	unsigned int	entryCount;			/* Número de entradas no diretorio*/
	unsigned int	fileSize;	        /* Necessário implementar suporte para readdir. Colocar em entrada de dir?*/
} DirData;

typedef struct T2FS_Info{
	unsigned int blockSectionStart;
	unsigned int blockSize;

} T2FSInfo;

typedef struct Open_File_Data{
	bool isValid;                       /*Para saber se este arquivo pode ser sobrescrito quando um novo for aberto */
    DirRecord fileRecord;               /*Cópia da entrada de diretório correspondente a este arquivo*/
	int pointerToCurrentByte;
	int parentDirBlock;					/*Bloco do diretório pai do arquivo*/
} OpenFileData;

typedef struct Open_Dir_Data{
	bool isValid;
	DirData fileData;
} OpenDirData;


typedef struct Block_And_Byte_Offset{
	unsigned int block;
	unsigned int byte;
} BlockAndByteOffset;

OpenFileData open_files[10]; //apenas 10 arquivos podem estar abertos simultaneamente
OpenDirData open_directories[255]; // não tem um limite de diretórios que possam estara abertos

extern int T2FSInitiated;
extern short diskVersion;
extern short sectorSize;
extern short partitionTableFirstByte;
extern short partitionCount;
extern int firstSectorPartition1;
extern int lastSectorPartition1;
extern int blockSectionStart;
extern int blockSize;
extern int rootDirBlockNumber;
extern int dirEntrySize;
extern int blockPointerSize;
//extern int blocksInPartition;

int initT2FS();

int formatFSData(int sectorsPerBlock);

int readFSInfo();

void printPartition1DataSection();

int getFileBlock(char *filename);

int goToFileFromParentDir();

int readBlock(int blockNumber,unsigned char* data);

int writeBlock(int blockNumber, unsigned char* data);

int getFileNameAndPath(char *pathname, char *path, char *name);

int allocateBlock();

int allocateRootDirBlock();

int writeDirData(int firstBlockNumber, DirData newDirData);

int insertEntryInDir(int dirFirstBlockNumber,DirRecord newDirEntry);

int getFirstSectorOfBlock(int blockNumber);

void writeNextBlockPointer(int blockNumber,unsigned int pointer);

int writeRecordInBlock(DirRecord newDirEntry, int blockToWriteEntry, int dirToInsertOffset);

int getLastBlockInFile(unsigned int fileFirstBlockNumber);

int getSectorsPerBlock(int blockSizeBytes);

int getBytesForBitmap();

unsigned char* createSectorBuffer();
unsigned char* createBlockBuffer();
unsigned char* createBitmapBuffer();
void destroyBuffer(unsigned char* buffer);

int setFileBlocksAsUnused(int firstBlock);

int SetDirectoryEntryAsInvalid(unsigned int directoryFirstBlockNumber, char* filename);

int fileExistsInDir(char* fileName, int parentDirFirstBlock);

int isPathnameAlphanumeric(char* pathname,int maxPathSize);

int getFileType(int firstBlockNumber);

int getDirectoryEntry(unsigned int directoryFirstBlockNumber, char* filename, DirRecord* dirRecordBuffer);

//=========================================FUNÇÕES PARA WRITE=======================================

int getOpenFileData(int handle,OpenFileData *openFileData);

int getCurrentPointerPosition(unsigned int currentPointer,unsigned int firstBlockOfFile,BlockAndByteOffset *blockAndByteOffset);

int getNewFileSize(unsigned int fileOriginalSize, unsigned int dataBeingWrittenSize, unsigned int currentPointer);

int writeData(char *buffer, int bufferSize, int blockToWriteOn, int blockToWriteOffset);

int writeCurrentFileData(int handle,OpenFileData openFileData);

int readFromBlockWithOffsetAndCutoff(unsigned int blockNumber, unsigned int offset, unsigned int cutoff, unsigned char* buffer);

int getPointerToNextBlock(unsigned int blockNumber, unsigned int* buffer);
int SetSizeOfFileInDir(unsigned int directoryFirstBlockNumber, char* filename, unsigned int newFileSize);

#endif
