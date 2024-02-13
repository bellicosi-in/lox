#include "chunk.h"
#include "memory.h"
#include "vm.h"


/* Initialize the new chunk. the dynamic array starts off completely empty.*/

/* for the line number, we store a separate array of integers that parallels the btyecode. when a runtime error occurs,
we look up the line number at the same index as the current instruction's offset in the code array.*/
void initChunk(Chunk* chunk){
    chunk->count=0;
    chunk->capacity=0;
    chunk->code=NULL;
    chunk->lines=NULL;
    initValueArray(&chunk->constants);
}


/* to write to the chunk. If the space allocated for it through malloc doesn't fit, we can also expand the array. basically updating the chunk*/
void writeChunk(Chunk* chunk,uint8_t byte,int line){
    if(chunk->capacity<chunk->count+1){
        int oldCapacity = chunk->capacity;
        chunk->capacity=GROW_CAPACITY(oldCapacity);
        chunk->code=GROW_ARRAY(uint8_t,chunk->code,oldCapacity,chunk->capacity);
        chunk->lines=GROW_ARRAY(int,chunk->lines,oldCapacity,chunk->capacity);
    }
    chunk->code[chunk->count]=byte;
    chunk->lines[chunk->count]=line;
    chunk->count++;

}

//adding the constant value to the constant pool of the array and returning the index
// when the VM loads the OP_CONSTANT instruction, it also loads the constant for use. 
int addConstant(Chunk* chunk,Value value){
    push(value);
    writeValueArray(&chunk->constants,value);
    pop();
    return chunk->constants.count-1;
}


void freeChunk(Chunk* chunk){
    FREE_ARRAY(uint8_t,chunk->code,chunk->capacity);
    FREE_ARRAY(int,chunk->lines,chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
    
}