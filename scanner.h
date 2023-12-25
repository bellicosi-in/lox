#ifndef clox_scanner_h

#define clox_scanner_h

typedef struct{
    const char* start;
    const char* current;
    int line;
}Scanner;

Scanner scanner;

void initScanner(const char* source);



#endif