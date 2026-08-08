#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAX_ORDER 10

extern uint8_t kernel_end;                                                     // defined in linker.ld

struct mb_tag {
    uint32_t type;
    uint32_t size;
};

struct mmap_entry {
    uint64_t base_addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

struct free_block {
    struct free_block *next;
};

static struct free_block *free_list[MAX_ORDER + 1];

static uint8_t *order_bm[MAX_ORDER + 1];
static uint64_t order_blk_cnt[MAX_ORDER + 1];

void add_buddy(uint64_t, int);
void seed_region(uint64_t, uint64_t);
void pmm_init(uint32_t);

#endif