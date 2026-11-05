#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdarg.h>
#include "font.h"

void console_init(uint32_t);
void console_putc(const char);
void console_write(const char*, uint64_t);
void console_print(const char*);
void console_printf(const char*, ...);

#endif