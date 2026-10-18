#include "tss_test.h"
#include "kernel/task.h"
#include "kernel/tss.h"
#include "drivers/serial.h"

void tss_test(struct task *current) {
    uint16_t tr16;
    asm volatile("str %0" : "=r"(tr16));

    uint64_t tr       = tr16;
    uint64_t expected = current->kstack_top;
    uint64_t actual   = tss_get_rsp0();

    serial_printf("[tss_test] TR=%x expected_rsp0=%x actual_rsp0=%x\n", 
        (void*[]){&tr, &expected, &actual});

    if (tr != 0x28 || actual != expected) {
        serial_print("[tss_test] FAIL\n");
        for (;;) asm volatile("cli; hlt");
    }
    serial_print("[tss_test] PASS\n");
}