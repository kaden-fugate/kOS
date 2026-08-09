#ifndef DRIVERS_H
#define DRIVERS_H

#define COM1 0x3F8

void serial_init(void);
void serial_print(const char *);
void serial_printf(const char *, void *[]);

#endif