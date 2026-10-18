#include "task.h"
#include "cpu.h"
#include "tss.h"
#include "drivers/serial.h"
#include "mm/heap.h"

struct task boot_task;
struct task *current;
struct task *task_b;

extern void switch_context(uint64_t*, uint64_t);
extern uint8_t kernel_stack_top[];

struct task *task_create(void (*entry)(void)) {
    struct task *t = kmalloc(sizeof(struct task));
    if (!t) return NULL;

    t->stk_base    = kmalloc(TASK_STACK_SIZE);
    if (!t->stk_base) { kfree(t); return NULL; }

    uint64_t top  = (uint64_t)(t->stk_base + TASK_STACK_SIZE) & ~0xFULL;
    t->kstack_top = top;
    uint64_t *sp  = (uint64_t*)top;

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
    if (!next || next == prev) return;

    current = next;
    tss_set_rsp0(next->kstack_top);
    switch_context(&prev->rsp, next->rsp);
}

void task_init() {
    boot_task.next     = &boot_task;
    boot_task.state    = TASK_RUNNING;
    boot_task.kstack_top = (uint64_t)kernel_stack_top;
    boot_task.stk_base = NULL;
    boot_task.rsp      = 0;

    current = &boot_task;
    serial_printf("boot kstack_top=%x\n", (void*[]){&boot_task.kstack_top});
}

void task_add(struct task *t) {
    uint64_t f = irq_save();
    t->next = current->next;
    current->next = t;
    irq_restore(f);
}

static struct task *task_remove(struct task *t) {
    struct task *prev = t;
    while (prev->next != t) prev = prev->next;

    struct task *next = t->next;
    prev->next = next;
    t->next = NULL;

    return next;
}

__attribute__((noreturn))
void task_exit() {
    irq_save();

    struct task *dead = current;
    struct task *next = task_remove(dead);

    dead->state = TASK_DEAD;
    current = next;

    tss_set_rsp0(next->kstack_top);
    switch_context(&dead->rsp, next->rsp);

    for (;;) asm volatile("cli; hlt");
}

void task_join(struct task *t) {
    while(t->state != TASK_DEAD) asm volatile("hlt");
}

void task_reap(struct task *t) {
    kfree(t->stk_base);
    kfree(t);
}