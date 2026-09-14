#ifndef HEAP_H
#define HEAP_H

#include "mm/def.h"

struct heap_block {
    uint64_t           size;
    uint8_t            free;
    struct heap_block *next;
    struct heap_block *prev;
};

#define NUM_BINS 8
static const uint64_t bin_sizes[NUM_BINS] = {
    16, 32, 64, 128, 256, 512, 1024, 0
};
static struct heap_block *bins[NUM_BINS] = { null };

#define HEAP_VIRT_BASE 0xDEADBEEF

void heap_init();

#endif