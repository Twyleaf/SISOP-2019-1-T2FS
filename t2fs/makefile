#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
# 

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

all:

testeObj:
	$(CC) -c teste/testeRmdir.c src/t2fs.c src/support.c src/bitmap.c -Wall -w
testeExe:
	$(CC) testeRmdir.o t2fs.o support.o bitmap.o lib/apidisk.o -o testes -lm

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~


