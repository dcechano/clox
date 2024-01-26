//
// Created by dylan on 11/8/23.
//

#ifndef CLOX_VM_MACRO_H
#define CLOX_VM_MACRO_H

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() \
    (frame->ip += 2, (uint16_t) ((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_INT() \
    (frame->ip += 3, (int32_t) ((frame->ip[-3] << 8) | frame->ip[-2] | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_LONG_CONSTANT() \
    (frame->closure->function->chunk.constants.values[READ_INT()])

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
