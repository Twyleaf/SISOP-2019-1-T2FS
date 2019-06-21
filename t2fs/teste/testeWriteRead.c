#include <stdio.h>
#include <string.h>

#include "../include/t2fs.h"
#include "../include/support.h"
#include "../include/bitmap.h"


int main(){
	printf("\nRetorno format2: %d\n",format2(2));
	FILE2 handle1 = create2("/arquivo");

    char* buffer = (unsigned char*)malloc(sizeof(unsigned char*)*10);
    buffer[0] = 'o';
    buffer[1] = 'l';
    buffer[2] = 'a';
    buffer[3] = ' ';
    buffer[4] = 'm';
    buffer[5] = 'u';
    buffer[6] = 'n';
    buffer[7] = 'd';
    buffer[8] = 'o';
    buffer[9] = '!';

    write2(handle1, buffer, 10);

    seek2(handle1, 0);

    buffer[0] = 'k';
    buffer[1] = 'k';
    buffer[2] = 'k';
    buffer[3] = 'k';
    buffer[4] = 'k';
    buffer[5] = 'k';
    buffer[6] = 'k';
    buffer[7] = 'k';
    buffer[8] = 'k';
    buffer[9] = 'k';

    read2(handle1, buffer, 10);

    printf("conteudo lido do arquivo:\n");
    int i =0;
    for(i = 0; i<10; i++){
        printf("%c",buffer[i]);
    }
    printf("\n");
	return 0;
}
