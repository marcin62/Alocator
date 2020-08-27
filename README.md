# Alocator

My malloc, calloc, realloc and free
Project provides implementation of thread-safe functions for dynamic allocation. <br>
Custom_sbrk derive memory from system, which we will manage. <br>

# How to run

gcc -o (nazwa) testy_jednostkowe.c alokator.c memmanager.c -pthread
./(nazwa)

# More details

For every allocation there is used control block, space for data and fences. <br>
Program is tested by unit tests for each function ('testy_jednostkowe.c'), and simple program where we perform easy allocations and frees.
