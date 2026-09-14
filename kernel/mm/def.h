#ifndef DEF_H
#define DEF_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_ORDER 10

#define PHYS_TO_VIRT(phys) ((uint64_t *)(uint64_t)(phys))

#define NULL ((void*)0x0)
#define null NULL

#endif