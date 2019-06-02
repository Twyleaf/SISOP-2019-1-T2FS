#include <stdio.h>
#include "../include/t2fs.h"
#include "../include/support.h"

#define SECTOR_SIZE 256

int main(){
	int Ret = format2(2);
	printPartition1DataSection();
	printf("\nRetorno: %d\n",Ret);
	return 0;
}
