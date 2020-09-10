#include "alokator.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

void * fun()
{
    heap_malloc(50);
    return NULL;
}

int main(void)
{
    //---Test heap_setup
    int status = heap_setup();
    assert(status == 0);

    //---Test glowny
    size_t free_bytes = heap_get_free_space();
    size_t used_bytes = heap_get_used_space();
    
    void* p1 = heap_malloc_debug(8 * 1024 * 1024, __LINE__, __FILE__); // 8MB
    void* p2 = heap_malloc_debug(8 * 1024 * 1024, __LINE__, __FILE__); // 8MB
    void* p3 = heap_malloc_debug(8 * 1024 * 1024, __LINE__, __FILE__); // 8MB
    void* p4 = heap_malloc_debug(45 * 1024 * 1024, __LINE__, __FILE__); // 45MB
    assert(p1 != NULL); // malloc musi się udać
    assert(p2 != NULL); // malloc musi się udać
    assert(p3 != NULL); // malloc musi się udać
    assert(p4 == NULL); // nie ma prawa zadziałać
    // Ostatnia alokacja, na 45MB nie może się powieść,
    // ponieważ sterta nie może być aż tak 
    // wielka (brak pamięci w systemie operacyjnym).

    // heap_dump_debug_information();
    status = heap_validate();
    assert(status == 0); // sterta nie może być uszkodzona

    // zaalokowano 3 bloki
    assert(heap_get_used_blocks_count() == 3);

    // zajęto 24MB sterty; te 2000 bajtów powinno
    // wystarczyć na wewnętrzne struktury sterty
    assert(
        heap_get_used_space() >= 24 * 1024 * 1024 &&
        heap_get_used_space() <= 24 * 1024 * 1024 + 2000
        );

    // zwolnij pamięć
    heap_free(p1);
    heap_free(p2);
    heap_free(p3);

    // wszystko powinno wrócić do normy
    // heap_dump_debug_information();
    assert(heap_get_free_space() == free_bytes);
    assert(heap_get_used_space() == used_bytes);

    // już nie ma bloków
    assert(heap_get_used_blocks_count() == 0);   

    //---Test heap_malloc
    assert(heap_malloc(4) != NULL);
    assert(heap_malloc(8*1024*1024) != NULL);
    assert(heap_malloc(0) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_calloc
    char * tab = (char *)heap_calloc(10, 1);
    assert(tab != NULL);
    for (int i = 0; i < 10; i++) assert(tab[i]==0);
    assert(heap_calloc(10, 0) == NULL);
    assert(heap_calloc(0, 1) == NULL);
    heap_free(tab);
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_realloc
    assert(heap_realloc(NULL, 4) != NULL);
    assert(heap_realloc(NULL, 0) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    void * mem = heap_realloc(NULL, 124);
    assert(mem != NULL);
    void * n_mem = heap_realloc(mem, 124);
    assert(n_mem != NULL);
    heap_realloc(n_mem, 0);
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_malloc_aligned
    assert(heap_malloc_aligned(4) != NULL);
    assert(heap_malloc_aligned(8*1024*1024) == NULL);
    assert(heap_malloc_aligned(0) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    assert(heap_malloc_aligned(3000) != NULL);
    assert(heap_malloc_aligned(3000) != NULL);
    assert(heap_malloc_aligned(3000) != NULL);
    assert(heap_malloc_aligned(5) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    assert(heap_malloc_aligned(3000) != NULL);
    void * space = heap_malloc_aligned(3000);
    assert(space != NULL);
    assert(heap_malloc_aligned(3000) != NULL);
    heap_free(space);
    assert(heap_malloc_aligned(5) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_calloc_aligned
    //---Z racji ze oba calloc_alignedi realloc_aligned wewnetrznie polegaja na malloc_aligned to
    //---Mechanizm alokacji _aligned przetestowalem w mallocu wyzej
    tab = (char *)heap_calloc_aligned(10, 1);
    assert(tab != NULL);
    for (int i = 0; i < 10; i++) assert(tab[i]==0);
    assert(heap_calloc_aligned(10, 0) == NULL);
    assert(heap_calloc_aligned(0, 1) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_realloc_aligned
    assert(heap_realloc_aligned(NULL, 4) != NULL);
    assert(heap_realloc_aligned(NULL, 0) == NULL);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    mem = heap_realloc_aligned(NULL, 124);
    assert(mem != NULL);
    n_mem = heap_realloc_aligned(mem, 124);
    assert(n_mem != NULL);
    heap_realloc_aligned(n_mem, 0);
    assert(heap_get_used_blocks_count() == 0);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_geat_used_space
    assert(heap_malloc(1000) != NULL);
    assert(heap_malloc(140) != NULL);
    assert(heap_get_used_space() >= 1140 && heap_get_used_space() <= 1332); //128B na struktury wewnetrzne + 64B na trzecia strukture
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_get_largest_used_block_size
    assert(heap_malloc(12) != NULL);
    assert(heap_malloc(6) != NULL);
    assert(heap_malloc(115) != NULL);
    assert(heap_get_largest_used_block_size() == 115);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_get_free_space
    assert(heap_get_free_space() == 16320); //Page size * 4 - sizeof(block) + fence_size * 2    
    //---Test heap_get_largest_free_area
    assert(heap_malloc(5000) != NULL);
    void * ptr = heap_malloc(6000);
    assert(ptr != NULL);
    assert(heap_malloc(3000) != NULL);
    heap_free(ptr);
    assert(heap_get_largest_free_area() == 6000);
    heap_reset();
    assert(heap_get_largest_free_area() == 16320);
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_get_block_size
    ptr = heap_malloc(125);
    assert(ptr != NULL);
    assert(heap_get_block_size(ptr) == 125);
    assert(heap_get_block_size(NULL) == 0);
    heap_free(ptr);
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_get_used_blocks_count
    assert(heap_get_used_blocks_count() == 0);
    assert(heap_malloc(12) != NULL);
    assert(heap_malloc(6) != NULL);
    assert(heap_malloc(115) != NULL);
    assert(heap_get_used_blocks_count() == 3);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_get_free_gaps_count
    void * ptr1 = heap_malloc(6000);
    assert(ptr1 != NULL);
    void * ptr2 = heap_malloc(6000);
    assert(ptr2 != NULL);
    void * ptr3 = heap_malloc(6000);
    assert(ptr3 != NULL);
    void * ptr4 = heap_malloc(6000);
    assert(ptr4 != NULL);
    void * ptr5 = heap_malloc(6000);
    assert(ptr5 != NULL);
    heap_free(ptr2);
    heap_free(ptr4);
    assert(heap_get_free_gaps_count() == 3); //Wolny blok zawsze po prawej ostatniego bloku
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test get_pointer_type
    ptr = heap_malloc(600);
    assert(ptr != NULL);
    assert(get_pointer_type(ptr) == pointer_valid);
    assert(get_pointer_type((void *)((unsigned char*)ptr + 1)) == pointer_inside_data_block);
    int a = 0;
    assert(get_pointer_type(&a) == pointer_out_of_heap);
    void * temp_ptr = (void *)(((unsigned char *)ptr) - sizeof(block) - fence_size);
    assert(get_pointer_type(temp_ptr) == pointer_control_block);
    assert(get_pointer_type(NULL) == pointer_null);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    ptr1 = heap_malloc(6000);
    assert(ptr1 != NULL);
    ptr2 = heap_malloc(6000);
    assert(ptr2 != NULL);
    ptr3 = heap_malloc(6000);
    assert(ptr3 != NULL);
    heap_free(ptr2);
    assert(get_pointer_type(ptr2) == pointer_unallocated);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_free
    ptr = heap_malloc(1);
    assert(ptr != NULL);
    heap_free(ptr);
    assert(heap_get_used_blocks_count() == 0);
    //---Test heap_validate
    assert(heap_validate() == 0);
    assert(heap_malloc(4) != NULL);
    assert(heap_validate() == 0);
    heap_reset();
    assert(heap_validate() == 0);
    assert(heap_get_used_blocks_count() == 0);
    ptr = heap_malloc(4);
    char * mod_ptr = (((unsigned char *)ptr) - 1);
    *mod_ptr = 33; //Naruszanie plotka 
    assert(heap_validate() == 4);
    heap_reset();
    ptr = heap_malloc(4);
    mod_ptr = (((unsigned char *)ptr) - fence_size - 1);
    *mod_ptr = 54; //Naruszenie struktury czyli suma kontrolna zwroci blad
    assert(heap_validate() == 2);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    //Testy z watkami
    const int size = 133;
    pthread_t thready[size];
    for (int i = 0; i < size; i++)
    {
        pthread_create(&thready[i], NULL, fun, NULL);
    }
    assert(heap_validate() == 0);
    heap_reset();
    assert(heap_get_used_blocks_count() == 0);
    return 0;
}
