#include "console.h"
#include "framebuffer.h"
#include "serial.h"
#include "kernel/cpu.h"

static uint8_t  scale = 3;
static uint32_t glyph_height;
static uint32_t glyph_width;

static uint32_t cursor_col;
static uint32_t cursor_row;

static uint32_t console_width;
static uint32_t console_height;

static uint32_t foreground;
static uint32_t background;

static void console_draw_glpyh(
    uint32_t px,
    uint32_t py,
    const uint8_t glyph[G_HEIGHT],
    uint32_t foreground,
    uint32_t background
) {
    for (uint32_t y = 0; y < G_HEIGHT; y++) {
        uint8_t r_bits = glyph[y];
        for (uint32_t x = 0; x < G_WIDTH; x++) {
            uint32_t color = (r_bits & (128LL >> x)) ? foreground : background;

            for (uint8_t sy = 0; sy < scale; sy++) 
                for (uint8_t sx = 0; sx < scale; sx++) 
                    fb_put_pix(px + x * scale + sx, py + y * scale + sy, color);

        }
    }
}

static void console_scroll() {
    const uint32_t pixel_rows = G_HEIGHT * scale;
    const uint64_t move_bytes = (uint64_t)(fb_height() - pixel_rows) * fb_pitch();

    volatile uint8_t *dst = fb_base();
    volatile uint8_t *src = fb_base() + (uint64_t)pixel_rows * fb_pitch();

    for (uint32_t i = 0; i < move_bytes; i++)
        dst[i] = src[i];
    
    for (uint32_t y = fb_height() - pixel_rows; y < fb_height(); y++) {
        for (uint32_t x = 0; x < fb_width(); x++) {
            fb_put_pix(x, y, background);
        }
    }
}

static void inc_cursor_row() {
    cursor_row = cursor_row + glyph_height;
    cursor_col = 0;


}

static void inc_cursor_col() {
    if (cursor_col + glyph_width >= console_width ||     // cursor off screen
        cursor_col + glyph_width * 2 >= console_width) { // glyph off screen
        inc_cursor_row();
        return;
    }

    cursor_col += glyph_width;
}

void console_init(uint32_t bg) {
    glyph_height = G_HEIGHT * scale;
    glyph_width  = G_WIDTH  * scale;

    cursor_col     = 0;
    cursor_row     = 0;

    console_width  = fb_width();
    console_height = fb_height();

    foreground = FB_COLOR_WHITE;
    background = bg;

    fb_clear(bg);
}

static void console_putc(const char c) {
    uint64_t row = cursor_row;
    uint64_t col = cursor_col;
    uint64_t height = console_height;
    uint64_t width = console_width;

    if (c == '\n') {
        inc_cursor_row();
        return;
    }
    else if (cursor_row >= console_height || 
        cursor_row + glyph_height >= console_height) {
        console_scroll();
        cursor_row -= glyph_height;
    }
    
    console_draw_glpyh(cursor_col, cursor_row, font8x16[(uint8_t)c], foreground, 
        background);
    
    inc_cursor_col();
}

void console_print(const char *s) {
    if (!s) return;
    uint64_t f = irq_save();

    for (int i = 0; s[i] != '\0'; i++) console_putc(s[i]);

    irq_restore(f);
}

static void print_uint(uint64_t num, uint8_t base) {
    char s[256] = {0};
    const char chars[16] = "0123456789ABCDEF";
    uint16_t i = 0;

    while (num > 0) {
        s[i++] = chars[num % base];
        num   /= base;
    }

    while (i > 0) console_putc(s[--i]);
}

static void print_int(int64_t num) {
    if (num < 0) {
        console_putc('-');
        print_uint((uint64_t)(-(num + 1)) + 1, 10);
        return;
    }

    print_uint((uint64_t)num, 10);
}

static void console_printf_locked(const char *frmt, va_list args) {
    if (!frmt) return;

    for (int i = 0; frmt[i] != '\0'; i++) {
        if (frmt[i] != '%') {
            console_putc(frmt[i]);
            continue;
        }
        frmt++;

        switch (frmt[i]) {
            case 'c':
                console_putc((char)va_arg(args, int));
                break;
            case 's':
                const char *s = va_arg(args, const char *);
                console_print(s);
                break;
            case 'd':
                const int32_t d = va_arg(args, const int32_t);
                print_int((int64_t)d);
                break;
            case 'u':
                const uint64_t u = va_arg(args, const uint64_t);
                print_uint(u, 10);
                break;
            case 'x':
                console_print("0x");
                const uint64_t x = va_arg(args, const uint64_t);
                print_uint(x, 16);
                break;
            case 'o':
                const uint64_t o = va_arg(args, const uint64_t);
                print_uint(o, 8);
                break;
            default:
                console_putc('%');
                console_putc(frmt[i]);
        }

    }

    va_end(args);
}

void console_printf(const char *frmt, ...) {
    uint64_t f = irq_save();

    va_list args;
    va_start(args, frmt);
    console_printf_locked(frmt, args);
    va_end(args);

    irq_restore(f);
}

void console_write(const char *frmt, uint16_t n) {
    uint64_t f = irq_save();

    for (uint16_t i = 0; i < n; i++) console_putc(frmt[i]);

    irq_restore(f);
}