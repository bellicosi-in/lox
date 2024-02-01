#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "scanner.h"



#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif


//building the parsing part of the compiler


typedef struct{
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
}Parser;



typedef enum {

    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;


//the function takes the bool argument but returns nothing.
typedef void (*ParseFn)(bool canAssign);


//struct for the parser table.
typedef struct{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
}ParseRule;

typedef struct {
    Token name;
    int depth;
}Local;

typedef struct{
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
}Compiler;

Parser parser;
Compiler* current = NULL;
Chunk* compilingChunk;

static Chunk* currentChunk(){
    return compilingChunk;
}


//this is the major function that handles the error
static void errorAt(Token* token, const char* message){
    if(parser.panicMode) return;
    parser.panicMode=true;
    fprintf(stderr,"[line %d] Error", token->line);
    if(token->type==TOKEN_EOF){
        fprintf(stderr," at end");
    } else if (token->type==TOKEN_ERROR) {
    
    }else{
        fprintf(stderr," at '%.*s' ", token->length,token->start);
    }
    fprintf(stderr,"%s\n",message);
    parser.hadError=true;
}

//this is called when the error is pertaining to the previous token.
static void error(const char* message){
    errorAt(&parser.previous,message);
}


//this is called when the error is pertaining to the current token.
static void errorAtCurrent(const char* message){
    errorAt(&parser.current,message);
}

//it advances us to the next valid token and also handles error if there is an error in the token. it steps through the token stream. it asks for the next token and stores it for later use.
static void advance(){
    parser.previous = parser.current;

    for(;;){
        parser.current=scanToken();
        if(parser.current.type!=TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);

    }
}


//it reads the next token but also validates that the token has an expected type. its the foundation of most syntax errors in the compiler. 
static void consume(TokenType type, const char* message){
    if(parser.current.type == type){
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(TokenType type){
    return parser.current.type == type;
}

static bool match(TokenType type){
    if(!check(type)) return false;
    advance();
    return true;
}


//code gen functions.emits appends a single byte to the chunk.

/*NOTE :  When emitByte is called, it is typically in response to a token that has already been parsed and processed (represented by parser.previous). 
The bytecode being emitted is a direct result of this previously processed token. Therefore, it makes sense to use the line number of this token (parser.previous.line) 
as it accurately reflects the source of the bytecode.*/


static void emitByte(uint8_t byte){
    writeChunk(currentChunk(),byte, parser.previous.line);
}

//for cases where we have to write an opcode followed by an operand.
static void emitBytes(uint8_t byte1, uint8_t byte2){
    emitByte(byte1);
    emitByte(byte2);
}


static int emitJump(uint8_t instruction){
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    //-2 to reach the jump instruction
    return currentChunk()->count - 2;
}


static void emitReturn(){
    emitByte(OP_RETURN);
}



//writing the constant to the chunk's constant pool and getting the index.
static uint8_t makeConstant(Value value){
    int constant = addConstant(currentChunk(),value);
    if(constant>UINT8_MAX){
        error("Too many constants in one chunk");
        return 0;
    }
    return (uint8_t)constant;

}


//writing the operand and opcode to the chunk.
static void emitConstant(Value value){
    emitBytes(OP_CONSTANT,makeConstant(value));
}


static void patchJump(int offset){
    int jump = currentChunk()->count - offset - 2;

    if(jump > UINT16_MAX){
        error("Too much code to jump over");
    }

    currentChunk()->code[offset] = jump>>8 && 0xff;
    currentChunk()-> code[offset + 1] = jump && 0xff;
}


static void initCompiler(Compiler* compiler){
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}


static void endCompiler(){
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if(!parser.hadError){
        disassembleChunk(currentChunk(),"code");
    }
#endif
}

static void beginScope(){
    current->scopeDepth++;
}

static void endScope(){
    current->scopeDepth--;

    while(current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth){
        emitByte(OP_POP);
        current->localCount--;
    }
}


static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token* name){
    return makeConstant(OBJ_VAL(copyString(name->start,name->length)));
}


static bool identifiersEqual(Token* a, Token* b){
    if(a->length != b->length) return false;
    return memcmp(a->start,b->start,a->length)==0;
}

static int resolveLocal(Compiler* compiler, Token* name){
    for(int i=compiler->localCount-1; i>=0; i--){
        Local* local= &compiler->locals[i];
        if(identifiersEqual(name,&local->name)){
            if(local->depth == -1){
                error("Can't read local variable in its own");
            }
            return i;
        }
    }
    return -1;
}

static void addLocal(Token name){
    if(current->localCount == UINT8_COUNT){
        error("Too many local variables in function");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->name=name;
    local->depth=-1;
}

static void declareVariable(){
    if(current->scopeDepth == 0)return;
    Token* name = &parser.previous;
    for(int i= current->localCount-1; i >= 0; i--){
        Local* local = &current->locals[i];
        if(local->depth != -1 && local->depth < current->scopeDepth){
            break;
        }
        if(identifiersEqual(name,&local->name)){
            error("already a variable with this name is in the scope");
        }
    }
    addLocal(*name);
}

static uint8_t parseVariable(const char* errorMessage){
    consume(TOKEN_IDENTIFIER,errorMessage);

    declareVariable();
    if(current->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized(){
    current->locals[current->localCount-1].depth = current->scopeDepth;
}


static void defineVariable(uint8_t global){
    if(current->scopeDepth > 0){
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL,global);
}

//dealing with infix operators // TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH
static void binary(bool canAssign){
    TokenType operatorType = parser.previous.type;
    ParseRule* rule= getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence+1));

    switch (operatorType){
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:    emitByte(OP_ADD); break;
        case TOKEN_MINUS:   emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR:    emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH:   emitByte(OP_DIVIDE); break;
        default: return;    //Unreachable
    }
}


/* this is to deal  with literals of the frontend such as NIL, TRUE , FALSE*/
static void literal(bool canAssign){
    switch(parser.previous.type){
        case TOKEN_FALSE: emitByte(OP_FALSE);break;
        case TOKEN_NIL: emitByte(OP_NIL);break;
        case TOKEN_TRUE: emitByte(OP_TRUE);break;
        default:return;
    }
}

//parantheses for grouping
static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}


//for token TOKEN_NUMBER
static void number(bool canAssign){
    double value = strtod(parser.previous.start,NULL);
    emitConstant(NUMBER_VAL(value));
}


// whenever the parser hits a string token, it calls this function. the +1, -2 is to remove the trailing quotation marks
static void string(bool canAssign){
    emitConstant(OBJ_VAL(copyString(parser.previous.start+1,parser.previous.length-2)));
}

static void namedVariable(Token name,bool canAssign){
    uint8_t getOp, setOp;
    int arg = resolveLocal(current,&name);
    if(arg!= -1){
        getOp=OP_GET_LOCAL;
        setOp=OP_SET_LOCAL;
    } else{
        arg= identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    if(canAssign && match(TOKEN_EQUAL)){
        expression();
        emitBytes(setOp,(uint8_t)arg);

    } else {
        emitBytes(getOp,(uint8_t)arg);
    }
}


static void variable(bool canAssign){
    namedVariable(parser.previous,canAssign);
}

//unary negation
static void unary(bool canAssign){
    TokenType operatorType = parser.previous.type;

    //compile the operand in terms of the precedence. PREC_UNARY has higher precedence than most others.
    parsePrecedence(PREC_UNARY);

    //emit the operator instruction
    switch(operatorType){
        case TOKEN_BANG: emitByte(OP_NOT);break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return; // unreaachable

    }
}

//array of parse rules
//the token enums are the indexes.
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_NONE},
    [TOKEN_GREATER]       = {NULL,     binary,   PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_NONE},
    [TOKEN_LESS]          = {NULL,     binary,   PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};


static void parsePrecedence(Precedence precedence){
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(prefixRule==NULL){
        error("expect expression");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while(precedence<=getRule(parser.current.type)->precedence){
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if(canAssign && match(TOKEN_EQUAL)){
        error("Invalid assignment target.");
    }
}

static ParseRule* getRule(TokenType type){
    return &rules[type];
}

//we parse the lowest precedence level which subsumes all of the higher precedence expressions too.
static void expression(){
    parsePrecedence(PREC_ASSIGNMENT);

}

static void block(){
    while(!check(TOKEN_RIGHT_BRACE)&& !check(TOKEN_EOF)){
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE,"Expect '}' after block.");
}


static void varDeclaration(){
    uint8_t global = parseVariable(" Expect variable name.");

    if(match(TOKEN_EQUAL)){
        expression();

    } else {
        //var a= nil;
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");

    defineVariable(global);
}

static void expressionStatement(){
    expression();
    consume(TOKEN_SEMICOLON,"Expect ';' after expression");
    emitByte(OP_POP);
}

static void ifStatement(){
    consume(TOKEN_LEFT_PAREN,"Expect '(' after the if .");
    expression();
    consume(TOKEN_RIGHT_PAREN,"expect ')' after condition");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if(match(TOKEN_ELSE)) statement();

    patchJump(elseJump);
}


static void printStatement(){
    expression();
    consume(TOKEN_SEMICOLON,"Expect ';' after value");
    emitByte(OP_PRINT);

}

/* We skip tokens indiscriminately until we reach something that looks like a statement boundary. 
We recognize the boundary by looking for a preceding token that can end a statement, 
like a semicolon. Or we’ll look for a subsequent token that begins a statement, 
usually one of the control flow or declaration keywords.*/

static void synchronize(){
    parser.panicMode = false;
    while(parser.current.type!=TOKEN_EOF){
        if(parser.previous.type == TOKEN_SEMICOLON) return;

        switch(parser.current.type){
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ;
        }

        advance();
    }
}


static void declaration(){
    if(match(TOKEN_VAR)){
        varDeclaration();
    } else {
        statement();
    }
    if(parser.panicMode) synchronize();
}

static void statement(){
    if(match(TOKEN_PRINT)){
        printStatement();
    }else if (match(TOKEN_IF)){
        ifStatement(); 
    }else if(match(TOKEN_LEFT_BRACE)){
        beginScope();
        block();
        endScope();
    }else{
        expressionStatement();
    }
}



bool compile(const char* source,Chunk* chunk){
    /* the first phase of compilation is scanning, so we are setting that up.*/
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk=chunk;
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    while(!match(TOKEN_EOF)){
        declaration();
    }
    endCompiler();
    return !parser.hadError;



    
}
