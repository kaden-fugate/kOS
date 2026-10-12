#ifndef KTEST_H
#define KTEST_H

#include <stdint.h>

void ktest(void (*p)(uint32_t, uint32_t));
void pre_emption_test();

#endif