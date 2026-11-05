#ifndef VMM_H
#define VMM_H

#include "mm/def.h"

void     vmm_init();
void     vmm_map(uint64_t*, uint64_t, uint64_t, uint64_t);
void     vmm_unmap(uint64_t*, uint64_t);
uint64_t vmm_alloc(uint64_t*, uint64_t, uint8_t, uint64_t);
uint64_t vmm_translate(uint64_t);
void     vmm_test();

extern uint64_t *k_pml4;

#endif

/*
63  62..........................52  51.......................12  11....9 8  7  6  5  4  3  2  1  0
+---+----------------------------+-----------------------------+---------+--+--+--+--+--+--+--+--+
| NX|      (unused/reserved)     |   Physical Address (bits    |   AVL   |PS|D |A |CD|WT|U |W |P |
|   |                            |    12-51 of the target)     |         |  |  |  |  |  |  |  |  |
+---+----------------------------+-----------------------------+---------+--+--+--+--+--+--+--+--+
*/