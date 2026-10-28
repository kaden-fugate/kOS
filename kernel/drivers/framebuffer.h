#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

#define FB_COLOR_BLACK       0x00000000
#define FB_COLOR_WHITE       0x00FFFFFF
#define FB_COLOR_RED         0x00FF0000
#define FB_COLOR_GREEN       0x0000FF00
#define FB_COLOR_BLUE        0x000000FF
#define FB_COLOR_YELLOW      0x00FFFF00
#define FB_COLOR_CYAN        0x0000FFFF
#define FB_COLOR_MAGENTA     0x00FF00FF
#define FB_COLOR_DARK_BLUE   0x00000020
#define FB_COLOR_GRAY        0x00808080

struct framebuffer {
    uint64_t          phys_addr;
    volatile uint8_t *virt_addr;
    uint32_t          pitch;
    uint32_t          width;
    uint32_t          height;
    uint8_t           bpp;
    uint8_t           type;
};

int framebuffer_init(uint64_t);
void fb_put_pix(uint32_t, uint32_t, uint32_t);

#endif