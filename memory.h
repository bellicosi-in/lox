#ifndef memory_clox_h 
#define memory_clox_h

#include "common.h"
#include "object.h"

/* */
#define ALLOCATE(type,count)\
            (type*)reallocate(NULL,0,sizeof(type)*count)


#define FREE(type,pointer) reallocate(pointer,sizeof(type),0)



/* to grow the capacity of a dynamic array in the multiples of 8 bytes. */
#define GROW_CAPACITY(capacity) (capacity<8?8:(capacity)*2)

/* using the function to get the pointer from the grown array, we use it to grow the array.*/
#define GROW_ARRAY(type,pointer,oldCount,newCount)\
                    (type*)reallocate(pointer,sizeof(type)*(oldCount),\
                    sizeof(type)*(newCount))


// this is the preprocessor directive that is used to free up the dynamic arrays.
#define FREE_ARRAY(type,pointer,oldCount)\
                    reallocate(pointer,sizeof(type)*(oldCount),0)


// this is the function that basically handles all the dynamic memory management in clox. it basically is the backend for the memory reallocation.
void* reallocate(void* pointer,size_t oldSize, size_t newSize);            


void freeObjects();
#endif