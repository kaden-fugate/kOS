#ifndef MM_DEF_H
#define MM_DEF_H

#include <stdint.h>

#define PAGE_SIZE 4096ULL
#define MAX_ORDER 10

#define KERNEL_VMA    0xFFFFFFFF80000000ULL
#define PHYS_MAP_BASE 0xFFFF800000000000ULL

#define PHYS_TO_VIRT(phys) \
    ((void *)((uint64_t)(phys) + PHYS_MAP_BASE))

#define DIRECT_VIRT_TO_PHYS(virt) \
    ((uint64_t)(virt) - PHYS_MAP_BASE)

#define KERNEL_VIRT_TO_PHYS(virt) \
    ((uint64_t)(virt) - KERNEL_VMA)

#define NULL ((void*)0x0)

#endif