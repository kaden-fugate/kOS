#include "drivers/pic.h"

void pic_remap() {

    // ICW1:
    // send initialization signal to the command port of each PIC
    // 0x11 = 0001 0001
    //  bit 0: ICW4 will be sent
    //  bit 4: start initialization at ICW1
    outb(M_CMD_PT, 0x11);
    outb(S_CMD_PT, 0x11);

    // ICW2:
    // set the vector offset
    // 0x20 = master PIC IRQs start at vector 0x20
    // 0x28 = slave PIC IRQs start at vector 0x28
    outb(M_DAT_PT, 0x20);
    outb(S_DAT_PT, 0x28);

    // ICW3:
    // tell PICs how theyre chained
    // 0x04 = bit 2 set, slave PIC is connected to line 2
    // 0x02 = value 2, connected to masters line 2
    outb(M_DAT_PT, 0x04);
    outb(S_DAT_PT, 0x02);

    // ICW4:
    // set operating mode
    // 0x01 = 8086 mode
    outb(M_DAT_PT, 0x01);
    outb(S_DAT_PT, 0x01);
}

void pic_set_mask(uint16_t mask) {
    outb(M_DAT_PT, (uint8_t)(mask & 0xFF));
    outb(S_DAT_PT, (uint8_t)(mask >> 8));
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) 
        outb(S_CMD_PT, PIC_EOI);

    outb(M_CMD_PT, PIC_EOI);
}