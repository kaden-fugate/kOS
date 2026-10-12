#ifndef TASK_H
#define TASK_H

#include "mm/heap.h"

#define TASK_STACK_SIZE 16384

extern struct task boot_task;
extern struct task *current;
extern struct task *task_b;

enum task_state { TASK_READY, TASK_RUNNING, TASK_DEAD };

struct task {
    uint64_t                 rsp;       // saved stack ptr
    uint8_t                 *stk_base;  // base of the stack
    struct task             *next;      // next task to run
    volatile enum task_state state;     // task state
};

struct task *task_create(void (*)(void));
void schedule();
void task_init();
void task_add(struct task*);
void task_exit();
void task_join(struct task*);
void task_reap(struct task*);

#endif