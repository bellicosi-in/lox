//building a new hash function

#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "value.h"

//its a key value pair.
typedef struct{
    ObjString* key;
    Value value;
}Entry;


//the table that stores the entries along with the count and hte capacity
typedef struct{
    int count;
    int capacity;
    Entry* entries;

}Table;



void initTable(Table* table);
void freeTable(Table* table);
bool tableGet(Table* table, ObjString* key, Value* value);
bool tableSet(Table* table, ObjString* key, Value value); 
bool tableDelete(Table* table,ObjString* key);
void tableAddAll(Table* from, Table* to);
ObjString* tableFindString(Table* table, const char* chars, int length , uint32_t hash);


#endif