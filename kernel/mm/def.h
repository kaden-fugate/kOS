#ifndef MM_DEF_H
#define MM_DEF_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_ORDER 10

// #define KERNEL_VMA 0xFFFFFFFF80000000ULL
#define KERNEL_VMA 0x0
#define PHYS_TO_VIRT(phys) ((uint64_t *)((uint64_t)(phys) + KERNEL_VMA))
#define VIRT_TO_PHYS(virt) ((uint64_t)(virt) - KERNEL_VMA)

#define NULL ((void*)0x0)

#endif