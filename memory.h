#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// Change capacity of Chunk struct
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

/* Grow the array by allocating more memory */
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (oldCount), \
        sizeof(type) * (newCount))

#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, sizeof(type) * oldCount, 0)

/*
 * oldSize 	    newSize 	            Operation
 *  0 	        Non‑zero 	            Allocate new block.
 *  Non‑zero 	0 	                    Free allocation.
 *  Non‑zero 	Smaller than oldSize 	Shrink existing allocation.
 *  Non‑zero 	Larger than oldSize 	Grow existing allocation.
 */
void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif //CLOX_MEMORY_H
