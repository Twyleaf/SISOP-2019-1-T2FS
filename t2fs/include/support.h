

#ifndef __SUPPORTT2FS___
#define __SUPPORTT2FS___

extern int T2FSInitiated;
extern short diskVersion;
extern short sectorSize;
extern short partitionTableFirstByte;
extern short partitionCount;
extern int firstSectorPartition1;
extern int lastSectorPartition1;
extern int blockSectionStart;

int initT2FS();

int formatFSData(int sectorsPerBlock);


#endif
