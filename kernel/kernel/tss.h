#ifndef TSS_H
#define TSS_H

#include <stdint.h>

struct tss {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} __attribute__((packed));

void tss_init(uint64_t);
void tss_set_rsp0(uint64_t);
uint64_t tss_get_rsp0();

#endif