

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

typedef struct {
    char file_name[32]; 
    unsigned char file_type;                   
    int file_size;                  
    int first_block;
    int number_of_blocks;
} Register;

typedef struct Open_File_Data{
	//Register register;
	int pointer_to_current_byte;
} OpenFileData;


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

int getFileNameAndPath(char *pathname, char *path, char *name);

int allocateBlock();

int writeDirData(int firstBlockNumber, DirData newDirData);

int insertEntryInDir(int dirFirstBlockNumber,DirRecord newDirEntry);

int getFirstSectorOfBlock(int blockNumber);

void writeNextBlockPointer(int blockNumber,unsigned int pointer);

int writeRecordInBlock(DirRecord newDirEntry, int blockToWriteEntry, int dirToInsertOffset);

int getLastBlockInFile(unsigned int fileFirstBlockNumber);

int getSectorsPerBlock(int blockSizeBytes);

int getBytesForBitmap();


#endif
