#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"


int main(){
	printf("\nRetorno format2: %d\n",format2(2));
	FILE2 handle1 = create2("/arquivo");

    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char*)*10);
    buffer[0] = 'a';
    buffer[1] = 'a';
    buffer[2] = 'a';
    buffer[3] = 'a';

    read2(handle1, buffer, 10);
	return 0;
}
