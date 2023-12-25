#include "scanner.h"

void initScanner(const char* source){
    scanner.start=source;
    scanner.current=source;
    scanner.line=1;
}