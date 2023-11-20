#include "chunk.h"
#include "memory.h"
#include <stdlib.h>

void initChunk(Chunk *chunk) {
    chunk->count    = 0;
    chunk->capacity = 0;
    chunk->bcode    = NULL;
    chunk->lines    = NULL;
    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    //    deallocate the memory then call initChunk() to zero out the fields.
    FREE_ARRAY(uint8_t, chunk->bcode, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    freeValueArray(&chunk->constants);
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
void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->bcode =
                GROW_ARRAY(uint8_t, chunk->bcode, oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldCapacity, chunk->capacity);
    }

    chunk->bcode[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

/*
 * Add a constant in the form of a `Value` the chunk's constant table
 * (ValueArray)*/
int addConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    /*
   * After we add the constant, we return the index where the constant was
   * appended so that we can locate that same constant later.
   * */
    return chunk->constants.count - 1;
}

void writeConstant(Chunk *chunk, Value value, int line) {
    int constIndx = addConstant(chunk, value);

    if (constIndx <= 255) {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, constIndx, line);
        return;
    }
    if (constIndx > 255 * 3) {
        exit(1);
    }

    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, 255, line);
    for (int i = 0; i < 2; i++) {
        constIndx = constIndx - 255;
        if (constIndx < 0) {
            writeChunk(chunk, 0, line);
        } else if (constIndx > 255) {
            writeChunk(chunk, 255, line);
        } else {
            writeChunk(chunk, constIndx, line);
        }
    }
}
