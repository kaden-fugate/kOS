#include "console.h"
#include "framebuffer.h"

static const uint8_t scale = 2;

void console_draw_glpyh(
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