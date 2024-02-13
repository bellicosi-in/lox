#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

//to figure out the type of the value of the Obj
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_CLOSURE(value) isObjType(value,OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType(value,OBJ_FUNCTION)
#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define IS_STRING(value) isObjType(value, OBJ_STRING)


#define AS_CLOSURE(value) ((ObjClosure*)AS_OBJ(value))
//returns the ObjFunction* pointer
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
//returns the OBJ_native pointer
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value))->function)
//returns the ObjString* pointer to the objstring pointer
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))

//returns the character array
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} ObjType;


//all the data types that live on the stack is called Obj.
struct Obj {
    ObjType type;
    struct Obj* next;
};


//struct for the function object
typedef struct{
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString* name;

}ObjFunction;

//The native function takes the argument count and a pointer to the first argument on the stack. 
// It accesses the arguments through that pointer. Once it’s done, it returns the result value.
typedef Value (*NativeFn) (int argCount, Value* args);

typedef struct{
    Obj obj;
    NativeFn function;

} ObjNative;

// Given an ObjString*, you can safely cast it to Obj* and then access the type field from it. Every ObjString “is” an Obj in the OOP sense of “is”.
// You can take a pointer to a struct and safely convert it to a pointer to its first field and back.
struct ObjString {
    Obj      obj;
    int      length;
    char*    chars;
    uint32_t hash;
};
// our runtime upvalue structure is an ObjUpvalue with the typical Obj header field.
// Following that is a location field that points to the closed-over variable. 
typedef struct ObjUpvalue{
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
}ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;

}ObjClosure;

ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction();
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
void printObject(Value value);


//to check if the value is of the right Obj type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif

