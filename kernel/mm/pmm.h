#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_ORDER 10

/* ------------------------------------------------------------------------- *\
|                                                                             |
| external name: kernel_end                                                   |
| description:   if you check linker.ld, you will see a descriptor named      |
|                'kernel_end'. this descriptor is located directly after the  |
|                .bss section which means it holds the address of the very end|
|                of our kernel. needed to resereve the kernel's memory in     |
|                pmm_init.                                                    |
|                                                                             |
\* ------------------------------------------------------------------------- */
extern uint8_t kernel_end;   

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: pmm_init                                                     |
| description:   initialize the physical memory space allocated by multiboot2.|
|                we do so using buddy allocation (i.e. we allocate our memory |
|                in 'blocks' of increasing PAGE_SIZE * 2^n where n <= 10. for |
|                each order [n = order above], we have a bitmap telling us    |
|                which blocks for the given order are free AND we have a      |
|                linked list pointing to all of our free blocks at this order)|
|                                                                             |
\* ------------------------------------------------------------------------- */
void pmm_init(uint32_t);

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: pmm_alloc                                                    |
| description:   this function is only to be used AFTER pmm_init.             |
|                will find an available block of a given order. if we only    |
|                have blocks at order > requested order, then we can split    |
|                down these larger blocks.                                    |
|                                                                             |
|                to split a block, find the blocks of the next order. mark    |
|                right block (buddy) to available. repeat until we reach the  |
|                correct order (do NOT free the orders buddy).                |
|                                                                             |
\* ------------------------------------------------------------------------- */
uint64_t pmm_alloc(uint64_t);

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: mb_tag                                                         |
| description: mirrors the exact layout for a multiboot2 tag. these tags will |
|              tell us the type of multiboot flag we're looking at (0 =       |
|              terminator [stop], 6 = usable memory [we like this], anything  |
|              else is currently unused).                                     |
| layout:                                                                     |
|   bits 00-31: type of tag                                                   |
|   bits 32-63: size of the given tag (used to skip to next tag)              |
\* ------------------------------------------------------------------------- */
struct mb_tag {
    uint32_t type;
    uint32_t size;
};

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: mmap_entry                                                     |
| description: once we have found the usable memory multiboot2 tag, we know   |
|              the memory within this tag contains memory map entries which   |
|              will tell us which physical addresses in memory are usable.    |
| layout:                                                                     |
|   bits 00-31:  base address of the usable memory                            |
|   bits 32-63:  length of this usable memory in bytes                        |
|   bits 64-95:  1 if memory is usable                                        |
|   bits 96-127: reserved..? not to be used                                   |
\* ------------------------------------------------------------------------- */
struct mmap_entry {
    uint64_t base_addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: free_block                                                     |
| description: this represents a node within a linked list. each order of     |
|              block will have a linked list pointing to all free blocks (for |
|              quick access).                                                 |
| layout:                                                                     |
|   bits 00-31:  base address of the usable memory                            |
|   bits 32-63:  length of this usable memory in bytes                        |
|   bits 64-95:  1 if memory is usable                                        |
|   bits 96-127: reserved..? not to be used                                   |
\* ------------------------------------------------------------------------- */
struct free_block {
    struct free_block *next;
    struct free_block *prev;
};

/* ------------------------------------------------------------------------- *\
|                                                                             |
| static name: free_list                                                      |
| description: a linked list for each order of block size (so.. an array of   |
|              heads to linked lists).                                        |
|                                                                             |
\* ------------------------------------------------------------------------- */
static struct free_block *free_list[MAX_ORDER + 1];

/* ------------------------------------------------------------------------- *\
|                                                                             |
| static name: order_bm                                                       |
| description: array of pointers to bytes. each array of bytes is a bitmap    |
|              telling us what block in the given order is free.              |
|                                                                             |
\* ------------------------------------------------------------------------- */
static uint8_t *order_bm[MAX_ORDER + 1];

/* ------------------------------------------------------------------------- *\
|                                                                             |
| static name: order_blk_cnt                                                  |
| description: array of number of blocks needed for each order in our buddy   |
|              allocator. we need this to determine the number of bytes to    |
|              allocate to each array in order_bm.                            |
|                                                                             |
\* ------------------------------------------------------------------------- */
static uint64_t order_blk_cnt[MAX_ORDER + 1];

#endif