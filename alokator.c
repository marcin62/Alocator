#include "alokator.h"
#include "pthread.h"

pthread_mutex_t mem_mutex;

//Deklaracje zmiennych
heap mem_heap;

void heap_free(void * adres)
{
    if (get_pointer_type(adres) != pointer_valid) return;
    block * Block = (block *)(((unsigned char *)adres) - sizeof(block) - fence_size);
    if (heap_validate()!=0) {printf("Heap_validate\n"); return;}
    pthread_mutex_lock(&mem_mutex);
    Block->status=free;
    Block->suma_kontrolna = 0;
    Block->suma_kontrolna = policz_sume(Block, sizeof(block));
    merge_blocks();
    pthread_mutex_unlock(&mem_mutex);
    if (!heap_get_used_blocks_count()) heap_reset();
}
void heap_dump_debug_information()
{
    pthread_mutex_lock(&mem_mutex);
    printf("Informacje o heapie:\n");
    printf("Adres heapa: %p\n", mem_heap.heap);
    printf("Rozmiar heapa: %lu\n", mem_heap.heap_rozmiar);
    printf("Suma kontrolna heapa: %u", mem_heap.suma_kontrolna);
    printf("\nInformacje o blockach w heapie:\n");
    int block_nr = 0;
    block * Block = (block *)mem_heap.heap;
    while(Block != NULL)
    {
        printf("\nNumer porzadkowy bloku: %d\n", block_nr++);
        printf("Rozmiar bloku: %lu\n", Block->rozmiar);
        printf("Status bloku: %d\n", Block->status);
        printf("Suma kontrolna bloku: %u\n", Block->suma_kontrolna);
        printf("Adres bloku: %p\n", Block);
        printf("Wskaznik na nastepny blok: %p\n", Block->next);
        printf("Wskaznik na poprzedni blok: %p\n", Block->prev);
        Block = Block->next;
    }
    printf("\nKoniec blokow\n");
    pthread_mutex_unlock(&mem_mutex);
}
void more_heap_space(size_t bajty)
{
    //---Proba pobrania wiekszej ilosci pamieci
    if (heap_full(bajty))
    {
        size_t nowy_rozmiar_sterty = to_ps(bajty + control_size);
        if (custom_sbrk(nowy_rozmiar_sterty) == mem_null) {
            printf("Proba pobrania wiekszej ilosci pamieci od systemu nie powiodla sie\n");
            return;
        }
        //---Aktualizacja struktury sterty
        mem_heap.heap_rozmiar += to_ps(bajty + control_size);
        mem_heap.suma_kontrolna = 0;
        mem_heap.suma_kontrolna = policz_sume(&mem_heap, sizeof(mem_heap));
        //---Lapiemy ostatni block na stercie
        block * Block = (block *)mem_heap.heap;
        while (Block)
        {
            if (Block -> next == NULL) break;
            Block = Block->next;
        }
        //Tworzymy nowy block o rozmiarze nowy_rozmiar_sterty - control_size
        create_new_block(Block, nowy_rozmiar_sterty - control_size);
        //Scalamy nowy blok jezeli jest to mozliwe
        merge_blocks();    
    }
}
void setup_plotek(void * adres, size_t rozmiar)
{
    char plotek[fence_size];
    int i = 0;
    while (i < fence_size) {plotek[i] = i; i++;}
    memcpy(((unsigned char *)adres) + sizeof(block), plotek, fence_size);
    memcpy(((unsigned char *)adres) + sizeof(block) + fence_size + rozmiar, plotek, fence_size);
}
int divide(block * block_to_be_divided, size_t size_of_new_block)
{
    if (block_to_be_divided == NULL) return 0;
    size_t size_of_right_block = block_to_be_divided->rozmiar - size_of_new_block - control_size;
    block * block_next = block_to_be_divided->next;
    block_to_be_divided->rozmiar=size_of_new_block;
    block_to_be_divided->next=(block *)((unsigned char *)block_to_be_divided + size_of_new_block + control_size);
    block_to_be_divided->suma_kontrolna = 0;
    block_to_be_divided->suma_kontrolna = policz_sume(block_to_be_divided, sizeof(block));
    if (block_next != NULL)
    {
        block_next->prev = (block *)((unsigned char *)block_to_be_divided + size_of_new_block + control_size);
        block_next->suma_kontrolna=0;
        block_next->suma_kontrolna=policz_sume(block_to_be_divided, sizeof(block));
    }
    setup_plotek(block_to_be_divided, size_of_new_block);
    block Block;
    Block.status = free;
    Block.rozmiar = size_of_right_block;
    Block.next = block_next;
    Block.prev = block_to_be_divided;
    Block.suma_kontrolna=0;
    Block.suma_kontrolna=policz_sume(&Block, sizeof(block));
    memcpy(((unsigned char *)block_to_be_divided + size_of_new_block + control_size), &Block, sizeof(block));
    setup_plotek(((unsigned char *)block_to_be_divided + size_of_new_block + control_size), size_of_right_block);
    return 1;
}
int merge_blocks()
{
    block * Block = (block *)mem_heap.heap;
    while (Block)
    {
        if (Block->next != NULL)
        {
            if (Block->status==free && Block->next->status==free)
            {
                Block->rozmiar = (Block->rozmiar + Block->next->rozmiar + sizeof(block) + fence_size);
                if (Block->next->next != NULL)
                {
                    Block->next->next->prev = Block;
                    Block->next->next->suma_kontrolna = 0;
                    Block->next->next->suma_kontrolna = policz_sume(Block->next->next, sizeof(block));
                }
                Block->next=Block->next->next;
                Block->suma_kontrolna = 0;
                Block->suma_kontrolna = policz_sume(Block, sizeof(block));
                setup_plotek(Block, Block->rozmiar);
            }
        }
        Block = Block -> next;
    }
    return 0;
}
int heap_reset()
{
    if (custom_sbrk(mem_heap.heap_rozmiar*-1) == mem_null) return -1;
    pthread_mutex_destroy(&mem_mutex);
    return heap_setup();
}
int heap_validate()
{
    //---Sprwadzanie struktury sterty
    if (mem_heap.heap == mem_null) return 1;
    if (mem_heap.heap == NULL) return 1;
    pthread_mutex_lock(&mem_mutex);
    size_t s_k = mem_heap.suma_kontrolna;
    mem_heap.suma_kontrolna = 0;
    mem_heap.suma_kontrolna = policz_sume(&mem_heap, sizeof(mem_heap));
    if (s_k != mem_heap.suma_kontrolna)
    {
        printf("Bledna suma kontrolna sterty\n");
        pthread_mutex_unlock(&mem_mutex);
        return 2;
    } 
    char plotek[fence_size];
    int i = 0;
    while (i < fence_size) {plotek[i] = i; i++;}
    //---Sprawdzanie blokow sterty 
    block * Block = (block *)mem_heap.heap;
    while (Block)
    {
        if (Block->rozmiar == 0) 
        {
            printf("Bledny rozmiar blocku o adresie: %p\n", Block);
            pthread_mutex_unlock(&mem_mutex);
            return 3;
        }
        size_t s_kb = Block->suma_kontrolna;
        Block->suma_kontrolna = 0;
        Block->suma_kontrolna = policz_sume(Block, sizeof(block));
        if (s_kb != Block->suma_kontrolna)
        {
            printf("Bledna suma kontrolna blocku o adresie: %p\n", Block);
            pthread_mutex_unlock(&mem_mutex);
            return 2;
        }
        int j = 0;
        char * plotek1 = (char *)(((unsigned char *)Block) + sizeof(block));
        char * plotek2 = (char *)(((unsigned char *)Block) + sizeof(block) + fence_size + Block->rozmiar);
        while (j < fence_size)
        {
            if (plotek[j] != plotek1[j])
            {
                printf("Bledny lewy plotek bloku: %p\n", Block);
                pthread_mutex_unlock(&mem_mutex);
                return 4;
            }
            if (plotek[j] != plotek2[j])
            {
                printf("Bledny prawy plotek bloku: %p\n", Block);
                pthread_mutex_unlock(&mem_mutex);
                return 4;
            }
            j++;
        }
        Block=Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return 0;
}
int heap_setup()
{
    //---Pobieramy przestrzen adresowa od systemu
    mem_heap.heap = custom_sbrk(starting_size);
    if(mem_heap.heap == mem_null) return -1;
    mem_heap.heap_rozmiar = starting_size;
    //---Tworzymy pierwszy pusty blok na dlugosc calego heapa
    block Block;
    Block.next = NULL;
    Block.prev = NULL;
    Block.rozmiar = starting_size - control_size;
    Block.status = free;
    Block.suma_kontrolna = 0;
    Block.suma_kontrolna = policz_sume(&Block, sizeof(Block));
    //---Wstawiamy ten blok na poczatek heapa
    memcpy(mem_heap.heap, &Block, sizeof(Block));
    setup_plotek(mem_heap.heap, Block.rozmiar);
    //---Odswiezamy sume kontrolna heapa
    mem_heap.suma_kontrolna = 0;
    mem_heap.suma_kontrolna = policz_sume(&mem_heap, sizeof(mem_heap));
    pthread_mutex_init(&mem_mutex, NULL);
    return 0;
}
int heap_full(size_t bajty)
{
    block * Block = (block *)mem_heap.heap;
    while (Block)
    {
        if(Block->next==NULL) break;
        Block = Block->next;
    }
    return !(Block->status==free && Block->rozmiar >= bajty + control_size);
}
void * heap_malloc(size_t bajty)
{
    if (bajty == 0) return NULL;
    pthread_mutex_lock(&mem_mutex);
    //---Sprawdzenie czy na heapie jest wystarczajaco miejsca
    //---Jesli brakuje miejsca to prosimy system o wiecej
    more_heap_space(bajty);
    //---Zapewnilismy miejsce na stercie, szukamy odpowidniego bloku
    block * cor_Block = NULL;
    block * Block = (block *)mem_heap.heap;
    while (Block != NULL)
    {
        if (Block->rozmiar==bajty && Block->status==free) {
            cor_Block = Block;
            break;
        }
        if (Block->rozmiar>(bajty+control_size) && Block->status==free)
        {
            if (divide(Block, bajty)) {
                cor_Block = Block; 
                break;
            }
            else 
            {
                pthread_mutex_unlock(&mem_mutex); 
                return NULL;
            }
        }
        if (Block->next==NULL) break;
        Block = Block->next;
    }
    //---Ustawiamy uzyskany blok i zwracamy adres danych.
    if (cor_Block == NULL) 
    {
        pthread_mutex_unlock(&mem_mutex); 
        return NULL;
    }
    cor_Block->status=taken;
    cor_Block->suma_kontrolna=0;
    cor_Block->suma_kontrolna=policz_sume(cor_Block, sizeof(block));
    pthread_mutex_unlock(&mem_mutex);
    return (void *)(((unsigned char *)cor_Block) + control_size - fence_size);
}
void * heap_calloc(size_t bajty, size_t mnoznik)
{
    if (!bajty || !mnoznik) return NULL;
    char * adres = heap_malloc(bajty * mnoznik);
    if (adres==NULL) return NULL;
    pthread_mutex_lock(&mem_mutex);
    for (int i = 0; i < bajty * mnoznik; i++) adres[i]=0;
    pthread_mutex_unlock(&mem_mutex);
    return (void *)adres; 
}
void * heap_realloc(void * adres, size_t nowy_rozmiar)
{
    if (adres == NULL && nowy_rozmiar > 0) return heap_malloc(nowy_rozmiar);
    if (adres == NULL && nowy_rozmiar == 0) return NULL;
    if (nowy_rozmiar == 0)
    {
        heap_free(adres);
        return NULL;
    }
    void * nowy_adres = heap_malloc(nowy_rozmiar);
    if (nowy_adres == NULL) return adres;
    pthread_mutex_lock(&mem_mutex);
    memcpy(nowy_adres, adres, nowy_rozmiar);
    pthread_mutex_unlock(&mem_mutex);
    heap_free(adres);
    return nowy_adres;
}
void * heap_malloc_aligned(size_t bajty)
{
    //---Zasada dziala algorytmu:
    //---Przechodzenie po stercie co page_size,
    //---Sprawdzanie czy znalezlismy sie w wolnej przestrzeni
    //---Sprawdzanie czy jest wystarczajaco miejsca do wstawienia bloku na poczatku strony
    //---Jesli wszystko sie zgadza to splitujemy tyle razy ile sie da (1 lub 2)
    //---Zwracamy blok o danym rozmiarze bajtow uzytkownikowi
    if (!bajty) return NULL;
    unsigned char * sterta = mem_heap.heap;
    size_t sterta_rozmiar = mem_heap.heap_rozmiar;
    if (sterta_rozmiar-page_size==0) return NULL;
    pthread_mutex_lock(&mem_mutex);
    for (size_t i = 0; i < sterta_rozmiar; i+=page_size)
    {
        pthread_mutex_unlock(&mem_mutex);
        if (get_pointer_type(sterta+i)!=pointer_inside_data_block) 
        {
            pthread_mutex_lock(&mem_mutex);
            continue;
        }
        block * Block = (block *)(((unsigned char *)heap_get_data_block_start(sterta + i)-sizeof(block)-fence_size));
        pthread_mutex_lock(&mem_mutex);
        if (Block->status==free)
        {
            size_t odleglosc_do_struktury = ((sterta+i)-(((unsigned char *)Block)+sizeof(block)+fence_size));
            if (odleglosc_do_struktury == 0)
            {
                if (Block->rozmiar > (bajty + control_size))
                {
                    divide(Block, bajty);
                }
                pthread_mutex_unlock(&mem_mutex);
                return (void *)(((unsigned char *)Block)+sizeof(block)+fence_size);
            }
            else
            {
                if (odleglosc_do_struktury > control_size)
                {
                    divide(Block, odleglosc_do_struktury-control_size);
                    Block = Block->next;
                    if (Block->rozmiar > (bajty + control_size))
                    {
                        divide(Block, bajty);
                    }
                    pthread_mutex_unlock(&mem_mutex);
                    return (void *)(((unsigned char *)Block)+sizeof(block)+fence_size);
                }
            }            
        }
    }
    pthread_mutex_unlock(&mem_mutex);
    return NULL;
}
void * heap_calloc_aligned(size_t bajty, size_t mnoznik)
{
    if (!bajty || !mnoznik) return NULL;
    char * adres = heap_malloc_aligned(bajty * mnoznik);
    pthread_mutex_lock(&mem_mutex);
    for (int i = 0; i < bajty*mnoznik; i++) adres[i] = 0;
    pthread_mutex_unlock(&mem_mutex);
    return (void *)adres; 
}
void * heap_realloc_aligned(void * adres, size_t nowy_rozmiar)
{
    if (adres == NULL) return heap_malloc_aligned(nowy_rozmiar);
    if (nowy_rozmiar == 0)
    {
        heap_free(adres);
        return NULL;
    }
    void * nowy_adres = heap_malloc_aligned(nowy_rozmiar);
    if (nowy_adres == NULL) return adres;
    pthread_mutex_lock(&mem_mutex);
    memcpy(nowy_adres, adres, nowy_rozmiar);
    pthread_mutex_unlock(&mem_mutex);
    heap_free(adres);
    return nowy_adres;
}
void * heap_malloc_debug(size_t bajty, int linia, const char * f_name)
{
    if (bajty == 0)
    {
        printf("Wywolano funkcje malloc podajac 0 bajtow w argumencie. Linia: %d, Plik: %s\n", linia, f_name);
        return NULL;
    }
    if (heap_validate()!=0)
    {
        printf("Walidacja w fukncji malloc nie powiodla sie");
        return NULL;
    }
    pthread_mutex_lock(&mem_mutex);
    //---Sprawdzenie czy na heapie jest wystarczajaco miejsca
    //---Jesli brakuje miejsca to prosimy system o wiecej
    more_heap_space(bajty);
    //---Zapewnilismy miejsce na stercie, szukamy odpowidniego bloku
    block * cor_Block = NULL;
    block * Block = (block *)mem_heap.heap;
    while (Block != NULL)
    {
        if (Block->rozmiar==bajty && Block->status==free) {
            cor_Block = Block;
            break;
        }
        if (Block->rozmiar>(bajty+control_size) && Block->status==free)
        {
            if (divide(Block, bajty)) {
                cor_Block = Block; 
                break;
            }
            else 
            {
                pthread_mutex_unlock(&mem_mutex);
                return NULL;
            }
        }
        if (Block->next==NULL) break;
        Block = Block->next;
    }
    //---Ustawiamy uzyskany blok i zwracamy adres danych.
    if (cor_Block == NULL) 
    {
        pthread_mutex_unlock(&mem_mutex); 
        return NULL;
    }
    cor_Block->status=taken;
    cor_Block->suma_kontrolna=0;
    cor_Block->suma_kontrolna=policz_sume(cor_Block, sizeof(block));
    pthread_mutex_unlock(&mem_mutex);
    return (void *)(((unsigned char *)cor_Block) + control_size - fence_size);
}
void * heap_calloc_debug(size_t bajty, size_t mnoznik, int linia, const char * f_name)
{
    if (!bajty || !mnoznik)
    {
        printf("Wywolanie funkcji calloc nie powiodlo sie, jeden z podanych argumentow jest zerowy. Linia: %d Plik: %s\n", linia, f_name);
        return NULL;
    }
    if (heap_validate()!=0)
    {
        printf("Walidacja w fukncji calloc nie powiodla sie");
        return NULL;
    }
    char * adres = heap_malloc(bajty * mnoznik);
    if (adres==NULL)
    {
        printf("Wywolanie funkcji malloc w funkcji calloc zwrocilo null. Linia: %d Plik: %s\n", linia, f_name);
    }
    pthread_mutex_lock(&mem_mutex);
    for (int i = 0; i < bajty * mnoznik; i++) adres[i]=0;
    pthread_mutex_unlock(&mem_mutex);
    return (void *)adres; 
}
void * heap_realloc_debug(void * adres, size_t nowy_rozmiar, int linia, const char * f_name)
{
    if (adres == NULL  && nowy_rozmiar > 0)
    {
        printf("Wywolanie funkcji realloc podano null jako adres, zwracanie malloca. Linia: %d Plik: %s\n", linia, f_name);
        return heap_malloc(nowy_rozmiar);
    }
    if (adres == NULL && nowy_rozmiar == 0) 
    {
        printf("Wywolanie funkcji realloc podano null jako adres, oraz rozmiar 0, zwracanie NULL. Linia: %d Plik: %s\n", linia, f_name);
        return NULL;
    }
    if (nowy_rozmiar == 0)
    {
        printf("Do funkcji realloc podano rozmiar rowny zeru, zwalnianie adresu...\n");
        heap_free(adres);
        return NULL;
    }
    if (heap_validate()!=0)
    {
        printf("Walidacja w fukncji realloc nie powiodla sie");
        return NULL;
    }
    void * nowy_adres = heap_malloc(nowy_rozmiar);
    if (nowy_adres == NULL)
    {
        printf("Funkcja malloc w funkcji realloc zwrocila NULL, zwracanie starego adresu. Linia: %d Plik: %s\n", linia, f_name);
        return adres;
    }
    pthread_mutex_lock(&mem_mutex);
    memcpy(nowy_adres, adres, nowy_rozmiar);
    pthread_mutex_unlock(&mem_mutex);
    heap_free(adres);
    return nowy_adres;
}
void * heap_malloc_aligned_debug(size_t bajty, int linia, const char * f_name)
{
    if (!bajty)
    {
        printf("Wywolanie funkcji malloc aligned nie powiodlo sie, podano zerowe bajty. Linia: %d Plik: %s\n", linia, f_name);
        return NULL;
    }
    unsigned char * sterta = mem_heap.heap;
    size_t sterta_rozmiar = mem_heap.heap_rozmiar;
    if (sterta_rozmiar-page_size==0) 
    {
        printf("Rozmiar sterty jest za maly dla malloc_aligned\n");
        return NULL;
    }
    pthread_mutex_lock(&mem_mutex);
    for (size_t i = 0; i < sterta_rozmiar; i+=page_size)
    {
        if (get_pointer_type(sterta+i)!=pointer_inside_data_block) continue;
        block * Block = (block *)(((unsigned char *)heap_get_data_block_start(sterta + i)-sizeof(block)-fence_size));
        if (Block->status==free)
        {
            size_t odleglosc_do_struktury = ((sterta+i)-(((unsigned char *)Block)+sizeof(block)+fence_size));
            if (odleglosc_do_struktury == 0)
            {
                if (Block->rozmiar > (bajty + control_size))
                {
                    int wynik_divide = divide(Block, bajty);
                    if (!wynik_divide)
                    {
                        printf("Nie udalo sie podzielic blokow w funkcji malloc aligned. Linia: %d Plik: %s\n", linia, f_name);
                        pthread_mutex_unlock(&mem_mutex);
                        return NULL;
                    }
                }
                Block->status=taken;
                Block->suma_kontrolna=0;
                Block->suma_kontrolna=policz_sume(Block, sizeof(block));
                pthread_mutex_unlock(&mem_mutex);
                return (void *)(((unsigned char *)Block)+sizeof(block)+fence_size);
            }
            else
            {
                if (odleglosc_do_struktury > control_size)
                {
                    int wynik_divide = divide(Block, odleglosc_do_struktury-control_size);
                    if (!wynik_divide)
                    {
                        printf("Nie udalo sie podzielic blokow w funkcji malloc aligned. Linia: %d Plik: %s\n", linia, f_name);
                        pthread_mutex_unlock(&mem_mutex);
                        return NULL;
                    }
                    
                    Block = Block->next;
                    if (Block->rozmiar > (bajty + control_size))
                    {
                        wynik_divide = divide(Block, bajty);
                        if (!wynik_divide)
                        {
                            printf("Nie udalo sie podzielic blokow w funkcji malloc aligned. Linia: %d Plik: %s\n", linia, f_name);
                            pthread_mutex_unlock(&mem_mutex);
                            return NULL;
                        }
                    }
                    Block->status=taken;
                    Block->suma_kontrolna=0;
                    Block->suma_kontrolna=policz_sume(Block, sizeof(block));
                    pthread_mutex_unlock(&mem_mutex);
                    return (void *)(((unsigned char *)Block)+sizeof(block)+fence_size);
                }
            }            
        }
    }
    pthread_mutex_unlock(&mem_mutex);
    return NULL;
}
void * heap_calloc_aligned_debug(size_t bajty, size_t mnoznik, int linia, const char * f_name)
{
    if (!bajty || !mnoznik) return NULL;
    char * adres = heap_malloc_aligned(bajty * mnoznik);
    pthread_mutex_lock(&mem_mutex);
    for (int i = 0; i < bajty*mnoznik; i++) adres[i] = 0;
    pthread_mutex_unlock(&mem_mutex);
    return (void *)adres; 
}
void * heap_realloc_aligned_debug(void * adres, size_t nowy_rozmiar, int linia, const char * f_name)
{
    if (adres == NULL) return heap_malloc_aligned(nowy_rozmiar);
    if (nowy_rozmiar == 0)
    {
        heap_free(adres);
        return NULL;
    }
    void * nowy_adres = heap_malloc_aligned(nowy_rozmiar);
    if (nowy_adres == NULL) return adres;
    pthread_mutex_lock(&mem_mutex);
    memcpy(nowy_adres, adres, nowy_rozmiar);
    pthread_mutex_unlock(&mem_mutex);
    heap_free(adres);
    return nowy_adres;
}
void * heap_get_data_block_start(const void * adres)
{
    if (adres == NULL) return 0;
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    while(Block != NULL)
    {
        if (adres >= (void *)Block && adres <= (void *)(((unsigned char *)Block)+Block->rozmiar+control_size)) break;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return (void *)(((unsigned char *)Block) + sizeof(block) + fence_size);
}
size_t heap_get_used_space()
{
    return mem_heap.heap_rozmiar - heap_get_free_space();
}
size_t heap_get_largest_used_block_size()
{
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    size_t largest_used_block_size = Block->rozmiar;
    while(Block != NULL)
    {
        if (Block->rozmiar > largest_used_block_size && Block->status == taken) largest_used_block_size = Block->rozmiar;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return largest_used_block_size;
}
size_t heap_get_free_space()
{
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    size_t free_space = 0;
    while(Block != NULL)
    {
        if (Block->status == free) free_space += Block->rozmiar;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return free_space;
}
size_t heap_get_largest_free_area()
{
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    size_t largest_free_area = Block->rozmiar;
    while(Block != NULL)
    {
        if (Block->rozmiar > largest_free_area && Block->status == free) largest_free_area = Block->rozmiar;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return largest_free_area;
}
size_t heap_get_block_size(const const void * adres)
{
    if (adres == NULL) return 0;
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    size_t block_size = 0;
    while(Block != NULL)
    {
        if (adres >= (void *)Block && adres <= (void *)(((unsigned char *)Block)+Block->rozmiar+control_size))
        {
            block_size = Block->rozmiar;
            break;
        }
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return block_size;
}
size_t policz_sume(void * adres, size_t dlugosc)
{
    if (adres==NULL) return 0;
    size_t suma_kontrolna = 0;
    int i = 0;
    while (i < dlugosc)
    {
        suma_kontrolna += *(((unsigned char *)adres)+i);
        i++;
    }
    return suma_kontrolna;
}
uint64_t heap_get_used_blocks_count()
{
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    uint64_t used_blocks_count = 0;
    while(Block != NULL)
    {
        if (Block->status == taken) used_blocks_count++;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return used_blocks_count;
}
uint64_t heap_get_free_gaps_count()
{
    pthread_mutex_lock(&mem_mutex);
    block * Block = (block *)mem_heap.heap;
    uint64_t free_gaps_count = 0;
    while(Block != NULL)
    {
        if (Block->status == free) free_gaps_count++;
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return free_gaps_count;
}
enum pointer_type_t get_pointer_type(const const void * adres)
{
    if (adres == NULL) return pointer_null;
    if (adres < mem_heap.heap || adres > (void *)(((unsigned char *)mem_heap.heap) + mem_heap.heap_rozmiar)) return pointer_out_of_heap;
    block * Block = (block *)mem_heap.heap;
    pthread_mutex_lock(&mem_mutex);
    while(Block != NULL)
    {
        if (adres == (void *)(((unsigned char *)Block) + control_size - fence_size))
        {
            if (Block->status == taken) 
            {
                pthread_mutex_unlock(&mem_mutex);
                return pointer_valid;
            }
            if (Block->status == free)
            {
                pthread_mutex_unlock(&mem_mutex);
                return pointer_unallocated;
            }
        }
        if (adres > (void *)(((unsigned char *)Block) + control_size - fence_size) && adres <= (void *)((unsigned char *)Block) + Block->rozmiar + control_size)
        {
            pthread_mutex_unlock(&mem_mutex);
            return pointer_inside_data_block;
        }
        Block = Block->next;
    }
    pthread_mutex_unlock(&mem_mutex);
    return pointer_control_block;
}

block * create_new_block(block * last_block, size_t rozmiar)
{
    block new_block;
    new_block.prev = last_block;
    new_block.rozmiar = rozmiar;
    new_block.status = free;
    new_block.next = NULL;
    new_block.suma_kontrolna = 0;
    new_block.suma_kontrolna = policz_sume(&new_block, sizeof(block));
    last_block->next = (block *)((unsigned char *)last_block + last_block->rozmiar + control_size);
    last_block->suma_kontrolna = 0;
    last_block->suma_kontrolna = policz_sume(last_block, sizeof(block));
    memcpy(((unsigned char *)last_block + last_block->rozmiar + control_size), &new_block, sizeof(block));
    setup_plotek((void *)((unsigned char *)last_block + last_block->rozmiar + control_size), rozmiar);
    return (block *)((unsigned char *)last_block + last_block->rozmiar + control_size);
}
