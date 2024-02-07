#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

//to figure out the type of the value of the Obj
#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value,OBJ_FUNCTION)
#define IS_STRING(value) isObjType(value, OBJ_STRING)


//returns the ObjFunction* pointer
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))
//returns the ObjString* pointer to the objstring pointer
#define AS_STRING(value)  ((ObjString*)AS_OBJ(value))

//returns the character array
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_FUNCTION,
    OBJ_STRING,
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
    Chunk chunk;
    ObjString* name;

}ObjFunction;

// Given an ObjString*, you can safely cast it to Obj* and then access the type field from it. Every ObjString “is” an Obj in the OOP sense of “is”.
// You can take a pointer to a struct and safely convert it to a pointer to its first field and back.
struct ObjString {
    Obj      obj;
    int      length;
    char*    chars;
    uint32_t hash;
};


ObjFunction* newFunction();
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
void printObject(Value value);


//to check if the value is of the right Obj type.
static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif

