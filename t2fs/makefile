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
OBJS= $(LIB_DIR)/apidisk.o $(BIN_DIR)/bitmap.o $(BIN_DIR)/support.o  $(BIN_DIR)/t2fs.o 

all:regra1 regra2 regra3 regraLib


testeObj:
	$(CC) -c teste/testeDir.c src/t2fs.c src/support.c src/bitmap.c -Wall -w
testeExe:
	$(CC) testeDir.o t2fs.o support.o bitmap.o lib/apidisk.o -o testes -lm

regra1: #dependências para a regra1
	$(CC) -c  $(SRC_DIR)/support.c -o $(BIN_DIR)/support.o 

regra2: #dependências para a regra2
	$(CC) -c $(SRC_DIR)/bitmap.c -o $(BIN_DIR)/bitmap.o 
	
regra3: #dependências para a regra3
	$(CC) -c $(SRC_DIR)/t2fs.c -o $(BIN_DIR)/t2fs.o 

regraLib: #dependências para a regraLib
	ar crs $(LIB_DIR)/libcthread.a $(OBJS)

clean:
	rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~

