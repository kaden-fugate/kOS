#ifndef PMM_H
#define PMM_H

#include <stdint.h>

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

void pmm_init(uint32_t);

#endif