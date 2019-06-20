#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"


int main(){
	printf("\nRetorno format2: %d\n",format2(2));
	FILE2 handle1 = create2("/arquivo");

    char* buffer = (unsigned char*)malloc(sizeof(unsigned char*)*10);
    buffer[0] = 't';
    buffer[1] = 'a';
    buffer[2] = ' ';
    buffer[3] = 'e';
    buffer[4] = 'r';
    buffer[5] = 'r';
    buffer[6] = 'a';
    buffer[7] = 'd';
    buffer[8] = 'o';
    buffer[9] = '!';

    read2(handle1, buffer, 10);

    printf("conteudo lido do arquivo:\n");
    int i =0;
    for(i = 0; i<10; i++){
        printf("%c",buffer[i]);
    }
    printf("\n");
	return 0;
}
