#include "mm/pmm.h"
#include "drivers/serial.h"

static uint64_t total_blocks;

void pmm_init(uint32_t info_addr) {

    uint8_t *ptr     = (uint8_t *) (uint64_t) info_addr;                       // from boot.asm, addr of stack_top
    uint8_t *tag_ptr = ptr + 8;                                                // skip past size + magic
    uint32_t size    = *(uint32_t *) ptr;                                      // multiboot header defines size of tag region
    uint64_t u_addr  = 0;                                                      // highest address (upper addr)

    {
        void *args[] = {&ptr, &tag_ptr};
        serial_print("[pmm_init]:\n");
        serial_printf("\tptr:\t %x\n\ttag_ptr: %x\n", args);
    }

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
                    uint64_t rgn_end = ent->base_addr + ent->len;
                    if (rgn_end > u_addr) u_addr = rgn_end;
                    {
                        void *args[] = {&(ent->base_addr), &(rgn_end)};
                        serial_printf("\tRAM:\t (%x\t-\t%x)\n", args);
                    }
                }
                ent_ptr += ent_sz;
            }

        }
        tag_ptr += (tag->size + 7) & ~ 7;
    }

    // 1. get total size of memory
    // 2. get size of memory in # of pages
    //
    // for each order we have in our buddy allocator:
    //      1. find number of blocks required for the given order
    //      2. find number of bytes required for the given order (1 block per
    //         bit, 8 bits per byte)
    //
    total_blocks     = (u_addr / PAGE_SIZE);
    uint8_t *dat_ptr = (uint8_t *) &kernel_end;

    {
        void *args[] = {&total_blocks, &u_addr, &dat_ptr};
        serial_printf("\ttot:\t %u (%x)\n\tk_end:\t %x\n", args);
        return;
    }

    for (int ord = 0; ord <= MAX_ORDER; ++ord) {
        // get the blocks needed for the order (smallest to greatest)
        order_blk_cnt[ord] = total_blocks >> ord;

        // get num of bytes required at that order
        uint64_t num_b = (order_blk_cnt[ord] + 7) / 8;
        
        for (uint64_t byte = 0; byte < num_b; ++byte)
            order_bm[ord][byte] = (uint8_t) 0xFF;
    
        free_list[ord] = 0x0;
    }

    uint8_t *dat_end = dat_ptr;

}