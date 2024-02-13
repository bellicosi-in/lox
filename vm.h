#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "common.h"
#include "value.h"
#include "table.h"
#include "object.h"


#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

//to store the metadata for each function that has not been called.
//each call frame represents a single function call.
//the slots field points into the VM's value stack at the first slot that this function can use.
typedef struct{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
}CallFrame;

typedef struct{
    CallFrame frames[FRAMES_MAX];
    int frameCount;

/*the temporary values we need to traxk have stack like behavior. when an instruction produces a value it pushes it onto the stack to manage them.
 when an instruction produces a value it pushes it onto the stack. when it needs to consume one or more values, it gets them by popping them off the stack*/
    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings;
    Obj* objects;
    
}VM;

// The “object” module is directly using the global vm variable from the “vm” module, so we need to expose that externally..
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