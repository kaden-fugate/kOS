#include "tests/heap_stress.h"

#include "drivers/serial.h"
#include "kernel/task.h"
#include "mm/pmm.h"

void stress_task() {
    asm volatile("sti");
    uint8_t id = (uint8_t)(uint64_t)current;     // differs per task
    serial_printf("[stress_task]: %x start.\n", (void*[]){&current});
    for (uint64_t k = 0; k < 0xFFFF; k++) {
        uint8_t *p[8];
        for (int i = 0; i < 8; i++) {
            p[i] = kmalloc(16 + i * 40);
            for (int j = 0; j < 16; j++) p[i][j] = id + i;
        }
        for (int i = 7; i >= 0; i--) {
            for (int j = 0; j < 16; j++)
                if (p[i][j] != (uint8_t)(id + i)) { 
                    serial_print("CORRUPT\n"); 
                    for (;;) asm volatile("hlt"); 
                }
            kfree(p[i]);
        }
    }
    serial_printf("[stress_task]: %x end.\n", (void*[]){&current});
}

void run_heap_test() {
    struct task *heap_test_a = task_create(stress_task);
    struct task *heap_test_b = task_create(stress_task);

    task_add(heap_test_a);
    task_add(heap_test_b);
}