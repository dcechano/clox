//
// Created by dylan on 11/10/23.
//

#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "chunk.h"

bool compile(const char *source, Chunk *chunk);

#endif// CLOX_COMPILER_H
