#include "tss.h"

#include "gdt.h"
#include "kutil/string.h"

static struct tss tss;

void tss_init(uint64_t kstack_top) {
    memset(&tss, 0, sizeof(tss));
    tss.rsp0        = kstack_top;
    tss.iopb_offset = sizeof(tss);

    gdt_set_tss((uint64_t)&tss, sizeof(tss) - 1);
    asm volatile("ltr %0" : : "r"((uint16_t)0x28));    
}

void tss_set_rsp0(uint64_t top) { 
    tss.rsp0 = top;
}

uint64_t tss_get_rsp0() { 
    return tss.rsp0;
}