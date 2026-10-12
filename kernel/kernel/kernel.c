#include <stdint.h>

#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/pit.h"

#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/task.h"

#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/heap.h"

#include "tests/ktest.h"

void kernel_main(uint32_t info_ptr, uint32_t magic) {
    serial_init();
    gdt_init();
    idt_init();
    pmm_init(info_ptr);
    vmm_init();
    heap_init();
    ktest();

    asm volatile("cli");
    pic_remap();
    pic_set_mask(0xFFFF);
    pit_init(100);
    pic_set_mask(0xFFFE);
    
    task_init();
    asm volatile("sti");

    pre_emption_test();

    for (;;) asm volatile("hlt");
}