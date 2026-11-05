#ifndef PIC_H
#define PIC_H

#include "drivers/def.h"

#define M_CMD_PT 0x20 // master command port
#define M_DAT_PT 0x21 // master data port

#define S_CMD_PT 0xA0 // slave command port
#define S_DAT_PT 0xA1 // master data port

#define PIC_EOI  0x20

void pic_remap();
void pic_set_mask(uint16_t);
void pic_send_eoi(uint8_t);

#endif