#include "ktest.h"

#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/pit.h"

#include "kernel/task.h"

#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"

#include "heap_stress.h"
#include "pmm_stress.h"
#include "vmm_stress.h"

uint8_t stall() {
    while( !(current == &boot_task && current->next == &boot_task) ) 
        asm volatile("hlt");
}

void ktest(void (*p)(uint32_t, uint32_t)) {
    serial_print("Welcome to kOS.\n");

    uint64_t rip;
    asm volatile("lea 0(%%rip), %0" : "=r"(rip));

    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=r"(rsp));

    serial_printf("[higher-half] kernel_main= %x \n", (void*[]){&p});
    serial_printf("[higher-half] RIP= %x \n", (void*[]){&rip});
    serial_printf("[higher-half] RSP= %x \n", (void*[]){&rsp});
    
    // test idt is working (keep uncommented if you dont want things to break)
    // int x = 1 / 0;
    // serial_print("If you see this, IDT didn't work! :(\n");

    pmm_test();
    vmm_test();
    heap_test();
}

void pre_emption_test() {
    run_heap_test();
    stall();

    run_pmm_test();
    stall();

    run_vmm_test();
    stall();

    serial_print("[pre_emption_test]: done.\n");
}