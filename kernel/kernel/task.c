#include "task.h"

#include "drivers/serial.h"
#include "mm/heap.h"
#include "cpu.h"

struct task boot_task;
struct task *current;
struct task *task_b;

extern void switch_context(uint64_t*, uint64_t);

struct task *task_create(void (*entry)(void)) {
    struct task *t = kmalloc(sizeof(struct task));
    if (!t) return NULL;

    t->stk_base    = kmalloc(TASK_STACK_SIZE);
    if (!t->stk_base) { kfree(t); return NULL; }

    uint64_t top = (uint64_t)(t->stk_base + TASK_STACK_SIZE) & ~0xFULL;
    uint64_t *sp = (uint64_t*)top;

    *(--sp) = (uint64_t)task_exit;  // function to execute on task completion
    *(--sp) = (uint64_t)entry;      // entry point of the new task
    *(--sp) = 0;                    // rbp
    *(--sp) = 0;                    // rbx
    *(--sp) = 0;                    // r12
    *(--sp) = 0;                    // r13
    *(--sp) = 0;                    // r14
    *(--sp) = 0;                    // r15

    t->rsp   = (uint64_t)sp;
    t->next  = NULL;
    t->state = TASK_READY;

    return t;
}

void schedule() {
    struct task *prev = current;
    struct task *next = prev->next;
    if (next == prev) return;

    current = next;
    switch_context(&prev->rsp, next->rsp);
}

void task_init() {
    boot_task.next     = &boot_task;
    boot_task.state    = TASK_RUNNING;
    boot_task.stk_base = NULL;
    boot_task.rsp      = 0;

    current = &boot_task;
}

void task_add(struct task *t) {
    uint64_t f = irq_save();
    t->next = current->next;
    current->next = t;
    irq_restore(f);
}

static void task_remove(struct task *t) {
    if (t == &boot_task) return;
    uint64_t f = irq_save();
    struct task *p = t;
    while (p->next != t) p = p->next;
    p->next = t->next;
    irq_restore(f);
}

void task_exit() {
    serial_print("[task] exited\n");
    irq_save();
    current->state = TASK_DEAD;
    task_remove(current);
    schedule();
}

void task_join(struct task *t) {
    while(t->state != TASK_DEAD) asm volatile("hlt");
}

void task_reap(struct task *t) {
    kfree(t->stk_base);
    kfree(t);
}