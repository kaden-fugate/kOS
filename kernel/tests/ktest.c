#include "ktest.h"

#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/pit.h"

#include "kernel/task.h"

#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"

#include "heap_stress.h"

uint8_t stall() {
    while( !(current == &boot_task && current->next == &boot_task) ) 
        asm volatile("hlt");
}

void ktest() {
    serial_print("Hello, friend!\n");
    
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

    serial_print("[pre_emption_test]: done.\n");
}