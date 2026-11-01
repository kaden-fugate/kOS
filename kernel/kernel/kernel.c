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

    console_draw_glpyh(0* 8 * 3, 2, font8x16['a'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(1* 8 * 3, 2, font8x16['b'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(2* 8 * 3, 2, font8x16['c'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(3* 8 * 3, 2, font8x16['d'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(4* 8 * 3, 2, font8x16['e'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(5* 8 * 3, 2, font8x16['f'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(6* 8 * 3, 2, font8x16['g'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(7* 8 * 3, 2, font8x16['h'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(8* 8 * 3, 2, font8x16['i'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(9* 8 * 3, 2, font8x16['j'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(10* 8 * 3, 2, font8x16['k'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(11* 8 * 3, 2, font8x16['l'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(12* 8 * 3, 2, font8x16['m'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(13* 8 * 3, 2, font8x16['n'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(14* 8 * 3, 2, font8x16['o'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(15* 8 * 3, 2, font8x16['p'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(16* 8 * 3, 2, font8x16['q'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(17* 8 * 3, 2, font8x16['r'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(18* 8 * 3, 2, font8x16['s'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(19* 8 * 3, 2, font8x16['t'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(20* 8 * 3, 2, font8x16['u'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(21* 8 * 3, 2, font8x16['v'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(22* 8 * 3, 2, font8x16['w'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(23* 8 * 3, 2, font8x16['x'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(24* 8 * 3, 2, font8x16['y'], FB_COLOR_WHITE, FB_COLOR_BLACK);
    console_draw_glpyh(25* 8 * 3, 2, font8x16['z'], FB_COLOR_WHITE, FB_COLOR_BLACK);

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