#ifndef MM_DEF_H
#define MM_DEF_H

#include <stdint.h>
#include "kernel/layout.h"

#define PAGE_SIZE 4096ULL
#define MAX_ORDER 10

#define PHYS_TO_VIRT(phys) \
    ((void *)((uint64_t)(phys) + PHYS_MAP_BASE))

#define DIRECT_VIRT_TO_PHYS(virt) \
    ((uint64_t)(virt) - PHYS_MAP_BASE)

#define KERNEL_VIRT_TO_PHYS(virt) \
    ((uint64_t)(virt) - KERNEL_VMA)

#define NULL ((void*)0x0)

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

#endif