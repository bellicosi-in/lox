#ifndef clox_debug_h
#define clox_debug_h
#include "chunk.h"


/* A disassembler , given a blob of machine code, it spits out a textual listing of the instructions. */
void disassembleChunk(Chunk* chunk,const char* name);
int disassembleInstruction(Chunk* chunk,int offset);

#endif