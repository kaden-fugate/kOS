#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: interrupt_frame                                                |
| description: full register state that we can save and hand off to our C     |
|              handler as a ptr.                                              |
| layout:                                                                     |
|   r15 - r8                                                  (in that order) |
|   rbp, rdi, rsi, rdx, rcx, rbx, rax                         (in that order) |
|   vector                                                    (type of error) |
|   error code                                        (additional error info) |
|   rip, cs, rflags, rsp, ss                                  (in that order) |                
|                                                                             |
\* ------------------------------------------------------------------------- */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;

    uint64_t vector, error_code;

    uint64_t rip, cs, rflags, rsp, ss;
};

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: idt_init                                                     |
| description:   init and load 256 idt enteries, fill in first 32 cpu         |
|                exception vectors + load to cpu via lidt.                    |
|                                                                             |
\* ------------------------------------------------------------------------- */
void idt_init(void);

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: idt_common_handler                                           |
| description:   c entry point that every isr stub calls after saving its     |
|                registers                                                    |
|                                                                             |
\* ------------------------------------------------------------------------- */
void idt_common_handler(struct interrupt_frame *frame);

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: idt_entry                                                      |
| description: 16 byte IDT entry                                              |
| layout:                                                                     |
|   bits 00-15: handlers address                           (or... part of it) |
|   bits 16-31: GDT CS seg selector to run handler under                      |
|   bits 32-39: idx of IST to pull stack from                 (0 = cur stack) |
|   bits 40-47: gate type, 0, DPL, present bit          (in that order [asc]) |
|   bits 48-95: handlers address                        (final part of it...) |
|   bits 96-127: literally just zeros                                         |
|                                                                             |
\* ------------------------------------------------------------------------- */
struct idt_entry {
    uint16_t offset_low;   
    uint16_t selector;        
    uint8_t  ist;         
    uint8_t  type_attr;    
    uint16_t offset_mid;   
    uint32_t offset_high;  
    uint32_t zero;         
} __attribute__((packed));   

/* ------------------------------------------------------------------------- *\
|                                                                             |
| struct name: idt_ptr                                                        |
| description: this is a small structure that lidt uses to find where the     |
|              idt is and how large it is.                                    |
| layout:                                                                     |
|   bits 00-15: gdt size - 1 (bytes)                                          |
|   bits 16-79: base address of the first entry in gdt                        |
|                                                                             |
\* ------------------------------------------------------------------------- */
struct idt_ptr {
    uint16_t limit;  
    uint64_t base;   
} __attribute__((packed));

#endif