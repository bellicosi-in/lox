#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value.h"

// All the opcodes are defined here - each isnstruction has one byte operand.
// each opcode determines how many bytes it has and what they mean. A simple operation like return may have no operands.  each time
// a new opcode is added to clox, its instruction format needs to be specified. 

typedef enum{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
}OpCode;


// we need to store data along with the instructions - this is what the struct is for. along with the OP CODE, there are other data associated with it
// which will be dynamic, hence the use of dynamic arrays.
typedef struct{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
}Chunk;


void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk,uint8_t byte,int line);
void freeChunk(Chunk* chunk);
int addConstant(Chunk* chunk,Value value);

#endif