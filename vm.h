//
// Created by dylan on 11/7/23.
//

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"

#define STACK_MAX 256

typedef struct {
    Chunk *chunk;
    // Instruction pointer. Points to the current chunk's next bytecode instruction.
    uint8_t *ip;
    Value stack[STACK_MAX];
    Value *stackTop;
    Table globals;
    Table strings;
    Obj *objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char *source);
static InterpretResult run();
void push(Value value);
Value pop();

#endif// CLOX_VM_H
