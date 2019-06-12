#include <limits.h>    /* for CHAR_BIT */
//https://stackoverflow.com/questions/1225998/what-is-a-bitmap-in-c
enum { BITS_PER_WORD = sizeof(unsigned char) * CHAR_BIT };
#define WORD_OFFSET(b) ((b) / BITS_PER_WORD)
#define BIT_OFFSET(b)  ((b) % BITS_PER_WORD)

void setBit(unsigned char *bitmap, int n) { 
    bitmap[WORD_OFFSET(n)] |= ((unsigned char)1 << BIT_OFFSET(n)); //Bitmap original or 0...1..0,
	//							onde o 1 está onde o bit deve ser adicionado.
}

void clearBit(unsigned char *bitmap, int n) {
    bitmap[WORD_OFFSET(n)] &= ~((unsigned char)1 << BIT_OFFSET(n)); //Bitmap original and 1...0...1, 
	//							onde o 0 está onde o bit deve ser apagado.
}

int getBit(unsigned char *bitmap, int n) {
    unsigned char bit = bitmap[WORD_OFFSET(n)] & ((unsigned char)1 << BIT_OFFSET(n));//Bitmap original and 0...1..0, 
    return bit != (unsigned char)0; //retorna 0 se determinado bit era 0, 1 se era 1.
}

int setAndReturnBit(unsigned char *bitmap, int bufferStart, int bufferSizeBytes){
	int currentByte=bufferStart;
	int currentBit;
	int bitPositionInBuffer, bitPositionInBitmap;
	for(currentByte = bufferStart; currentByte < bufferSizeBytes; currentByte++){
		if(bitmap[currentByte]!=UCHAR_MIN){
			for(currentBit = 0; currentBit < BITS_PER_WORD; currentBit++){
				bitPositionInBuffer = ((bufferStart+currentByte)*CHAR_BIT)+currentBit;
				bitPositionInBitmap = (currentByte*CHAR_BIT)+currentBit;
				if(getBit(bitmap, bitPositionInBuffer)==0){
					clearBit(bitmap, bitPositionInBuffer);
					return bitPositionInBitmap;
				}
			}
		}
	}
}