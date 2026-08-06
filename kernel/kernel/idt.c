#include "kernel/idt.h"
#include "drivers/serial.h"

// 256 possible vectors, 32 cpu-defined exceptions.
#define IDT_ENTRIES 256
struct idt_entry idt[IDT_ENTRIES];  // the table itself
struct idt_ptr idtp;                 // what we hand to `lidt`

// declare the 32 labels defined in isr.asm.
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

static void idt_set_entry(int vector, void (*handler)(void), uint8_t type_attr) {
    uint64_t addr = (uint64_t)handler;

    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08;        // our kernel CODE segment (GDT index 1)
    idt[vector].ist         = 0;           // don't switch stacks (not set up yet)
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

void idt_init(void) {

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | ** FLAGS (1000 1110) **                                                     |
    | layout:                                                                     |
    |   bit 07 (1):       present                                                 |
    |   bit 06-05 (00):   DPL/ring, ring - only ring 0 can trigger this           |
    |   bit 04 (0):       zero for interrupt/traps                                |     
    |   bit 03-00 (1110): trap gate, leave interrupt flag alone                   |
    |                                                                             |
    \* ------------------------------------------------------------------------- */ 
    uint8_t flags = 0x8E;

    // 32 CPU designed exception vecs, defined in isr.asm
    idt_set_entry(0,  isr0,  flags);
    idt_set_entry(1,  isr1,  flags);
    idt_set_entry(2,  isr2,  flags);
    idt_set_entry(3,  isr3,  flags);
    idt_set_entry(4,  isr4,  flags);
    idt_set_entry(5,  isr5,  flags);
    idt_set_entry(6,  isr6,  flags);
    idt_set_entry(7,  isr7,  flags);
    idt_set_entry(8,  isr8,  flags);
    idt_set_entry(9,  isr9,  flags);
    idt_set_entry(10, isr10, flags);
    idt_set_entry(11, isr11, flags);
    idt_set_entry(12, isr12, flags);
    idt_set_entry(13, isr13, flags);
    idt_set_entry(14, isr14, flags);
    idt_set_entry(15, isr15, flags);
    idt_set_entry(16, isr16, flags);
    idt_set_entry(17, isr17, flags);
    idt_set_entry(18, isr18, flags);
    idt_set_entry(19, isr19, flags);
    idt_set_entry(20, isr20, flags);
    idt_set_entry(21, isr21, flags);
    idt_set_entry(22, isr22, flags);
    idt_set_entry(23, isr23, flags);
    idt_set_entry(24, isr24, flags);
    idt_set_entry(25, isr25, flags);
    idt_set_entry(26, isr26, flags);
    idt_set_entry(27, isr27, flags);
    idt_set_entry(28, isr28, flags);
    idt_set_entry(29, isr29, flags);
    idt_set_entry(30, isr30, flags);
    idt_set_entry(31, isr31, flags);

    // ** remaining 224 entries are zero'd out **

    // populate idt ptr to be loaded using lidt
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    // load idt, dont need to refresh the same way we do with gdt
    asm volatile("lidt %0" : : "m"(idtp));
}

// self explanatory... right?
static const char *exception_names[32] = {
    "Divide by zero", "Debug", "NMI", "Breakpoint", "Overflow", 
    "Bound range exceeded", "Invalid opcode", "Device not available", 
    "Double fault", "Coprocessor segment overrun", "Invalid TSS", 
    "Segment not present", "Stack-segment fault", "General protection fault", 
    "Page fault", "Reserved", "x87 floating point exception", 
    "Alignment check", "Machine check", "SIMD floating point exception",
    "Virtualization exception", "Control protection exception", "Reserved", 
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor injection exception", "VMM communication exception", 
    "Security exception", "Reserved"
};


void idt_common_handler(struct interrupt_frame *frame) {

    // 14: page fault
    if (frame->vector == 14) {
        // faulting addr is not on stack with a page fault, put it in cr2
        // instead
        uint64_t fault_addr;
        asm volatile("mov %%cr2, %0" : "=r"(fault_addr));

        serial_print("PAGE FAULT\n");
    } else {
        serial_print("EXCEPTION: ");
        serial_print(exception_names[frame->vector]);
        serial_print("\n");
    }

    // just halt forever on these exceptions
    for (;;) {
        asm volatile("hlt");
    }
}