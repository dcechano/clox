#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"

typedef enum {
    // Return from the current function
    OP_RETURN,
} OpCode;

// The Chunk struct represents a dynamic array in memory.
typedef struct {
    int count;
    int capacity;
    uint8_t* bcode;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif

