#include <stdint.h>
#include "drivers/serial.h"

void kernel_main(void) {
    serial_init();
    serial_print("Hello, friend!\n");

    volatile uint16_t *vga = (uint16_t*)0xB8000;
    const char *str = "Hello, 64-bit kernel!";
    uint8_t color = 0x0F;
    for (int i = 0; str[i] != '\0'; i++) {
        vga[i] = (uint16_t)str[i] | ((uint16_t)color << 8);
    }

    for (;;) {
        asm volatile("hlt");
    }
}