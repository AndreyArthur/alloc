#include "memory.h"
#include "heap.h"

static void *memory_to_chunk(void *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    return (void *)((size_t)ptr - 8);
}

static void *chunk_to_memory(void *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    return &((size_t *)ptr)[1];
}

void *memory_alloc(size_t size) {
    void *chunk = chunk_alloc(size);
    return chunk_to_memory(chunk);
}

void *memory_realloc(void *ptr, size_t size) {
    void *chunk = memory_to_chunk(ptr);
    void *realloced = chunk_realloc(chunk, size);
    return chunk_to_memory(realloced);
}

void memory_free(void *ptr) {
    void *chunk = memory_to_chunk(ptr);
    chunk_free(chunk);
}
