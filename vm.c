//
// Created by dylan on 11/7/23.
//

#include <stdio.h>
#include "common.h"
#include "vm.h"
#include "debug.h"

VM vm;

void initVM() {

}

void freeVM() {

}

InterpretResult interpret(Chunk *chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->bcode;
    return run();
}

static InterpretResult run() {
#define READ_BYTE() ((int)*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[READ_BYTE() + READ_BYTE() + READ_BYTE()])

    for(;;) {
#ifdef DEBUG_TRACE_EXECUTION
        disassembleInstructions(vm.chunk, (int) (vm.ip - vm.chunk->bcode));
#endif
        
        int instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_LONG_CONSTANT();
                printValue(constant);
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_LONG_CONSTANT
}
