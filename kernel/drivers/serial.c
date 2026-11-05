#include "serial.h"
#include "kernel/cpu.h"

void serial_init() {
    outb(COM1 + 1, 0x00); // disable interrupts
    outb(COM1 + 3, 0x80); // enable DLAB
    outb(COM1 + 0, 0x03); // divisor low byte: 38400 baud
    outb(COM1 + 1, 0x00); // divisor high byte
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // enable FIFO
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

int serial_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(const char c) {
    while (!serial_transmit_empty());
    outb(COM1, c);
}

void serial_print(const char *s) {
    uint64_t f = irq_save();
    for (int i = 0; s[i]; i++) serial_putc(s[i]);
    irq_restore(f);
}

void serial_print_uint(uint64_t num, int base) {
    char s[256] = {0};
    const char chars[16] = "0123456789ABCDEF";
    uint16_t i = 0;

    while (num > 0) {
        s[i++] = chars[num % base];
        num   /= base;
    }

    while (i > 0) serial_putc(s[--i]);
}

void serial_print_int(int64_t num) {
    if (num < 0) {
        serial_putc('-');
        serial_print_uint((uint64_t)(-(num + 1)) + 1, 10);
        return;
    }

    serial_print_uint(num, 10);
}

static void serial_printf_locked(const char *frmt, va_list argv) {
    // while we havent reached a NULL terminator
    //  iterate through the format
    //  if we see %, look at the next character:
    //      if d, 32bit:   int
    //      if u, 32bit:   unsigned int
    //      if x, 32bit:   hex
    //      if l, 64bit:   int/u_int/hex
    //      if s, string:  (we have serial_print)
    //      if c, char:    (serial putc)
    //      anything else: just print the character

    for (int i = 0; frmt[i]; ++i) {
        if (frmt[i] != '%') { 
            serial_putc(frmt[i]); 
            continue;
        }
        frmt++;
        
        // we found a %
        switch (frmt[i]) {
            case 'c':
                serial_putc((const char)va_arg(argv, const int));
                break;
            case 's':
                serial_print(va_arg(argv, const char *));
                break;
            case 'd':
                serial_print_int(va_arg(argv, const int32_t));
                break;
            case 'u':
                serial_print_uint(va_arg(argv, const uint64_t), 10);
                break;
            case 'x':
                serial_print("0x");
                serial_print_uint(va_arg(argv, const uint64_t), 16);
                break;
            case 'o':
                serial_print_uint(va_arg(argv, const uint64_t), 8);
                break;
            default:
                serial_putc('%');
                serial_putc(frmt[i]);
        }
    }
}

void serial_printf(const char *frmt, ...) {
    uint64_t f = irq_save();

    va_list argv;
    va_start(argv, frmt);
    serial_printf_locked(frmt, argv);
    va_end(argv);

    irq_restore(f);
}