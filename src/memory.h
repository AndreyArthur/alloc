#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *memory_alloc(size_t size);

void *memory_realloc(void *ptr, size_t size);

void memory_free(void *ptr);

#endif
