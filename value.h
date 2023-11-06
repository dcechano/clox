//
// Created by dylan on 11/5/23.
//

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

typedef double Value;

// ValueArray represents a 'constants table'.
typedef struct {
    int capacity;
    int count;
    Value *values;
} ValueArray;

void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);



#endif //CLOX_VALUE_H
