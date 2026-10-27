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
    console_draw_glpyh(0, 2, font8x16['A'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(16, 2, font8x16['B'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(32, 2, font8x16['C'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(48, 2, font8x16['D'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(64, 2, font8x16['E'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(80, 2, font8x16['F'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(96, 2, font8x16['G'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(112, 2, font8x16['H'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(128, 2, font8x16['I'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(144, 2, font8x16['J'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(160, 2, font8x16['K'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(176, 2, font8x16['L'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(192, 2, font8x16['M'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(208, 2, font8x16['N'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(224, 2, font8x16['O'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(240, 2, font8x16['P'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(256, 2, font8x16['Q'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(272, 2, font8x16['R'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(288, 2, font8x16['S'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(304, 2, font8x16['T'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(320, 2, font8x16['U'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(336, 2, font8x16['V'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(352, 2, font8x16['W'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(368, 2, font8x16['X'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(384, 2, font8x16['Y'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(400, 2, font8x16['Z'], FB_COLOR_WHITE, FB_COLOR_BLACK);



    for (;;) asm volatile("hlt");
}