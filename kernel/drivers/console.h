#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include "font.h"

void console_draw_glpyh(uint32_t, uint32_t, const uint8_t[G_HEIGHT], uint32_t, uint32_t); 
void console_init(uint32_t);
void console_putc(char);
void console_write(const char*, uint64_t);
void console_print(const char*);

#endif