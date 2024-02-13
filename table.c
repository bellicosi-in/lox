#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table){
    table->count=0;
    table->capacity=0;
    table->entries=NULL;
}

void freeTable(Table* table){
    FREE_ARRAY(Entry,table->entries,table->capacity);
    initTable(table);
}

//the function for finding a key value pair in the hash table or for finding a particular bucket to store that key value pair.
static Entry* findEntry(Entry* entries,int capacity, ObjString* key){
    uint32_t index = key->hash % capacity;
    Entry* tombstone = NULL;
    for(;;){
        Entry* entry = &entries[index];
        if(entry->key == NULL){
            if(IS_NIL(entry->value)){
                return tombstone!=NULL ? tombstone:entry;
            } else {
                //we found a tombstone
                if(tombstone == NULL) tombstone = entry;
            }

        } else if (entry->key == key){
            //we found the key
            return entry;
        }
        index=(index+1)%capacity;
    }
}


//to get a particular key value pair.
bool tableGet(Table* table, ObjString* key, Value* value){
    if(table->count==0) return false;

    Entry* entry = findEntry(table->entries,table->capacity,key);
    if(entry->key ==NULL) return false;

    *value=entry->value;
    return true;
}
//used to resize the hash table. create a new array of entries witht he new capacity and rehash or reinset the entries from the old table into the new one.
static void adjustCapacity(Table* table, int capacity){
    Entry* entries = ALLOCATE(Entry,capacity);
    for(int i=0;i<capacity;i++){
        entries[i].key=NULL;
        entries[i].value=NIL_VAL;
    }
    table->count=0;
    for(int i=0;i<table->capacity; i++){
        Entry* entry = &table->entries[i];
        if(entry->key==NULL)continue;

        Entry* dest= findEntry(entries,capacity,entry->key);
        dest->key=entry->key;
        dest->value = entry->value;
        table->count++;
    }
    FREE_ARRAY(Entry,table->entries,table->capacity);
    table->entries=entries;
    table->capacity=capacity;
}

//adds the given key/value pair to the given hash table. also increases the count only when the key is not present in the table.
bool tableSet(Table* table,ObjString* key,Value value){
    if(table->count+1 > table->capacity * TABLE_MAX_LOAD){
        int capacity = GROW_CAPACITY(table->capacity);
        adjustCapacity(table,capacity);
    }
    Entry* entry = findEntry(table->entries, table->capacity,key);
    bool isNewKey= entry->key==NULL;
    if (isNewKey && IS_NIL(entry->value)) table->count++;


    entry->key=key;
    entry->value=value;
    return isNewKey;
}



bool tableDelete(Table* table, ObjString* key){
    if(table->count==0) return false;
    //find the entry
    Entry* entry=findEntry(table->entries,table->capacity,key);
    if(entry->key == NULL) return false;


    //place a tombstone
    entry->key=NULL;
    entry->value=BOOL_VAL(true);
    return true;
}

//to add all the entries from one hash tbale to another table.
void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->key != NULL) {
      tableSet(to, entry->key, entry->value);
    }
  }
}


//looks for the string in the interned table to check if its already available.
ObjString* tableFindString(Table* table,const char* chars,int length,uint32_t hash){
    if(table->count==0) return NULL;

    uint32_t index = hash % table->capacity;
    for(;;){
        Entry* entry = &table->entries[index];
        if(entry->key==NULL){
            //stop if we find an empty non-tombstone entry
            if(IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars,chars,length)==0){
            return entry->key;
        }
        index = (index + 1)%table->capacity;
    }
}


//for removing weak references
void tableRemoveWhite(Table* table){
    for(int i = 0; i< table->capacity; i++){
        Entry* entry = &table->entries[i];
        if(entry->key != NULL && !entry->key->obj.isMarked){
            tableDelete(table,entry->key);
        }
    }
}

//we walk the entry array. for each one we mark its value. we also mark the key strings for each entry since the GC manages those strings too.

void markTable(Table* table){
    for(int i=0;i<table->capacity;i++){
        Entry* entry = &table->entries[i];
        markObject((Obj*)entry->key);
        markValue(entry->value);
    }
}