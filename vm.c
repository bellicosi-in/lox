#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "vm.h"
#include "debug.h"
#include "memory.h"
#include "compiler.h"


/* We declare a single global variable to avoid the chore of passing aroung a pointer to it.*/
/* when you run clox, it starts up the VM before it creates that hand authored chunk from the last chapter.*/

VM vm;

static Value clockNative(int argCount, Value* args){
    return NUMBER_VAL((double)clock()/CLOCKS_PER_SEC);
}

void resetStack(){
    vm.stackTop=vm.stack;
    vm.frameCount = 0;
}

static void runtimeError(const char* format, ...){
    va_list args;
    va_start(args,format);
    vfprintf(stderr,format,args);
    va_end(args);
    fputs("\n",stderr);

    for(int i = vm.frameCount - 1;i>=0; i++){
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        //calculates the offset for each function frame for the particular instruction
        size_t instruction = frame->ip - function->chunk.code -1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if(function->name == NULL){
            fprintf(stderr, "script\n");

        }else{
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    } 
    resetStack();
}
static void defineNative(const char* name, NativeFn function){
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals,AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}



void initVM(){
    resetStack();
    vm.objects = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
    
}

void freeVM(){
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
    

}

void push(Value value){
    *vm.stackTop=value;
    vm.stackTop++;
}

Value pop(){
    vm.stackTop--;
    return *vm.stackTop;
}


//to peek if the value is actually a number, we need to go to the stack, and check the value. this function does that. distance is for how far down
//the stack we can go.
static Value peek(int distance){
    return vm.stackTop[-1-distance];
}

static bool call(ObjClosure* closure, int argCount){
    if(argCount!= closure->function->arity){
        runtimeError("expected %d arguments but got %d", closure->function->arity, argCount);
        return false;
    }
    if(vm.frameCount == FRAMES_MAX){
        runtimeError("Stack OVerflow");
        return false;
    }
    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

//whenever a function is called, we check if the function called is a valid object.
static bool callValue(Value callee, int argCount){
    if(IS_OBJ(callee)){
        switch(OBJ_TYPE(callee)){
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee),argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
                
            
            default: 
                break;
        }
        
    }
    runtimeError("Can only call functions and classes");
    return false;
}

static bool isFalsey(Value value){
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}


//to concatenate two strings, we allocate separate memory and then allocate it on the heap.
static void concatenate(){
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    int length = a->length + b->length;
    char* chars= ALLOCATE(char,length+1);
    memcpy(chars,a->chars,a->length);
    memcpy(chars+a->length,b->chars,b->length);
    chars[length]='\0';

    ObjString* result = takeString(chars,length);
    push(OBJ_VAL(result));

}

static InterpretResult run(){
    CallFrame* frame = &vm.frames[vm.frameCount-1];
  /* each turn through the loop in run we execute a sinlge bytecode instruction. */
#define READ_BYTE() (*frame->ip++)


#define READ_SHORT() (frame->ip+=2, (uint16_t)((frame->ip[-2] << 8)| frame->ip[-1]))
/* reads the next byte from the bytecode abd treats the resulting number as an index and looks up the corresponding value in the chunk's constant table.*/
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType,op) do{ \
                        if(!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))){ \
                            runtimeError("Operands must be numbers."); \
                            return INTERPRET_RUNTIME_ERROR;\
                        } \
                        double b = AS_NUMBER(pop()); \
                        double a = AS_NUMBER(pop()); \
                        push(valueType(a op b)); \
                        }while(false)
    for(;;){
#ifdef DEBUG_TRACE_EXECUTION


/* this disassembles instructions dynamically on the fly. 
When you subtract vm.chunk->code from vm.ip, what you're calculating is the number of bytes
 between the start of the bytecode array and the current instruction. 
 This difference gives the offset of the current instruction within the bytecode.*/


        disassembleInstruction(&frame->closure->function->chunk,(int)(frame->ip-frame->function->chunk.code));

        /*stack tracing*/
        printf("    ");
        for(Value* slot=vm.stack;slot<vm.stackTop;slot++){
            printf("[  ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
#endif
        uint8_t instruction;

        //given an OPCODE , we need to get to the right C code that implements the semantics for that particular bytecode(OPCODE).
        //This process is called decoding or dispatching the information.
        
        switch(instruction=READ_BYTE()){
            case OP_CONSTANT:{
                Value constant = READ_CONSTANT();
                push(constant);
                // printValue(constant);
                // printf("\n");
                break;
            }

            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false));break;
            case OP_POP: pop(); break;

            // It takes a single-byte operand for the stack slot where the local lives. 
            //It loads the value from that index and then pushes it on top of the stack where later instructions can find it.
            case OP_GET_LOCAL:{
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }

            case OP_SET_LOCAL:{
                uint8_t slot= READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            //getting the global variable
            case OP_GET_GLOBAL:{
                ObjString* name= READ_STRING();
                Value value;
                if(!tableGet(&vm.globals,name,&value)){
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            //setting the global variable for the first time
            case OP_DEFINE_GLOBAL:{
                ObjString* name= READ_STRING();
                tableSet(&vm.globals,name,peek(0));
                pop();
                break;
            }

            //setting the global variable
            case OP_SET_GLOBAL:{
                ObjString* name= READ_STRING();
                if(tableSet(&vm.globals,name,peek(0))){
                    tableDelete(&vm.globals,name);
                    runtimeError("undefined variable '%s'.",name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD:{
                if(IS_STRING(peek(0))&& IS_STRING(peek(1))){
                    concatenate();
                }else if(IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))){
                    double b=AS_NUMBER(pop());
                    double a= AS_NUMBER(pop());
                    push(NUMBER_VAL(a+b));

                } else {
                    runtimeError("Operands must be two numbers or two strings");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT:{
                BINARY_OP(NUMBER_VAL,-);
                break;
            }
            case OP_MULTIPLY:{
                BINARY_OP(NUMBER_VAL,*);
                break;
            }
            case OP_DIVIDE:{
                BINARY_OP(NUMBER_VAL,/);
                break;
            }
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:{
                if(!IS_NUMBER(peek(0))){
                    runtimeError("Operand must be a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }

            case OP_PRINT:{
                printValue(pop());
                printf("\n");
                break;
            }
            //the read_Short macro reads the pffset from the bytecode stream updated by the emitJump function.
            case OP_JUMP_IF_FALSE:{
                uint16_t offset = READ_SHORT();
                if(isFalsey(peek(0))) frame->ip+=offset;
                break;
            }
            case OP_JUMP:{
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_LOOP:{
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            //whenevr a function is called.
            case OP_CALL:{
                int argCount = READ_BYTE();
                if(!callValue(peek(argCount),argCount)){
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE:{
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure* closure =  newClosure(function);
                push(OBJ_VAL(closure));
                break;
            }
            case OP_RETURN:{
                Value result = pop();
                vm.frameCount--;
                if(vm.frameCount == 0){
                    pop();
                    return INTERPRET_OK;
                }
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;

            }

            
        }
    }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

/* this is the main entrypoint into vm. the VM springs into action when we command it to interpret a chunk of bytecode. we pass in the string of source code.*/

InterpretResult interpret(const char* source){
    /* we pass over an empty chunk, initialize it and then pass it over to the compiler that fills it with bytecode.*/

     ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    //wrapping the global funciton in a closure before we pass it onto the vm.
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure,0);
    return run();

}