#include "../src/heap.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void test_block_alloc() {
    void *first_block = block_alloc(4080);
    void *first_block_next = (void *)((size_t *)first_block)[0];
    size_t first_block_capacity = ((size_t *)first_block)[1];

    assert(first_block_capacity == 4096);
    assert(first_block_next == NULL);

    void *first_chunk = &((size_t *)first_block)[2];

    assert(chunk_get_size(first_chunk) == 4072);
    assert(chunk_is_free(first_chunk) == true);
    assert(chunk_is_last(first_chunk) == true);

    void *second_block = block_alloc(4096);
    void *second_block_next = (void *)((size_t *)second_block)[0];
    size_t second_block_capacity = ((size_t *)second_block)[1];

    assert(second_block_capacity == 8192);
    assert(second_block_next == NULL);

    first_block_next = (void *)((size_t *)first_block)[0];

    assert(second_block == first_block_next);

    block_free(first_block);
    block_free(second_block);
}

void test_block_get_capacity() {
    {
        void *block = block_alloc(1024);
        size_t capacity = block_get_capacity(block);

        assert(capacity == 4096);

        block_free(block);
    }

    {
        void *block = block_alloc(4096);
        size_t capacity = block_get_capacity(block);

        assert(capacity == 8192);

        block_free(block);
    }
}

void test_block_get_next() {
    void *first = block_alloc(1024);
    void *second = block_alloc(4096);

    assert(block_get_next(first) == second);
    assert(block_get_next(second) == NULL);

    block_free(second);

    assert(block_get_next(first) == NULL);

    block_free(first);
}

void test_block_free() {
    void *first_block = block_alloc(1024);
    void *second_block = block_alloc(1024);
    void *third_block = block_alloc(1024);

    void *first_block_next = (void *)((size_t *)first_block)[0];
    void *second_block_next = (void *)((size_t *)second_block)[0];
    void *third_block_next = (void *)((size_t *)third_block)[0];

    assert(first_block_next == second_block);
    assert(second_block_next == third_block);
    assert(third_block_next == NULL);

    block_free(second_block);

    first_block_next = (void *)((size_t *)first_block)[0];
    third_block_next = (void *)((size_t *)third_block)[0];

    assert(first_block_next == third_block);
    assert(third_block_next == NULL);

    block_free(first_block);
    block_free(third_block);
}

void test_chunk_alloc() {
    void *first = chunk_alloc(12);

    assert(first != NULL);
    assert(chunk_is_free(first) == false);
    assert(chunk_is_last(first) == false);
    assert(chunk_get_size(first) == 16);

    void *next = chunk_get_next(first);

    assert(next != NULL);
    assert(chunk_is_free(next) == true);
    assert(chunk_is_last(next) == true);
    assert(chunk_get_size(next) == 4048);

    void *second = chunk_alloc(4032);

    assert(second != NULL);
    assert(chunk_is_free(second) == false);
    assert(chunk_is_last(second) == false);
    assert(chunk_get_size(second) == 4032);
    assert(second == next);

    next = chunk_get_next(second);

    assert(next != NULL);
    assert(chunk_is_free(next) == true);
    assert(chunk_is_last(next) == true);
    assert(chunk_get_size(next) == 8);

    void *third = chunk_alloc(4);

    assert(third != NULL);
    assert(chunk_is_free(third) == false);
    assert(chunk_is_last(third) == true);
    assert(chunk_get_size(third) == 8);
    assert(third == next);

    void *fourth = chunk_alloc(124);

    assert(fourth != NULL);
    assert(chunk_is_free(fourth) == false);
    assert(chunk_is_last(fourth) == false);
    assert(chunk_get_size(fourth) == 128);

    void *null = chunk_alloc(0);
    assert(null == NULL);

    chunk_free(first);
    chunk_free(second);
    chunk_free(third);
    chunk_free(fourth);
}

void test_chunk_free() {
    void *first = chunk_alloc(10);
    void *second = chunk_alloc(10);
    void *third = chunk_alloc(10);

    chunk_free(second);

    assert(chunk_is_free(second));

    void *new_second = chunk_alloc(10);

    assert(second == new_second);

    chunk_free(new_second);
    chunk_free(first);

    assert(chunk_is_free(first));
    assert(chunk_get_size(first) == 40);

    chunk_free(third);
}

void test_chunk_realloc() {
    {
        void *first = chunk_alloc(16);
        void *second = chunk_alloc(8);
        void *temp = chunk_alloc(8);
        void *third = chunk_alloc(8);

        chunk_free(temp);

        assert(chunk_get_size(temp) == 8);
        assert(chunk_is_free(temp));
        assert(chunk_get_next(second) == temp);

        void *new_second = chunk_realloc(second, 16);
        assert(second == new_second);
        assert(chunk_get_size(new_second) == 24);
        assert(chunk_get_next(new_second) == third);

        chunk_free(first);
        chunk_free(new_second);
        chunk_free(third);
    }

    {
        void *first = chunk_alloc(16);
        void *second = chunk_alloc(8);
        void *temp = chunk_alloc(16);
        void *third = chunk_alloc(8);

        chunk_free(temp);

        void *new_second = chunk_realloc(second, 16);

        assert(new_second == second);
        assert(chunk_get_size(new_second) == 16);
        assert((size_t)chunk_get_next(new_second) - (size_t)temp == 8);
        assert(chunk_is_free(chunk_get_next(new_second)));

        chunk_free(first);
        chunk_free(new_second);
        chunk_free(third);
    }

    {
        void *first = chunk_alloc(16);
        void *second = chunk_alloc(4040);

        assert((size_t)second - (size_t)first == 24);
        assert(chunk_is_last(second) == true);

        void *new_first = chunk_realloc(first, 24);

        assert(chunk_is_free(first));
        assert(chunk_is_last(new_first) == false);
        assert(chunk_is_last(chunk_get_next(new_first)) == true);
        assert(memcmp(&((size_t *)first)[1], &((size_t *)new_first)[1], 16) ==
               0);

        chunk_free(new_first);
        chunk_free(second);
    }

    {
        void *first = chunk_alloc(16);
        void *second = chunk_alloc(16);
        void *third = chunk_alloc(8);
        void *fourth = chunk_alloc(8);

        chunk_free(third);

        assert(chunk_is_free(third));
        assert(chunk_get_size(third) == 8);

        void *new_second = chunk_realloc(second, 8);
        assert(second == new_second);

        void *new_third = chunk_get_next(new_second);

        assert((size_t)third - (size_t)new_third == 8);
        assert(chunk_get_size(new_third) == 16);
        assert(chunk_is_free(new_third));

        chunk_free(first);
        chunk_free(new_second);
        chunk_free(fourth);
    }

    {
        void *first = chunk_alloc(32);
        void *second = chunk_alloc(16);

        assert(chunk_get_next(first) == second);

        void *new_first = chunk_realloc(first, 16);

        assert(new_first == first);
        assert(chunk_get_next(new_first) != second);
        assert(chunk_is_free(chunk_get_next(new_first)));
        assert(chunk_get_next(chunk_get_next(new_first)) == second);

        chunk_free(new_first);
        chunk_free(second);
    }

    {
        void *first = chunk_alloc(16);
        void *second = chunk_alloc(16);

        assert(chunk_get_next(first) == second);
        assert(chunk_is_free(first) == false);
        assert(chunk_is_last(first) == false);
        assert(chunk_get_size(first) == 16);

        first = chunk_realloc(first, 16);

        assert(chunk_get_next(first) == second);
        assert(chunk_is_free(first) == false);
        assert(chunk_is_last(first) == false);
        assert(chunk_get_size(first) == 16);

        chunk_free(first);
        chunk_free(second);
    }

    {
        void *first = chunk_realloc(NULL, 16);
        void *keepalive = chunk_alloc(1);

        assert(chunk_is_free(first) == false);
        assert(chunk_get_size(first) == 16);

        void *null = chunk_realloc(first, 0);

        assert(null == NULL);
        assert(chunk_is_free(first) == true);
        assert(chunk_get_size(first) == 16);

        chunk_free(keepalive);
    }
}

int main() {
    test_block_alloc();
    test_block_get_capacity();
    test_block_get_next();
    test_block_free();

    test_chunk_alloc();
    test_chunk_realloc();
    test_chunk_free();

    return 0;
}
