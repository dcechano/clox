//
// Created by dylan on 11/8/23.
//

#ifndef CLOX_VM_MACRO_H
#define CLOX_VM_MACRO_H

#define READ_BYTE() ((int)*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() (vm.chunk->constants.values[READ_BYTE() + READ_BYTE() + READ_BYTE()])
#define BINARY_OP(op) \
    do {              \
        double b = pop();\
        double a = pop();\
        push(a op b); \
    } while(false)

#endif //CLOX_VM_MACRO_H
