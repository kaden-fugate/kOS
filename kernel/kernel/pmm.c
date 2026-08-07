#include "kernel/pmm.h"
#include "drivers/serial.h"

void pmm_init(uint32_t info_addr) {

    uint8_t *ptr  = (uint8_t *) (uint64_t) info_addr;                          // from boot.asm, addr of stack_top
    uint32_t size = *(uint32_t *) ptr;                                         // multiboot header defines size of tag region
    uint8_t *tag_ptr = ptr + 8;                                                // skip past size + magic

    while (tag_ptr < ptr + size) {
        struct mb_tag *tag = (struct mb_tag *) tag_ptr;

        if (!tag->type) break;                                                 // terminator tag, stop

        if (tag->type == 6) {
            uint32_t ent_sz = *(uint32_t *)(tag_ptr + 8);
            uint8_t *ent_ptr = tag_ptr + 16;                                   // 4 eight byte values to skip past
            uint8_t *ent_end = tag_ptr + tag->size;

            while (ent_ptr < ent_end) {
                struct mmap_entry *ent = (struct mmap_entry *) ent_ptr;
                if (ent->type == 1) {                                          // entry is marked as usable RAM
                    serial_print("Found usable RAM.\n");
                }
                ent_ptr += ent_sz;
            }

        }
        tag_ptr += (tag->size + 7) & ~ 7;
    }

}