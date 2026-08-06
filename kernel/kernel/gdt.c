#include "gdt.h"

/* ------------------------------------------------------------------------- *\
|                                                                             |
| macro name:  gdt_entries                                                    |
| description: our actual gdt will have 5 entries:                            |
|   1. null, 2. kernel code, 3. kernel data, 4. user code, 5. user data       |
|                                                                             |
\* ------------------------------------------------------------------------- */
#define GDT_ENTRIES 5

struct gdt_entry gdt[GDT_ENTRIES];                                             // actual gdt
struct gdt_ptr gdtp;                                                           // what we give to lgdt

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: gdt_set_entry                                                |
| description:   populate the gdt entry using the parameters.                 |
| paramters:                                                                  |
|   1. index:       gdt entry to populate                                     |
|   2. base:        base address          (see gdt_entry def for more info)   |
|   3. limit:       segment limit         (see gdt_entry def for more info)   |
|   4. access:      access bits           (see gdt_entry def for more info)   |
|   5. granularity: important bits        (see gdt_entry def for more info)   |
|                                                                             |
\* ------------------------------------------------------------------------- */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t granularity) {
    gdt[index].base_low    = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high   = (base >> 24) & 0xFF;

    gdt[index].limit_low   = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);

    gdt[index].access = access;
}

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: gdt_reload_segments                                          |
| description:   use a far return to move our new kernel code selector into   |
|                CS.                                                          |
|                                                                             |
\* ------------------------------------------------------------------------- */
static inline void gdt_reload_segments(void) {
    asm volatile(
        "pushq $0x08\n"                                                        // push our kernel code selector (0x08)
        "leaq 1f(%%rip), %%rax\n"                                              // load address of 1 into rax
        "pushq %%rax\n"                                                        // push rax as the return address from the far return
        "lretq\n"                                                              // far-return: pops CS = 0x08 and RIP = 1 (the label, not the num)
        "1:\n"                                                                 // exec resumes here + new CS
        "movw $0x10, %%ax\n"                                                   // mov kernel data selector to ax (0x010)
        "movw %%ax, %%ds\n"                                                    // reload data segment
        "movw %%ax, %%es\n"                                                    // reload extra segment
        "movw %%ax, %%fs\n"                                                    // reload fs
        "movw %%ax, %%gs\n"                                                    // reload gs
        "movw %%ax, %%ss\n"                                                    // reload stack segment
        :
        :
        : "rax", "memory"
    );
}

void gdt_init(void) {

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | gdt entry: null descriptor                                                  |
    |                                                                             |
    \* ------------------------------------------------------------------------- */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | gdt entry:      kernel code segment                                         |
    | layout:                                                                     |
    |   base address: 0x0                                                         |
    |   segment lim:  0xFFFFF                              (entire address space) |
    |   access:       0x9A (1001 1010):                                           |
    |                   present: 1, ring: 0, type: 1, exec: 1, run: 1, access: 0  |
    |   grnulrty:     0xAF (1010 1111):                                           |
    |                   grnulrty: 1, big: 0, L bit: 1, AVL: 0, limit: 1111        |
    |                                                                             |
    \* ------------------------------------------------------------------------- */    
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xAF);

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | gdt entry:      kernel data segment                                         |
    | layout:                                                                     |
    |   base address: 0x0                                                         |
    |   segment lim:  0xFFFFF                              (entire address space) |
    |   access:       0x92 (1001 0010):                                           |
    |                   present: 1, ring: 0, type: 1, exec: 0, read: 1, access: 0 |
    |   grnulrty:     0xCF (1100 1111):                                           |
    |                   grnulrty: 1, big: 1, L bit: 0, AVL: 0, limit: 1111        |
    |                                                                             |
    \* ------------------------------------------------------------------------- */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | gdt entry:      user code segment                                           |
    | layout:                                                                     |
    |   base address: 0x0                                                         |
    |   segment lim:  0xFFFFF                              (entire address space) |
    |   access:       0xFA (1111 1010):                                           |
    |                   present: 1, ring: 3, type: 1, exec: 1, read: 1, access: 0 |
    |   grnulrty:     0xAF (1010 1111):                                           |
    |                   grnulrty: 1, big: 0, L bit: 1, AVL: 0, limit: 1111        |
    |                                                                             |
    \* ------------------------------------------------------------------------- */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA, 0xAF);

    // Entry 4: user data segment (ring 3). Same idea: 0x92 | 0x60 = 0xF2.
    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | gdt entry:      user code segment                                           |
    | layout:                                                                     |
    |   base address: 0x0                                                         |
    |   segment lim:  0xFFFFF                              (entire address space) |
    |   access:       0xF2 (1111 1010):                                           |
    |                   present: 1, ring: 3, type: 1, exec: 0, read: 1, access: 0 |
    |   grnulrty:     0xCF (1100 1111):                                           |
    |                   grnulrty: 1, big: 1, L bit: 0, AVL: 0, limit: 1111        |
    |                                                                             |
    \* ------------------------------------------------------------------------- */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2, 0xCF);

    // populate gdt ptr to be loaded using lgdt
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint64_t)&gdt;

    // load new gdt into CPUs GDTR register. still need to update our CS/DS/etc
    // though.
    asm volatile("lgdt %0" : : "m"(gdtp));

    // reload segments now that we have gdt initialized and loaded
    gdt_reload_segments();
}