#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "memory.h"
#include "vm.h"

int main() {
    initVM();
    Chunk chunk;
    initChunk(&chunk);

    writeConstant(&chunk, 10.0, 123);
    writeConstant(&chunk, 2.0, 123);
    writeConstant(&chunk, 3.0, 123);
    writeChunk(&chunk, OP_ADD, 123);

//
    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 124);
    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);
    return 0;
}
