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

    for (int i = 0; i < 256; i++) {
        writeConstant(&chunk, 420.0 + i, 69);
    }

    writeConstant(&chunk, 2.0, 124);
    writeChunk(&chunk, OP_RETURN, 123);
    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);
    return 0;
}
