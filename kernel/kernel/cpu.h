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

static inline uint64_t read_cr3(void) {
    uint64_t value;

    asm volatile(
        "mov %%cr3, %0"
        : "=r"(value)
        :
        : "memory"
    );

    return value;
}

static inline void write_cr3(uint64_t phys_addr) {
    asm volatile(
        "mov %0, %%cr3"
        :
        : "r"(phys_addr)
        : "memory"
    );
}

#endif