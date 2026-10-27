#include "framebuffer.h"
#include "mm/vmm.h"
#include "drivers/serial.h"

struct mb2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct mb2_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

struct mb2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
} __attribute__((packed));

#define MB2_TAG_END         0
#define MB2_TAG_FRAMEBUFFER 8
#define MB2_FRAMEBUFFER_RGB 1

static struct framebuffer fb;

static uint64_t align_up(uint64_t val, uint64_t n) {
    return (val + (n - 1)) & ~(n - 1);
}

static const struct framebuffer *framebuffer_get() {
    return &fb;
}

static const uint32_t framebuffer_width() {
    return fb.width;
}

static const uint32_t framebuffer_height() {
    return fb.height;
}

static int framebuffer_discover(uint64_t mb_info_phys) {
    uint8_t *info = (uint8_t *)PHYS_TO_VIRT(mb_info_phys);
    struct mb2_info *header = (struct mb2_info *)info;
    uint8_t *end = info + header->total_size;
    struct mb2_tag *tag = (struct mb2_tag *)(info + sizeof(struct mb2_info));

    fb.phys_addr = 0;
    fb.pitch     = 0;
    fb.width     = 0;
    fb.height    = 0;
    fb.bpp       = 0;
    fb.type      = 0;

    while ((uint8_t *)tag < end && tag->type != MB2_TAG_END) {
        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            struct mb2_tag_framebuffer *fb_tag = 
                (struct mb2_tag_framebuffer *)tag;

            fb.phys_addr = fb_tag->framebuffer_addr;
            fb.pitch     = fb_tag->framebuffer_pitch;
            fb.width     = fb_tag->framebuffer_width;
            fb.height    = fb_tag->framebuffer_height;
            fb.bpp       = fb_tag->framebuffer_bpp;
            fb.type      = fb_tag->framebuffer_type;

            serial_printf("[fb] multiboot framebuffer tag found [%x]\n", 
                (void*[]){&fb.phys_addr});

            if (fb.type != MB2_FRAMEBUFFER_RGB) {
                serial_printf("[fb] unsupported framebuffer type %x\n", 
                    (void*[]){&fb.type});
                return -1;        
            }
            else if (fb.bpp != 32) {
                serial_printf("[fb] unsupported framebuffer type %x\n", 
                    (void*[]){&fb.bpp});
                return -1;
            }

            return 0;
        }

        tag = (struct mb2_tag *)align_up((uint64_t)tag + tag->size, 8ULL);
    }
    serial_print("[fb] no multiboot framebuffer tag\n");
    return -1;
}

// only called AFTER framebuffer_discover
static int framebuffer_map() {
    uint64_t phys_page = fb.phys_addr & ~(PAGE_SIZE - 1);
    uint64_t offset    = fb.phys_addr &  (PAGE_SIZE - 1);
    uint64_t n_bytes   = offset + (uint64_t)fb.pitch * fb.height; // pitch = bytes/row
    uint64_t map_size  = align_up(n_bytes, PAGE_SIZE);

    for (uint64_t i = 0; i < map_size; i += PAGE_SIZE)
        vmm_map(k_pml4, FRAMEBUFF_VIRT + i, phys_page + i, 
            PTE_PRESENT | PTE_WRITEABLE | PTE_NX);
    
    fb.virt_addr = (volatile uint8_t *)(FRAMEBUFF_VIRT + offset);

    return 0;
}

int framebuffer_init(uint64_t mb_info_phys) {
    if (framebuffer_discover(mb_info_phys)) return -1;
    return framebuffer_map();
}

void fb_put_pix(uint32_t x, uint32_t y, uint32_t rgb) {
    if (x >= fb.width || y >= fb.height) return;

    // 32 bpp means each pixel has 4 bytes
    volatile uint32_t *pixel = (volatile uint32_t *) (
        fb.virt_addr + (uint64_t)y * fb.pitch + (uint64_t)x * 4
    );
    *pixel = rgb;
}