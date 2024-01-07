CC = gcc
CFLAGS = -std=c99 -Wall -g

main: main.o debug.o memory.o chunk.o value.o vm.o compiler.o object.o
		gcc -o main main.o debug.o memory.o chunk.o value.o vm.o compiler.o object.o

main.o: main.c debug.h chunk.h common.h memory.h vm.h
		gcc -c main.c
vm.o: vm.c value.h chunk.h common.h
		gcc -c vm.c


compiler.o: compiler.c compiler.h object.h memory.h debug.h scanner.h chunk.h
		gcc -c compiler.c

debug.o: debug.c debug.h chunk.h
		gcc -c debug.c
value.o:value.c memory.h common.h
		gcc -c value.c
memory.o : memory.c memory.h common.h object.h table.h
		gcc -c memory.c

table.o: table.c table.h object.h vm.h chunk.h memory.h 
		gcc -c table.c
object.o: object.c object.h memory.h value.h vm.h chunk.h
		gcc -c object.c
chunk.o : chunk.c chunk.h common.h
		gcc -c chunk.c
