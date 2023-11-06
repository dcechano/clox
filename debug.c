//
// Created by dylan on 11/4/23.
//

#include <stdio.h>
#include "debug.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstructions(chunk, offset);
    }
}


static int constantInstruction(const char* name, Chunk* chunk, int offset){
//    `offset + 1` because `offset` points to the current
//    OpCode where `offset + 1` contains the constant
    uint8_t constant = chunk->bcode[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
//    return the index to the next instruction
    return offset + 2;
}

int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstructions(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->bcode[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
