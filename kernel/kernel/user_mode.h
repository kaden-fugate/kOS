#ifndef USER_MODE_H
#define USER_MODE_H

#include <stdint.h>

__attribute__((noreturn))
void enter_user(uint64_t, uint64_t);

#endif