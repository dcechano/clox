//
// Created by dylan on 11/10/23.
//
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "common.h"
#include "compiler.h"

#include "memory.h"
#include "scanner.h"


#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif


typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,// =
    PREC_OR,        // or
    PREC_AND,       // and
    PREC_EQUALITY,  // == !=
    PREC_COMPARISON,// < > <= >=
    PREC_TERM,      // + -
    PREC_FACTOR,    // * /
    PREC_UNARY,     // ! -
    PREC_CALL,      // . ()
    PREC_PRIMARY
} Precedence;

// Function pointer behind a typedef
typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

/*
 * struct representing a local variable.
 * name refers to the identifier of the local
 * depth refers to the offset from the global scope.
 */
typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

/*
 * locals: list of local variables currently in scope.
 * localCount: current size of locals.
 * scopeDepth: The current scope's offset from the global scope.
 * enclosing: A stack of compilers for tracking compilers of nested functions
 */
typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

Parser parser;
Compiler* currentCompiler = NULL;
// Chunk* compilingChunk;

static Chunk* currentChunk() {
    return &currentCompiler->function->chunk;
}

static void errorAt(const Token* token, const char* message) {
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        //        Ignore
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;
        errorAtCurrent(parser.current.start);
    }
}

static void consume(const TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(const TokenType type) {
    return parser.current.type == type;
}

static bool match(const TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

static void emitByte(const uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(const uint8_t byte0, const uint8_t byte1) {
    emitByte(byte0);
    emitByte(byte1);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) error("Loop body too large");

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(const uint8_t instruction) {
    emitByte(instruction);
    // 0xff are placeholder bytes until we know how many bytes to truly jump.
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static int makeConstant(const Value value) {
    return (uint8_t) addConstant(currentChunk(), value);
}

static void emitConstant(const Value value) {
    writeConstant(currentChunk(), value, 1);
}

static void patchJump(const int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over");
    }

    currentChunk()->bcode[offset]     = (jump >> 8) & 0xff;
    currentChunk()->bcode[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing  = currentCompiler;
    compiler->function   = NULL;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function   = newFunction();
    currentCompiler      = compiler;
    if (type != TYPE_SCRIPT) {
        currentCompiler->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local       = &currentCompiler->locals[currentCompiler->localCount++];
    local->depth       = 0;
    local->isCaptured  = false;
    local->name.start  = "";
    local->name.length = 0;
}

static ObjFunction* endCompiler() {
    emitReturn();
    ObjFunction* function = currentCompiler->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != NULL
                                                 ? function->name->chars
                                                 : "<script>");
    }
#endif

    currentCompiler = currentCompiler->enclosing;
    return function;
}

static void endScope() {
    currentCompiler->scopeDepth--;

    while (currentCompiler->localCount > 0 &&
           currentCompiler->locals[currentCompiler->localCount - 1]
                           .depth > currentCompiler->scopeDepth) {
        if (currentCompiler->locals[currentCompiler->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        currentCompiler->localCount--;
    }
}

static void beginScope() {
    currentCompiler->scopeDepth++;
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);


static void binary(bool _) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule        = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return;
    }
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Functions cannot have more than 255 arguments.");
            }

            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect '(' after arguments");
    return argCount;
}

static void call(bool _) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void literal(bool _) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return;
    }
}

static void number(bool _) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump  = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool _) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static uint8_t identifierConstant(const Token* name);
static int resolveLocal(Compiler* compiler, const Token* name);
static int resolveUpvalue(Compiler* compiler, Token* name);

static void namedVariable(Token name, const bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(currentCompiler, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(currentCompiler, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg   = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

static void variable(const bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void statement();
static void declaration();

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) & !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}

static uint8_t parseVariable(const char* errorMessage);
static void defineVariable(uint8_t global);
static void markInitialized();

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            currentCompiler->function->arity++;
            if (currentCompiler->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before fucntion body");
    block();

    ObjFunction* function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    /*
      The OP_CLOSURE instruction is unique in that it has a variably sized encoding.
      For each upvalue the closure captures, there are two single-byte operands.
      Each pair of operands specifies what that upvalue captures.
      If the first byte is one, it captures a local variable in the enclosing function.
      If zero, it captures one of the function’s upvalues.
      The next byte is the local slot or upvalue index to capture.
     */
    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expected function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}


static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'");
    // Check initializer statement
    if (match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump  = -1;
    // Check conditional statement
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after loop condition");

        // Jump out of the loop if the condition was false
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    // check increment clause
    // This block is tricky.
    // Refer to https://craftinginterpreters.com/jumping-back-and-forth.html#increment-clause
    if (!match(TOKEN_RIGHT_PAREN)) {
        // first emit unconditional jump over increment clause because
        // increment should only be executed after the body of the loop
        int bodyJump       = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        // parse increment clause
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);       // loopStart == condition expression's index in bytecode
        loopStart = incrementStart;// change loopStart to increment's index
        // patch jump operand made at the start of block
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);
    // patch jump operand
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);// Condition
    }
    endScope();
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'");
    expression();// the 'if' condition
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE)) {
        statement();
    }
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (currentCompiler->type == TYPE_SCRIPT) {
        error("Can't return from global scope");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn(TOKEN_SEMICOLON);
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return statement");
        emitByte(OP_RETURN);
    }
}

static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void caseDeclaration(int switchStart) {
    if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
        switch (parser.previous.type) {
            case TOKEN_CASE: {
                expression();
                consume(TOKEN_COLON, "Expect ':' after 'case' expression.");


                emitByte(OP_CASE_COMP);
                int thenJump = emitJump(OP_JUMP_IF_FALSE);
                emitByte(OP_POP);
                statement();
                emitLoop(switchStart);
                patchJump(thenJump);
                emitByte(OP_POP);
                break;
            }
            case TOKEN_DEFAULT: {
                consume(TOKEN_COLON, "Expect ':' after 'default' keyword.");
                emitByte(OP_POP);
                statement();
                emitLoop(switchStart);
                break;
            }
        }
    } else {
        advance();
    }
}


static void switchStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");

    const int jumpToCase = emitJump(OP_JUMP);
    const int loopStart  = currentChunk()->count;
    const int endSwitch  = emitJump(OP_JUMP);
    patchJump(jumpToCase);

    consume(TOKEN_LEFT_BRACE, "Expect '{' after switch expression");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        caseDeclaration(loopStart);
    }

    patchJump(endSwitch);
    consume(TOKEN_RIGHT_BRACE, "Expect '}' to conclude case statement");
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default: {
            }
        }
        advance();
    }
}

static void declaration() {
    if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();

    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else if (match(TOKEN_SWITCH)) {
        switchStatement();
    } else {
        expressionStatement();
    }
}

static void grouping(bool _) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void unary(bool _) {
    TokenType operatorType = parser.previous.type;

    //    compile the operand
    parsePrecedence(PREC_UNARY);
    //    Emit the operator instruction
    switch (operatorType) {
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return;
    }
}

static void and_(bool canAssign);


ParseRule rules[] = {
        [TOKEN_COLON]         = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_PAREN]    = {grouping, call, PREC_CALL},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        [TOKEN_NUMBER]        = {number, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, and_, PREC_AND},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, or_, PREC_OR},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_SWITCH]        = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static void parsePrecedence(const Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t identifierConstant(const Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(const Token* a, const Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler* compiler, const Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index   = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t) local, true);
    }

    // upvalues can be several 'levels' up so we look for the variable
    // in the enclosing scopes above.
    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t) upvalue, false);
    }


    return -1;
}

static void addLocal(const Token name) {
    if (currentCompiler->localCount == UINT8_COUNT) {
        error("Too many local variables in function. Cannot have > 256 local variables");
    }

    Local* local      = &currentCompiler->locals[currentCompiler->localCount++];
    local->name       = name;
    local->depth      = -1;
    local->isCaptured = false;
}

/*
 *  Declare a local variable. If the compiler is in the global scope the 
 *  function returns. 
 */
static void declareVariable() {
    if (currentCompiler->scopeDepth == 0) {
        return;
    }

    Token* name = &parser.previous;
    // Iterate through the list of local variables. If a local with the same
    // name is found, emit and error.
    for (int i = currentCompiler->localCount - 1; i >= 0; i--) {
        Local* local = &currentCompiler->locals[i];
        if (local->depth != -1 && local->depth < currentCompiler->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Cannot shadow variables in the same scope.");
        }
    }
    addLocal(*name);
}

/*
* Parse a variable. uint8_t returned is the index of the constant
* on the current chunk's consant table. Local variables get 
* dummy index of 0.
*/
static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (currentCompiler->scopeDepth > 0) return 0;

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (currentCompiler->scopeDepth == 0) {
        return;
    }
    currentCompiler->locals[currentCompiler->localCount - 1].depth =
            currentCompiler->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (currentCompiler->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void and_(const bool _) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

ObjFunction* compile(const char* source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError  = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
    Compiler* compiler = currentCompiler;
    while (compiler != NULL) {
        markObject((Obj*) compiler->function);
        compiler = compiler->enclosing;
    }
}
