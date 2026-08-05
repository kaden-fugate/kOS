#include <stdint.h>
#include "drivers/serial.h"
#include "kernel/gdt.h"

void kernel_main(void) {
    serial_init();
    serial_print("Hello, friend!\n");

    gdt_init();

    serial_print("If this runs, GDT didn't triple fault! :)\n");

    for (;;) {
        asm volatile("hlt");
    }
}