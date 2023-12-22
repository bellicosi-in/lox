CC = gcc
CFLAGS = -std=c99 -Wall -g

main: main.o debug.o memory.o chunk.o value.o vm.o
		gcc -o main main.o debug.o memory.o chunk.o value.o vm.o

main.o: main.c debug.h chunk.h common.h memory.h vm.h
		gcc -c main.c
vm.o: vm.c value.h chunk.h common.h
		gcc -c vm.c
debug.o: debug.c debug.h chunk.h
		gcc -c debug.c
value.o:value.c memory.h common.h
		gcc -c value.c
memory.o : memory.c memory.h common.h
		gcc -c memory.c

chunk.o : chunk.c chunk.h common.h
		gcc -c chunk.c
