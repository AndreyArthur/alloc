#include "../src/memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void test_memory_alloc() {
    char *message = memory_alloc(sizeof(char) * 37);
    strcpy(message, "I've implemented a memory allocator!");

    assert(strcmp(message, "I've implemented a memory allocator!") == 0);

    memory_free(message);
}

void test_memory_realloc() {
    size_t len = 4096;
    uint64_t *arr = memory_realloc(NULL, 0);

    for (size_t i = 0; i < len; i++) {
        arr = memory_realloc(arr, sizeof(uint64_t) * (i + 1));
        arr[i] = i + 1;
    }

    for (size_t i = 0; i < len; i++) {
        assert(arr[i] == i + 1);
    }

    memory_realloc(arr, 0);
}

int main() {
    test_memory_alloc();
    test_memory_realloc();

    return 0;
}
