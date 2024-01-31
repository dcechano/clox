//
// Created by dylan on 11/10/23.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"
#include "object.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif// CLOX_COMPILER_H
