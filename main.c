#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"


static void repl(){
    char line[1024];
    for(;;){
        printf("> ");
        if(!fgets(line,sizeof(line),stdin)){
            printf("\n");
            break;
        }
        interpret(line);
    }
}


static void runFile(const char* path){
    char* source= readFile(path);
    InterpretResult result=interpret(source);
    free(source);
    if(result==INTERPRET_COMPILE_ERROR) exit(65);
    if(result==INTERPRET_RUNTIME_ERROR) exit(70);


}


//reading the whole file and allocating memory for it. this is done through a trick of seeking to the end of the file and ftelling the size.
static char* readFile(const char* path){

    FILE* file=fopen(path,"rb");
    if(file==NULL){
        fprintf(stderr,"Could not open file \"%s\".\n",path);
        exit(74);

    }
    fseek(file,0L,SEEK_END);
    size_t fileSize=ftell(file);
    rewind(file);

    char* buffer= (char*)malloc(fileSize+1);
    if(buffer==NULL){
        fprintf(stderr,"not enough memory to read \"%s\".\n",path);
        exit(74);
    }
    size_t bytesRead=fread(buffer,sizeof(char),fileSize,file);
    buffer[bytesRead]='\0';
    if(bytesRead<fileSize){
        fprintf(stderr,"could not read the whole file \"%s\".\n",path);
        exit(74);
    }


}


int main(int argc, char* argv[]){
    initVM();
    

    if(argc==1){
        repl();
    }else if (argc==2){
        runFile(argv[1]);
    } else {
        fprintf(stderr,"Usage: clox[path]\n");
        exit(64);
    }
    
   
   
   
   
   
   
   
   
   
   
   
   
   
   
    
    freeVM();
    

    return 0;
}