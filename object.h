//
// Created by dylan on 11/23/23.
//

#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include "value.h"
#include "vm.h"

#define ALLOCATE(type, count) \
    (type*) reallocate(NULL, 0, sizeof(type) * (count))


#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) ((OBJ_STRING *) AS_OBJ(value))
#define AS_CSTRING(value) (((OBJ_STRING *) AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING
} ObjType;

struct Obj {
    ObjType type;
};

struct ObjString {
    Obj obj;
    int length;
    char *chars;
};

ObjString *copyString(const char *chars, int length);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif//CLOX_OBJECT_H
