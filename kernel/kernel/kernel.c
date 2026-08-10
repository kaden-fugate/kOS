#include <stdint.h>
#include "drivers/serial.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"

#include "mm/pmm.h"

void kernel_main(uint32_t info_ptr, uint32_t magic) {
    serial_init();
    serial_print("Hello, friend!\n");

    gdt_init();
    idt_init();
    
    // test idt is working (keep uncommented if you dont want things to break)
    // int x = 1 / 0;
    // serial_print("If you see this, IDT didn't work! :(\n");

    pmm_init(info_ptr);

    uint64_t total_alloc = 0x0;
    uint64_t free        = pmm_alloc(0);
    while (free != 0x0) {
        total_alloc += PAGE_SIZE;
        free         = pmm_alloc(0);
    }

    {
        void *args[] = {&total_alloc};
        serial_printf("ALL MEM ALLOC'D: %u\n", args);
    }

    for (;;) {
        asm volatile("hlt");
    }
}