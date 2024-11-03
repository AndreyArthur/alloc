#ifndef HEAP_H
#define HEAP_H

#include <stdbool.h>
#include <stddef.h>

void *block_alloc(size_t size);

size_t block_get_capacity(void *start);

void *block_get_next(void *start);

void *block_get_first_chunk(void *start);

void block_free(void *start);

void *chunk_alloc(size_t size);

bool chunk_is_free(void *chunk);

bool chunk_is_last(void *chunk);

size_t chunk_get_size(void *chunk);

void *chunk_get_next(void *chunk);

void *chunk_alloc(size_t size);

void *chunk_realloc(void *chunk, size_t size);

void chunk_free(void *chunk);

#endif
