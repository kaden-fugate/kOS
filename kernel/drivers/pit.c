#include "drivers/pit.h"

void pit_init(uint32_t freq) {

    if (!freq) return;

    uint32_t divisor = PIT_BASE_FREQ / freq;

    if (!divisor) divisor = 1;
    else if (divisor > 0xFFFF) divisor = 0xFFFF;

    // 0x36 = 00 11 011 0
    //  0: BCD mode
    //  011: square-wave generator
    //  11: access mode, send low byte then high byte
    //  00: channel = 0
    outb(PIT_CMD_PT, 0x36);

    // PIT is 8 bits wide, feed lower bits first then upper
    outb(PIT_DAT_PT, divisor & 0xFF);
    outb(PIT_DAT_PT, (divisor >> 8) & 0xFF);

}