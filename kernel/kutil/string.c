#include "kutil/string.h"

void memset(void *dest, int value, size_t size) {
    uint8_t *ptr = (uint8_t *) dest;
    uint8_t  val = (uint8_t)   value;

    for (size_t i = 0; i < size; i++)
        ptr[i] = val;
}

void memcpy(void *dest, const void *src, size_t size) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;

    for (size_t i = 0; i < size; i++) d[i] = s[i];
}