#ifndef CPU_H
#define CPU_H

#include <stdint.h>

static inline uint64_t irq_save() {
    uint64_t flgs;
    asm volatile("pushfq; pop %0; cli" : "=r"(flgs) : : "memory");
    return flgs;
}

static inline void irq_restore(uint64_t flgs) {
    asm volatile("push %0; popfq" : : "r"(flgs) : "memory", "cc");
}

#endif