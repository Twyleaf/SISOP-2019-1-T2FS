

#ifndef __SUPPORTT2FS___
#define __SUPPORTT2FS___

typedef struct Dir_Record{
    char    name[32]; 					/* Nome do arquivo*/
    BYTE    fileType;                   /* Tipo do arquivo: regular (0x01) ou diretório (0x02) */
    //DWORD   fileSize;                   /* Numero de bytes do arquivo   */
	DWORD	dataPointer					/* Ponteiro para primeiro bloco de dados do arquivo */
} DirRecord;


extern int T2FSInitiated;
extern short diskVersion;
extern short sectorSize;
extern short partitionTableFirstByte;
extern short partitionCount;
extern int firstSectorPartition1;
extern int lastSectorPartition1;
extern int blockSectionStart;
extern int blocksInPartition;

int initT2FS();

int formatFSData(int sectorsPerBlock);

void printPartition1DataSection();

int getFileBlock(char *filename);

int goToFileFromParentDir();


#endif
