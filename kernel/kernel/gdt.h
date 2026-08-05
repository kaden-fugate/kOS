#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: gdt_int                                                      |
| description:   init and load GTD (replace what GRUB set up... it put us in  |
|                32 bit). we should call this first before anything relies on |
|                our segment selectors.                                       |
|                                                                             |
\* ------------------------------------------------------------------------- */
void gdt_init(void);

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: gdt_entry                                                      |
| description: mirrors the exact layout that the CPU expects when reading the |
|              GDT from mem. __attribute__((packed)) is just here to make sure|
|              that the compiler doesn't place padding bytes between our      |
|              fields.                                                        |
| layout:                                                                     |
|   bits 00-15: segment limit (or bits 0-16 of it, at least)                  |
|   bits 16-31: segment base address                                          |
|   bits 32-39: segment base address (cont. ...bits 24-31 of it)              |
|   bits 40-47: present bit, priv ring, desc type, xrw flags for segment      |
|   bits 48-55: flags (bits 17-19 of seg limit, long mode bit, etc.)          |
|   bits 56-63: segment base address (cont. ...bits 32-39)                    |
|                                                                             |
\* ------------------------------------------------------------------------- */
struct gdt_entry {
    uint16_t limit_low;    
    uint16_t base_low;     
    uint8_t  base_middle;  
    uint8_t  access;       
    uint8_t  granularity;  
    uint8_t  base_high;    
} __attribute__((packed));

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: gdt_ptr                                                        |
| description: this is a small structure that lgdt uses to find where the     |
|              gdt is and how large it is.                                    |
| layout:                                                                     |
|   bits 00-15: gdt size - 1 (bytes)                                          |
|   bits 16-79: base address of the first entry in gdt                        |
|                                                                             |
\* ------------------------------------------------------------------------- */
struct gdt_ptr {
    uint16_t limit;  // size of the GDT in bytes, minus 1 (CPU convention)
    uint64_t base;   // linear address of the first entry in the GDT
} __attribute__((packed));

/* ------------------------------------------------------------------------- *\
|                                                                             |
| macro name:  gdt_entries                                                    |
| description: our actual GDT will have 5 entries:                            |
|   1. null, 2. kernel code, 3. kernel data, 4. user code, 5. user data       |
|                                                                             |
\* ------------------------------------------------------------------------- */
#define GDT_ENTRIES 5

#endif