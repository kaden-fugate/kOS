#include <stdint.h>
#include "serial.h"

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void serial_init(void) {
    outb(COM1 + 1, 0x00); // disable interrupts
    outb(COM1 + 3, 0x80); // enable DLAB
    outb(COM1 + 0, 0x03); // divisor low byte: 38400 baud
    outb(COM1 + 1, 0x00); // divisor high byte
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // enable FIFO
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

int serial_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    while (!serial_transmit_empty());
    outb(COM1, c);
}

void serial_print(const char *s) {
    for (int i = 0; s[i]; i++) serial_putc(s[i]);
}

void serial_print_uint(uint64_t val, int base) {
    if (!val) {
        serial_putc('0');
        return;
    }

    char       buf[16] = "";
    const char chars[] = "0123456789ABCDEF";
    int i        = 0;

    while (val > 0) {
        buf[i++] = chars[val % base];
        val     /= base;
    }

    while (i > 0) serial_putc(buf[--i]);
}

void serial_print_int(int64_t val) {
    if (val < 0) {
        serial_putc('-');
        val = -val;
    }

    serial_print_uint(val, 10);
}

void serial_printf(const char *frmt, void *argv[]) {
    // while we havent reached a null terminator
    //  iterate through the format
    //  if we see %, look at the next character:
    //      if d, 32bit:   int
    //      if u, 32bit:   unsigned int
    //      if x, 32bit:   hex
    //      if l, 64bit:   int/u_int/hex
    //      if s, string:  (we have serial_print)
    //      if c, char:    (serial putc)
    //      anything else: just print the character

    int cur_arg = 0;
    int64_t  *i_val;
    uint64_t *u_val;
    char     *c_val;
    for (int i = 0; frmt[i]; ++i) {
        if (frmt[i] != '%') { 
            serial_putc(frmt[i]); 
            continue;
        }
        ++i;
        
        // we found a %
        switch (frmt[i]) {
            case 'd':
                i_val = (int64_t *)argv[cur_arg++];
                serial_print_int(*i_val);
                break;
            case 'u':
                u_val = (uint64_t *)argv[cur_arg++];
                serial_print_uint(*u_val, 10);
                break;
            case 'x':
                serial_print("0x");
                u_val = (uint64_t *)argv[cur_arg++];
                serial_print_uint(*u_val, 16);
                break;
            case 's':
                c_val = (char *)argv[cur_arg++];
                serial_print(c_val);
                break;
            default:
                serial_putc('%');
                serial_putc(frmt[i]);
                continue;
        }
    }
}