#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "kernel/idt.h"

#define SYSCALL_VECTOR 0x80

enum syscall_num {
    SYS_PING  = 0,
    SYS_WRITE = 1,
    SYS_EXIT  = 2,
    SYS_YIELD = 3
};

uint64_t syscall_dispatch(struct interrupt_frame*);

#endif