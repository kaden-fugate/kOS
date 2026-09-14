#ifndef VMM_H
#define VMM_H

#include "mm/def.h"

// virt addr -> page table level macros
#define PML4_IDX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_IDX(addr) (((addr) >> 30) & 0x1FF)
#define PD_IDX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_IDX(addr)   (((addr) >> 12) & 0x1FF)

// page table entry macros
#define PTE_PRESENT   (1ULL << 0)
#define PTE_WRITEABLE (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_PS        (1ULL << 7)
#define PTE_NX        (1ULL << 63)
#define PTE_ADDR_MASK (0x000FFFFFFFFFF000ULL)

void     vmm_init();
void     vmm_map(uint64_t*, uint64_t, uint64_t, uint64_t);
void     vmm_unmap(uint64_t*, uint64_t);
uint64_t vmm_alloc(uint64_t*, uint64_t, uint8_t, uint64_t);
void     vmm_test();

extern uint64_t *k_pml4;

#endif

/*
63  62..........................52  51.......................12  11....9 8  7  6  5  4  3  2  1  0
+---+----------------------------+-----------------------------+---------+--+--+--+--+--+--+--+--+
| NX|      (unused/reserved)     |   Physical Address (bits    |   AVL   |PS|D |A |CD|WT|U |W |P |
|   |                            |    12-51 of the target)     |         |  |  |  |  |  |  |  |  |
+---+----------------------------+-----------------------------+---------+--+--+--+--+--+--+--+--+
*/