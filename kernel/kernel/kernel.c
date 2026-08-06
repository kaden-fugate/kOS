#include <stdint.h>
#include "drivers/serial.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"

void kernel_main(void) {
    serial_init();
    serial_print("Hello, friend!\n");

    gdt_init();

    serial_print("If this runs, GDT didn't triple fault! :)\n");

    idt_init();

    serial_print("If this runs, maybe IDT didn't triple fault! :)\n");

    // test idt is working
    int x = 1 / 0;
    serial_print("If you see this, IDT didn't work! :(\n");

    for (;;) {
        asm volatile("hlt");
    }
}