#include "mm/heap.h"
#include "mm/vmm.h"

#include "drivers/serial.h"

static uint64_t heap_cur = HEAP_VIRT_BASE;

uint8_t size_to_bin(uint64_t size) {
    for (int i = 0; i < NUM_BINS - 1; i++) {
        if (size <= bin_sizes[i]) return i;
    }
    return NUM_BINS - 1; 
}

void heap_init() {

    uint64_t virt = vmm_alloc(k_pml4, HEAP_VIRT_BASE, 0, PTE_WRITEABLE);

    // init heap block
    struct heap_block *block = (struct heap_block *)virt;
    
    block->size = PAGE_SIZE - sizeof(struct heap_block);
    block->free = 1;
    block->next = NULL;
    block->prev = NULL;

    // insert into bin
    uint8_t bin = size_to_bin(block->size);
    
    block->next = bins[bin];
    if (bins[bin]) bins[bin]->prev = block;
    bins[bin]   = block;

    heap_cur += PAGE_SIZE;
}

void *grow_heap(uint64_t size) {

    uint64_t total = size + sizeof(struct heap_block);
    uint8_t  order = 0;

    // find smallest order that fits size
    while (((uint64_t)PAGE_SIZE << order) < total) order++;

    uint64_t virt = vmm_alloc(k_pml4, heap_cur, order, PTE_WRITEABLE);
    if (!virt) return NULL;

    // new heap block
    struct heap_block *block = (struct heap_block *)virt;
    
    block->size = ((uint64_t)PAGE_SIZE << order) - sizeof(struct heap_block);
    block->free = 1;
    block->next = NULL;
    block->prev = NULL;

    // insert into bin
    uint8_t bin = size_to_bin(block->size);

    block->next = bins[bin];
    if (bins[bin]) bins[bin]->prev = block;
    bins[bin]   = block;

    heap_cur += ((uint64_t)PAGE_SIZE << order);

    // retry malloc
    return kmalloc(size);
}

void *kmalloc(uint64_t size) {
    if (!size) return NULL;
    uint64_t temp_sz = size + 32;
    serial_printf("Made it to (0) [%u] [%u].\n", (void*[]){&size, &temp_sz});
    size = (size + 15) & ~15ULL;
    uint8_t bin = size_to_bin(size);
    serial_printf("Made it to (1) [%u] [%u].\n", (void*[]){&size, &temp_sz});

    // search all bins >= size
    for (uint64_t i = bin; i < NUM_BINS; i++) {
        struct heap_block *cur  = bins[i];
        struct heap_block *prev = NULL;
        serial_printf("Made it to (2) [%u].\n", (void*[]){&i});

        while (cur) {
            if (cur->size >= size) {
                // remove node from bin
                if (prev) prev->next = cur->next;
                else      bins[i]    = cur->next; 
                if (cur->next) cur->next->prev = prev;

                cur->next = NULL;
                cur->prev = NULL;

                // if >= 16 bytes leftover, split that into another bin
                uint64_t remaining = cur->size - size;
                serial_printf("Made it to (3).\n", (void*[]){});
                
                if (remaining >= sizeof(struct heap_block) + MIN_SPLIT_SZ) {
                    struct heap_block *leftover = 
                    (struct heap_block *)((uint8_t *)(cur + 1) + size);
                    leftover->size = remaining - sizeof(struct heap_block);
                    leftover->free = 1;

                    int leftover_bin = size_to_bin(leftover->size);
                    leftover->next   = bins[leftover_bin];

                    if (bins[leftover_bin]) bins[leftover_bin]->prev = leftover;
                    leftover->prev = NULL;

                    bins[leftover_bin] = leftover;
                    cur->size = size;
                    serial_printf("%u bytes remaining.\n", (void*[]){&leftover->size});
                }
                temp_sz = size + 32;
                serial_printf("allocated %u bytes.\n", (void*[]){&temp_sz});
                cur->free = 0;
                return (void *)(cur + 1);
            }
            prev = cur;
            cur  = cur->next;
        }
    }

    // if not, allocate new page and pass it to the user
    return grow_heap(size);
}

void kfree(void *ptr) {

    // make new heap_block
    struct heap_block *block = (struct heap_block *)ptr - 1;
    block->free = 1;
    
    // check forward neighbor
    struct heap_block *neighbor 
        = (struct heap_block *)((uint8_t *)(block + 1) + block->size);
    
    // while less than heap cur and free
    while ((uint64_t)neighbor < heap_cur && neighbor->free){
        // block grows
        block->size   += sizeof(struct heap_block) + neighbor->size;
        
        // neighbor not free
        neighbor->free = 0;

        // remove from bin
        uint8_t nb_bin = size_to_bin(neighbor->size);
        if (neighbor->prev) neighbor->prev->next = neighbor->next;
        else                bins[nb_bin] = neighbor->next;

        if (neighbor->next) neighbor->next->prev = neighbor->prev;

        // next neighbor
        neighbor = (struct heap_block *)((uint8_t *)(block + 1) + block->size);
    }
    
    // add to its new bin
    uint8_t bin = size_to_bin(block->size);
    
    block->next = bins[bin];
    if (bins[bin]) bins[bin]->prev = block;
    bins[bin] = block;
}

void heap_test() {
    // 4096 bytes on heap to start
    // 4000 bytes after a kmalloc'd
    void *a = kmalloc(50 - sizeof(struct heap_block));

    // 928 bytes after b kmalloc'd (only bins[6] populated)
    void *b = kmalloc(3072 - sizeof(struct heap_block));
    if (!bins[6]) {serial_print("[heap_test] bins[6] check FAILED!\n"); return ;}
    uint64_t free = bins[6]->free;
    serial_printf(
        "[heap_test] bins[6]:\n"
        "[heap_test] \tsize: %u\n"
        "[heap_test] \tfree: %d\n", 
        (void*[]){&bins[6]->size, &free}
    );

    // 16 left after c kmalloc'd
    void *c = kmalloc(912 - sizeof(struct heap_block));
    if (!bins[0]) { serial_print("[heap_test] bins[0] check FAILED!\n"); return; }
    free = bins[0]->free;
    serial_printf(
        "[heap_test] bins[0]:\n"
        "[heap_test] \tsize: %u\n"
        "[heap_test] \tfree: %d\n", 
        (void*[]){&bins[0]->size, &free}
    );

    // need new page after d is kmalloc'd
    void *d = kmalloc(64 - sizeof(struct heap_block));
    if (!bins[7]) { serial_print("[heap_test] bins[7] check FAILED!\n"); return; }
    free = bins[7]->free;
    serial_printf(
        "[heap_test] bins[7]:\n"
        "[heap_test] \tsize: %u\n"
        "[heap_test] \tfree: %d\n", 
        (void*[]){&bins[7]->size, &free}
    );

    kfree(a);
    kfree(b);
    kfree(c);
    kfree(d);
    
}