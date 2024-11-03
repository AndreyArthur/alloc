#include "heap.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#define PAGESIZE 4096
#define CHUNK_FREE 0x0000000000000001
#define CHUNK_LAST (CHUNK_FREE << 1)

void *heap = NULL;

static size_t page_align(size_t size) {
    return size % PAGESIZE == 0 ? size : (size / PAGESIZE + 1) * PAGESIZE;
}

static size_t align(size_t size) {
    return size % sizeof(size_t) == 0
               ? size
               : (size / sizeof(size_t) + 1) * sizeof(size_t);
}

static void block_delete(void *previous, void *to_be_deleted) {
    if (previous == NULL) {
        heap = block_get_next(to_be_deleted);
    } else {
        size_t *size_t_previous = previous;
        size_t_previous[0] = (size_t)block_get_next(to_be_deleted);
    }

    munmap(to_be_deleted, block_get_capacity(to_be_deleted));
}

static void chunk_initialize(void *ptr, size_t size, size_t flags) {
    size_t aligned = align(size);
    size_t *first = &((size_t *)ptr)[0];
    *first = aligned | flags;
}

void *block_alloc(size_t size) {
    size_t metadata_size = sizeof(size_t) * 2;
    size_t pages_size = page_align(size + metadata_size);
    size_t *new_block_ptr = mmap(NULL, pages_size, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (heap == NULL) {
        heap = new_block_ptr;
        new_block_ptr[0] = (size_t)NULL;
        new_block_ptr[1] = pages_size;
        chunk_initialize(&new_block_ptr[2], pages_size - 16 - 8,
                         CHUNK_LAST | CHUNK_FREE);
        return new_block_ptr;
    }

    void *current = heap;
    while (block_get_next(current) != NULL) {
        current = block_get_next(current);
    }

    new_block_ptr[0] = (size_t)NULL;
    new_block_ptr[1] = pages_size;
    chunk_initialize(&new_block_ptr[2], pages_size - 16 - 8,
                     CHUNK_FREE | CHUNK_LAST);
    ((size_t *)current)[0] = (size_t)new_block_ptr;

    return new_block_ptr;
}

size_t block_get_capacity(void *start) { return ((size_t *)start)[1]; }

void *block_get_next(void *start) { return (void *)((size_t *)start)[0]; }

void *block_get_first_chunk(void *start) { return &((size_t *)start)[2]; }

void block_free(void *start) {
    if (heap == NULL) {
        return;
    }

    void *previous = NULL;
    void *current = heap;
    while (current != NULL) {
        if (current == start) {
            block_delete(previous, current);
            return;
        }

        previous = current;
        current = block_get_next(current);
    }
}

bool chunk_is_free(void *chunk) {
    size_t first = ((size_t *)chunk)[0];
    return first & CHUNK_FREE;
}

static void chunk_set_size(void *chunk, size_t size) {
    size_t *first = &((size_t *)chunk)[0];

    bool is_free = chunk_is_free(chunk);
    bool is_last = chunk_is_last(chunk);

    *first = align(size);

    if (is_free) {
        *first |= CHUNK_FREE;
    }

    if (is_last) {
        *first |= CHUNK_LAST;
    }
}

static void chunk_set_freeness(void *chunk, bool set_to_free) {
    size_t *first = &((size_t *)chunk)[0];
    if (set_to_free) {
        *first |= CHUNK_FREE;
    } else {
        *first &= ~CHUNK_FREE;
    }
}

static void chunk_set_lastness(void *chunk, bool set_to_last) {
    size_t *first = &((size_t *)chunk)[0];
    if (set_to_last) {
        *first |= CHUNK_LAST;
    } else {
        *first &= ~CHUNK_LAST;
    }
}

bool chunk_is_last(void *chunk) {
    size_t first = ((size_t *)chunk)[0];
    return (first >> 1) & (CHUNK_LAST >> 1);
}

size_t chunk_get_size(void *chunk) {
    size_t first = ((size_t *)chunk)[0];
    return first & ~(CHUNK_FREE | CHUNK_LAST);
}

void *chunk_get_next(void *chunk) {
    if (chunk_is_last(chunk)) {
        return NULL;
    }

    size_t size = chunk_get_size(chunk);
    uint8_t *bytes_chunk = chunk;
    void *next = &bytes_chunk[sizeof(size_t) + size];

    return next;
}

static void chunk_appropriate(void *chunk, size_t size) {
    if (!chunk_is_free(chunk)) {
        return;
    }
    bool original_is_last = chunk_is_last(chunk);
    size_t original_size = chunk_get_size(chunk);
    size_t new_size = align(size);
    if (new_size > original_size) {
        return;
    }

    size_t remaining_size = original_size - new_size;
    if (remaining_size >= 16) {
        chunk_set_size(chunk, new_size);
        chunk_set_freeness(chunk, false);
        chunk_set_lastness(chunk, false);

        void *new_chunk = chunk_get_next(chunk);
        size_t *first = &((size_t *)new_chunk)[0];
        *first = 0;
        chunk_set_size(new_chunk, remaining_size - sizeof(size_t));
        chunk_set_freeness(new_chunk, true);
        chunk_set_lastness(new_chunk, original_is_last);

        return;
    }

    chunk_set_freeness(chunk, false);
}

static void *chunk_find_fit(size_t size) {
    size_t aligned = align(size);
    void *current_block = heap;
    if (current_block == NULL) {
        return NULL;
    }

    while (current_block != NULL) {
        void *current_chunk = block_get_first_chunk(current_block);
        while (current_chunk != NULL) {
            bool is_free = chunk_is_free(current_chunk);
            bool has_enough_size = aligned <= chunk_get_size(current_chunk);
            if (is_free && has_enough_size) {
                return current_chunk;
            }

            current_chunk = chunk_get_next(current_chunk);
        }

        current_block = block_get_next(current_block);
    }

    return NULL;
}

void *chunk_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t aligned = align(size);
    void *fit = chunk_find_fit(aligned);

    if (fit == NULL) {
        block_alloc(sizeof(size_t) + aligned);
    }

    fit = chunk_find_fit(aligned);

    chunk_appropriate(fit, aligned);

    return fit;
}

static bool block_is_empty(void *block) {
    size_t capacity = block_get_capacity(block);
    void *first_chunk = block_get_first_chunk(block);

    if (!chunk_is_free(first_chunk)) {
        return false;
    }

    size_t block_metadata_size = sizeof(size_t) * 2;
    size_t chunk_metadata_size = sizeof(size_t);
    size_t first_chunk_size = chunk_get_size(first_chunk);
    size_t full_size =
        block_metadata_size + chunk_metadata_size + first_chunk_size;

    return full_size == capacity;
}

void *chunk_realloc(void *chunk, size_t size) {
    if (chunk == NULL) {
        return chunk_alloc(size);
    }

    size_t current_size = chunk_get_size(chunk);
    size_t new_size = align(size);
    if (new_size == 0) {
        chunk_free(chunk);
        return NULL;
    }

    if (new_size > current_size) {
        size_t diff = new_size - current_size;

        void *next_chunk = chunk_get_next(chunk);
        if (next_chunk && chunk_is_free(next_chunk)) {
            size_t next_chunk_size = chunk_get_size(next_chunk);
            size_t next_chunk_is_last = chunk_is_last(next_chunk);
            size_t next_chunk_freed_size = next_chunk_size + sizeof(size_t);

            if (next_chunk_freed_size >= diff) {
                if (next_chunk_freed_size - diff < 16) {
                    chunk_set_size(chunk, current_size + next_chunk_freed_size);
                    chunk_set_lastness(chunk, next_chunk_is_last);
                } else {
                    chunk_set_size(chunk, current_size + diff);
                    void *new_next_chunk = chunk_get_next(chunk);
                    chunk_set_size(new_next_chunk, next_chunk_size - diff);
                    chunk_set_lastness(new_next_chunk, next_chunk_is_last);
                    chunk_set_freeness(new_next_chunk, true);
                }
                return chunk;
            }
        }

        void *new_chunk = chunk_alloc(new_size);
        void *current_memory = &((size_t *)chunk)[1];
        void *new_memory = &((size_t *)new_chunk)[1];
        memcpy(new_memory, current_memory, current_size);

        chunk_free(chunk);
        return new_chunk;
    }

    if (new_size < current_size) {
        size_t diff = current_size - new_size;

        void *next_chunk = chunk_get_next(chunk);

        if (next_chunk && chunk_is_free(next_chunk)) {
            size_t next_chunk_old_size = chunk_get_size(next_chunk);
            bool next_chunk_is_last = chunk_is_last(next_chunk);

            chunk_set_size(chunk, new_size);
            void *new_next = chunk_get_next(chunk);
            chunk_set_size(new_next, next_chunk_old_size + diff);
            chunk_set_lastness(new_next, next_chunk_is_last);
            chunk_set_freeness(new_next, true);

            return chunk;
        }

        if (diff >= 16) {
            bool current_chunk_is_last = chunk_is_last(chunk);

            chunk_set_size(chunk, new_size);
            chunk_set_lastness(chunk, false);

            void *new_chunk = chunk_get_next(chunk);
            chunk_set_size(new_chunk, diff - sizeof(size_t));
            chunk_set_lastness(new_chunk, current_chunk_is_last);
            chunk_set_freeness(new_chunk, true);

            return chunk;
        }
    }

    return chunk;
}

void chunk_free(void *chunk) {
    if (chunk == NULL) {
        return;
    }

    void *current_block = heap;
    void *previous_chunk, *current_chunk;
    while (current_block != NULL) {
        previous_chunk = NULL;
        current_chunk = block_get_first_chunk(current_block);

        bool found = false;
        while (current_chunk != NULL) {
            if (current_chunk == chunk) {
                found = true;
                break;
            }

            previous_chunk = current_chunk;
            current_chunk = chunk_get_next(current_chunk);
        }
        if (found == true) {
            break;
        }

        current_block = block_get_next(current_block);
    }

    if (current_chunk != chunk) {
        return;
    }

    chunk_set_freeness(current_chunk, true);

    void *first_free_chunk;
    if (previous_chunk && chunk_is_free(previous_chunk)) {
        first_free_chunk = previous_chunk;
    } else {
        first_free_chunk = current_chunk;
    }

    bool last = false;
    void *free_chunk = first_free_chunk;
    size_t free_size = chunk_get_size(free_chunk);
    void *next = chunk_get_next(free_chunk);
    while (next && chunk_is_free(next)) {
        free_chunk = next;
        next = chunk_get_next(next);
        if (chunk_is_last(free_chunk)) {
            last = true;
        }
        free_size += sizeof(size_t) + chunk_get_size(free_chunk);
    }

    chunk_set_size(first_free_chunk, free_size);
    chunk_set_lastness(first_free_chunk, last);

    if (block_is_empty(current_block)) {
        block_free(current_block);
    }
}
