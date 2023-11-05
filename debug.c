//
// Created by dylan on 11/4/23.
//

#include <stdio.h>
#include "debug.h"

int simpleInstruction(const char *name, int offset);

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstructions(chunk, offset);
    }
}

int disassembleInstructions(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->bcode[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unkown opcode %d\n", instruction);
            return offset + 1;
    }
}

int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}
