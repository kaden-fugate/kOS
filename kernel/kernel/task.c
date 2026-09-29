#include "kernel/task.h"
#include "drivers/serial.h"

struct task boot_task;
struct task *current = &boot_task;
struct task *task_b;

struct task *task_create(void (*entry)(void)) {
    struct task *t = kmalloc(sizeof(struct task));
    t->stk_base    = kmalloc(TASK_STACK_SIZE);

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

    t->rsp = (uint64_t)sp;
    return t;
}

void task_exit() {
    serial_print("[task] exited\n");
    for (;;) asm volatile("hlt");   // need to do further cleanup later on
}

void schedule() {
    struct task *prev = current;
    struct task *next = prev->next;
    if (next == prev) return;

    current = next;
    switch_context(&prev->rsp, next->rsp);
}

void task_b_main() {
    asm volatile("sti");
    for (;;) {
        serial_print("B\n");
        asm volatile("hlt");
    }
}