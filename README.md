# Alocator

My malloc, calloc, realloc and free
Project provides implementation of thread-safe functions for dynamic allocation. <br>
Custom_sbrk derive memory from system, which we will manage. <br>

# How to run

## More details

For every allocation there is used control block, space for data and fences. <br>
Program is tested by unit tests for each function ('unit_test.c'), and simple program where we perform easy allocations and frees. Additionally there is part for thread-safe test, where we run 4 threads at the same time and similar as before they execute simple allocations and frees.
