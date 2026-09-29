#ifndef TASK_H
#define TASK_H

#include "mm/heap.h"

#define TASK_STACK_SIZE 16384

extern struct task boot_task;
extern struct task *current;
extern struct task *task_b;

struct task {
    uint64_t rsp;       // saved stack ptr
    uint8_t *stk_base;  // base of the stack
    struct task *next;  // next task to run
};

struct task *task_create(void (*)(void));
void switch_context(uint64_t*, uint64_t);
void task_exit();
void schedule();
void task_b_main();

#endif