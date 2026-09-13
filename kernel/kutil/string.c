#include "kutil/string.h"

void memset(void *dest, int value, size_t size) {
    uint8_t *ptr = (uint8_t *) dest;
    uint8_t  val = (uint8_t)   value;

    for (size_t i = 0; i < size; i++)
        ptr[i] = val;
}