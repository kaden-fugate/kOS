#ifndef KTEST_H
#define KTEST_H

#include <stdint.h>

void ktest_pre_mt(void (*p)(uint32_t, uint32_t));
void pre_emption_test();
void ktest_post_mt();

#endif