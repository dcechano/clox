//
// Created by dylan on 11/8/23.
//

#ifndef CLOX_VM_MACRO_H
#define CLOX_VM_MACRO_H

#define READ_BYTE() ((int) *vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_LONG_CONSTANT() \
    (vm.chunk->constants.values[READ_BYTE() + READ_BYTE() + READ_BYTE()])

#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while (false)

#endif// CLOX_VM_MACRO_H
