#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"

#define STACK_MAX 256

typedef struct{
    Chunk* chunk;
    /* the ip tracks the bytecode through the chunk thats passed to the vm. ip always points to the instruction about to be executed. */
    uint8_t* ip;

/*the temporary values we need to traxk have stack like behavior. when an instruction produces a value it pushes it onto the stack to manage them.
 when an instruction produces a value it pushes it onto the stack. when it needs to consume one or more values, it gets them by popping them off the stack*/
    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings;
    Obj* objects;
    
}VM;
extern VM vm;


/* the vm runs the chunk and then responds a value from this enum. */
typedef enum{
    INTERPRET_OK,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_COMPILE_ERROR
}InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();






#endif