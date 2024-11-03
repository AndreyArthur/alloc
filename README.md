# A Dinamic Memory Allocator written in c

This project is a simple, "reinvent-the-wheel" implementation of a memory allocator in C. Built as an educational exercise, it serves to understand the internal workings of dynamic memory allocation, mimicking functionalities similar to `malloc`, `free`, and `realloc`.

## Features

- **Heap Management**: Provides a page-aligned memory block allocation, managing blocks and chunks to optimize memory usage.
- **Chunk Handling**: Allocates and frees memory in smaller chunks within allocated blocks.
- **Reallocation**: Supports reallocating memory blocks with potential resizing for existing allocations.

## File Structure

- **src/heap.c & src/heap.h**: Implements the core of the heap management, defining block allocation, chunk creation, and chunk management functions.
- **src/memory.c & src/memory.h**: Defines higher-level functions, wrapping heap functionalities to mimic typical memory allocation (`memory_alloc`, `memory_realloc`, and `memory_free`).
- **tests**: Includes unit tests for validating block and memory allocation functions.

## Project Goals

This allocator project is intended to:

1. Gain a deeper understanding of low-level memory management.
2. Explore how `malloc`-like mechanisms could be implemented under the hood.

## License

This project is licensed under the terms specified in the `LICENSE` file.
