#include "mm/heap.h"
#include "mm/vmm.h"

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
    block->next = null;
    block->prev = null;

    uint8_t bin = size_to_bin(block->size);
    bins[bin]   = block;
}
