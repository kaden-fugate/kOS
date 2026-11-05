#ifndef PIT_H
#define PIT_H

#include "drivers/def.h"

#define PIT_BASE_FREQ 1193182

#define PIT_CMD_PT 0x43
#define PIT_DAT_PT 0x40

void pit_init(uint32_t);

#endif