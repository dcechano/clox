#include <stdlib.h>
#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->bcode = NULL;
}

void freeChunk(Chunk* chunk) {
//    deallocate the memory then call initChunk() to zere out the fields.
    FREE_ARRAY(uint8_t, chunk->bcode, chunk->capacity);
    initChunk(chunk);
}

/*
 * write an uint8_t to the chunk->bcode field.
 * The uint8_t is passed in as a `OpCode` enum.
 *
 * # Example:
 *
 * Chunk chunk;
 * initChunk(&chunk);
 * writeChunk(&chunk, OP_RETURN);
 * */
void writeChunk(Chunk* chunk, uint8_t byte) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->bcode = GROW_ARRAY(uint8_t, chunk->bcode, oldCapacity, chunk->capacity);
    }

    chunk->bcode[chunk->count] = byte;
    chunk->count++;
}

