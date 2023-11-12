#include "memory.h"
#include <stdlib.h>

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    //    realloc copies the old data over to the newly sized array and frees the
    //    old array.
    void *result = realloc(pointer, newSize);
    if (result == NULL)
        exit(1);// if result == NULL that means that we ran out of memory!
    return result;
}
