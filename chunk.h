#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

/*
 * Opcode used by the compiler.
 * Some opcodes imply that there is an operand stored as binary
 * date immediately after the opcode.*/
typedef enum {
  /* OP_CONSTANT implies that there is an index to a constant immediately
   * following the OpCode in memory.
   * Example: 0023 where 00 is the opcode and
   * 23 is the index to find the constant in the constant table.*/
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  // Return from the current function
  OP_RETURN,
} OpCode;

// The Chunk struct represents a dynamic array in memory.
typedef struct {
  int count;
  int capacity;
  uint8_t *bcode;
  int *lines;
  ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void freeChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, int line);
int addConstant(Chunk *chunk, Value value);
void writeConstant(Chunk *chunk, Value value, int line);

#endif