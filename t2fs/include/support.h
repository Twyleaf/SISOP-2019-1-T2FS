

#ifndef __SUPPORTT2FS___
#define __SUPPORTT2FS___

#define SUCCEEDED 0

typedef struct Dir_Record{
    char    name[32]; 					/* Nome do arquivo*/
    unsigned char    fileType;			/* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    //DWORD   fileSize;                   /* Numero de bytes do arquivo   */
	unsigned int	dataPointer	;		/* Ponteiro para primeiro bloco de dados do arquivo */
} DirRecord;

typedef struct Dir_Data{
    char    name[32]; 					/* Nome do arquivo*/
    unsigned char    fileType;			/* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
	unsigned int	entryCount;			/* Número de entradas no diretorio*/
	//unsigned int	fileSize;	Necessário implementar suporte para readdir. Colocar em entrada de dir?
} DirData;

typedef struct T2FS_Info{
	unsigned int blockSectionStart;
	unsigned int blockSize;

} T2FSInfo;

typedef struct OpenFileData{
	unsigned int first_block;
	char file_name[59];
	int pointer_to_current_byte;
}


extern int T2FSInitiated;
extern short diskVersion;
extern short sectorSize;
extern short partitionTableFirstByte;
extern short partitionCount;
extern int firstSectorPartition1;
extern int lastSectorPartition1;
extern int blockSectionStart;
extern int blockSize;
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

int getFileNameAndPath(char *pathname, char *path, char *name);

void allocateBlock();

int writeDirData(int firstBlockNumber, DirData newDirData);

#endif
