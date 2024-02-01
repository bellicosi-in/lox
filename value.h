#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
}ValueType;

//to store the double value so that we can change the type depending on the requirements without needing to change it everywhere.
/* Each chunk will carry with it a list of the values that appear as literals in the program. */
typedef struct{
    ValueType type;
    union{
        bool boolean;
        double number;
        Obj* obj;
    }as;
}Value;

//to check the type of the value.
#define IS_BOOL(value) ((value).type==VAL_BOOL)
#define IS_NIL(value) ((value).type==VAL_NIL)
#define IS_NUMBER(value) ((value).type==VAL_NUMBER)
#define IS_OBJ(value) ((value).type==VAL_OBJ)


//converting the Value type value back into a C type value
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)


//converting the suitable value into the appropriate Value struct type
#define BOOL_VAL(value) ((Value){VAL_BOOL,{.boolean=value}})
#define NIL_VAL ((Value){VAL_NIL,{.number=0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER,{.number=value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ,{.obj=(Obj*)object}})

typedef struct{
    int count;
    int capacity;
    Value* values;
}ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array,Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);
    
    




#endif