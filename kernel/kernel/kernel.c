#include <stdint.h>

#include "drivers/serial.h"

#include "kernel/gdt.h"
#include "kernel/idt.h"

#include "mm/pmm.h"
#include "mm/vmm.h"

void kernel_main(uint32_t info_ptr, uint32_t magic) {
    serial_init();
    serial_print("Hello, friend!\n");

    gdt_init();
    idt_init();
    
    // test idt is working (keep uncommented if you dont want things to break)
    // int x = 1 / 0;
    // serial_print("If you see this, IDT didn't work! :(\n");

    pmm_init(info_ptr);
    pmm_test();

    vmm_init();
    vmm_test();

    for (;;) {
        asm volatile("hlt");
    }
}