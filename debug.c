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
//    OpCode where `offset + 1` contains the constantIndx
    uint8_t constantIndx = chunk->bcode[offset + 1];
    printf("%-16s %4d '", name, constantIndx);
    printValue(chunk->constants.values[constantIndx]);
    printf("'\n");
//    return the index to the next instruction
    return offset + 2;
}

static int constantLongInstruction(const char* name, Chunk* chunk, int offset){
    int constantIndx0 = chunk->bcode[offset + 1];
    int constantIndx1 = chunk->bcode[offset + 2];
    int constantIndx2 = chunk->bcode[offset + 3];

    int constantIndx = constantIndx0 + constantIndx1 + constantIndx2;
    printf("%-16s %4d '", name, constantIndx);
    printValue(chunk->constants.values[constantIndx]);
    printf("'\n");
//    return the index to the next instruction
    return offset + 4;
}

int simpleInstruction(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstructions(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 &&
        chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->bcode[offset];
    switch (instruction) {
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);

        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
