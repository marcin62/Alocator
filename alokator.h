#ifndef _ALOKATOR_
#define _ALOKATOR_

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "custom_unistd.h"

#define fence_size 16
#define page_size 4096
#define starting_size page_size * 4
#define control_size (sizeof(struct block_t) + fence_size * 2)
#define mem_null ((void *)-1)
#define to_ps(bajty) ((bajty + page_size - 1) / page_size) * page_size

enum pointer_type_t
{
    pointer_null,
    pointer_out_of_heap,
    pointer_control_block,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

typedef enum
{
    free,
    taken
} block_status_t;

typedef struct block_t
{
    size_t rozmiar;
    block_status_t status;
    int suma_kontrolna;
    struct block_t * next;
    struct block_t * prev;
} block;

typedef struct heap_t
{
    int suma_kontrolna;
    size_t heap_rozmiar;
    void * heap;
} heap;

void heap_free(void *);
void heap_dump_debug_information();
void more_heap_space(size_t);
void setup_plotek(void *, size_t);
int divide(block *, size_t);
int merge_blocks();    
int heap_reset();
int heap_validate();
int heap_setup();
int heap_full(size_t);
void * heap_malloc(size_t);
void * heap_calloc(size_t, size_t);
void * heap_realloc(void *, size_t);
void * heap_malloc_aligned(size_t);
void * heap_calloc_aligned(size_t, size_t);
void * heap_realloc_aligned(void *, size_t);
void * heap_malloc_debug(size_t, int, const char *);
void * heap_calloc_debug(size_t, size_t, int, const char *);
void * heap_realloc_debug(void *, size_t, int, const char *);
void * heap_malloc_aligned_debug(size_t, int, const char *);
void * heap_calloc_aligned_debug(size_t, size_t, int, const char *);
void * heap_realloc_aligned_debug(void *, size_t, int, const char *);
void * heap_get_data_block_start(const void *);
size_t heap_get_used_space();
size_t heap_get_largest_used_block_size();
size_t heap_get_free_space();
size_t heap_get_largest_free_area();
size_t heap_get_block_size(const const void *);
size_t policz_sume(void *, size_t);
uint64_t heap_get_used_blocks_count();
uint64_t heap_get_free_gaps_count();
enum pointer_type_t get_pointer_type(const const void *);
block * create_new_block(block *, size_t);
#endif


