#include <stdint.h>

#include "drivers/console.h"
#include "drivers/framebuffer.h"
#include "drivers/serial.h"
#include "drivers/pic.h"
#include "drivers/pit.h"

#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/task.h"
#include "kernel/tss.h"

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
    framebuffer_init(info_ptr);
    console_init(FB_COLOR_BLACK);
    char c = '!';
    int num_d = -67;
    uint64_t num_u = 1234567890;
    uint64_t num_x = 0xDEADBEEF;
    int num_o = 64;
    console_printf("%s is a test msg%c %d %u %x %o", "This", c, num_d, num_u, num_x, num_o);

    ktest_pre_mt(kernel_main);

    asm volatile("cli");
    pic_remap();
    pic_set_mask(0xFFFF);
    pit_init(100);
    pic_set_mask(0xFFFE);
    
    task_init();
    tss_init(boot_task.kstack_top);
    asm volatile("sti");

    pre_emption_test();
    asm volatile("int $0x80");
    serial_print("[syscall_test] retruned to ring 0\n");

    ktest_post_mt();

    for (;;) asm volatile("hlt");
}