#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/*
 * Opcode used by the compiler.
 * Some opcodes imply that there is an operand stored as binary
 * data immediately after the opcode.*/
typedef enum {
    /* OP_CONSTANT implies that there is an index to a constant immediately
   * following the OpCode in memory.
   * Example: 0023 where 00 is the opcode and
   * 23 is the index to find the constant in the constant table.*/
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_CASE_COMP,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_LOCAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    // Return from the current function
    OP_RETURN,
} OpCode;

// The Chunk struct represents a dynamic array in memory.
typedef struct {
    int count;
    int capacity;
    uint8_t* bcode;
    int* lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
int writeConstant(Chunk* chunk, Value value, int line);

#endif
