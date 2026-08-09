#include "mm/pmm.h"
#include "drivers/serial.h"

static uint64_t total_blocks;

void buddy_alloc(uint64_t start, uint8_t order) {
    struct free_block *free = (struct free_block *) (start * PAGE_SIZE);
    free->next = free_list[order];
    free_list[order] = free;

    uint64_t block_idx = start >> order;
    order_bm[order][block_idx / 8] &= ~(1ULL << (block_idx % 8));              // (block_idx / 8): blocks byte in bm, (block_idx % 8): blocks bit in bm's byte
}

void seed_region(uint64_t start, uint64_t end) {
    uint64_t order;
    uint64_t block_sz;
    uint64_t frame = start;
    uint8_t  aligned = 0;
    uint8_t  fits    = 0;
    
    {
        void *args[] = {&start, &end};
        serial_printf("[seed_region] range: %u - %u\n", args);
    }

    while (frame < end) {

        order = MAX_ORDER;

        while (order > 0) {

            // does block_sz align with start does it fit inside the range?
            block_sz = 1ULL << order;
            aligned = (frame % block_sz == 0);
            fits    = (frame + block_sz <= end);

            {
                void *args[] = {&order, &block_sz};
                serial_printf("[seed_region] ord (%u):\t%u\n", args);
            }

            if (aligned && fits) break;
            --order;
        }
        block_sz = 1ULL << order;
        
        {
            void *args[] = {&frame, &block_sz};
            serial_printf("\tblk:\t %u (%u)\n", args);
        }
        
        buddy_alloc(frame, order);
        frame += block_sz;
    }
}

// find 6 tag first
// find req bytes for each order
// set all sections as unavail
// seed the region:
//      start at highest order size
//      if order aligned and fits in section of mem (else shrink order):
//          - update start frame
//          - add frame to free list at that order
//          - switch this frames bit to avail in orders bitmap
void pmm_init(uint32_t info_addr) {

    uint8_t *mb_ptr     = (uint8_t *) (uint64_t) info_addr;                    // from boot.asm, addr of stack_top
    uint8_t *tag_ptr = mb_ptr + 8;                                             // skip past size + magic
    uint32_t mb_size  = *(uint32_t *) mb_ptr;                                  // multiboot header defines size of tag region

    struct mb_tag *mmap_tag = 0x0;
    uint64_t       mmap_end = 0x0;

    {
        void *args[] = {&mb_ptr, &tag_ptr};
        serial_print("[pmm_init]:\n");
        serial_printf("\tptr:\t %x\n\ttag_ptr: %x\n", args);
    }

    // find multiboots memory map tag (type 6, only one)
    while (tag_ptr < mb_ptr + mb_size) {
        struct mb_tag *tag = (struct mb_tag *) tag_ptr;

        if (!tag->type) break;                                                 // terminator tag, stop

        if (tag->type == 6) {
            mmap_tag = tag;                                                    // 4 eight byte values to skip past
            {
                void *args[] = {&mmap_tag};
                serial_printf("\tmmap_tag:%x\n", args);
            }

            break;                                                             // found our mmap tag, stop
        }
        tag_ptr += (tag->size + 7) & ~ 7;
    }

    // no mmap tag = wtf happened? abort
    if (!mmap_tag) {
        serial_print(
            "FATAL ERROR:\tNo memory map found in multiboot header.\n"
        );
        return;
    }

    // get the highest address in our memory map
    uint32_t ent_sz    = *(uint32_t *) ((uint8_t *)mmap_tag + 8);
    uint8_t *ent_start =  (uint8_t  *) mmap_tag + 16;
    uint8_t *ent_end   =  (uint8_t  *) mmap_tag + mmap_tag->size;

    {
        void *args[] = {&ent_start, &ent_end};
        serial_printf("\tentries: %x - %x\n", args);
    }

    uint8_t *ent_ptr = ent_start;
    while (ent_ptr < ent_end) {
        struct mmap_entry *ent = (struct mmap_entry *) ent_ptr;
        if (ent->type == 1) {                                       // entry is marked as usable RAM
            uint64_t rgn_end = ent->base_addr + ent->len;
            if (rgn_end > mmap_end) mmap_end = rgn_end;
            {
                void *args[] = {&(ent->base_addr), &(rgn_end)};
                serial_printf("\tRAM:\t (%x\t-\t%x)\n", args);
            }
        }
        ent_ptr += ent_sz;
    }

    // 1. get total size of memory
    // 2. get size of memory in # of pages
    //
    // for each order we have in our buddy allocator:
    //      1. find number of blocks required for the given order
    //      2. find number of bytes required for the given order (1 block per
    //         bit, 8 bits per byte)
    //
    total_blocks     = ((uint64_t) mmap_end / PAGE_SIZE);
    uint8_t *dat_ptr = (uint8_t *) &kernel_end;

    {
        void *args[] = {&total_blocks, &mmap_end, &dat_ptr};
        serial_printf("\ttot:\t %u (%x)\n\tk_end:\t %x\n", args);
    }

    for (uint64_t ord = 0; ord <= MAX_ORDER; ++ord) {

        {
            void *args[] = {&ord};
            serial_printf("\torder (%d):\n", args);
        }

        // get the blocks needed for the order (smallest to greatest)
        order_blk_cnt[ord] = total_blocks >> ord;

        // get num of bytes required at that order
        uint64_t num_b = (order_blk_cnt[ord] + 7) / 8;

        order_bm[ord] = dat_ptr;
        dat_ptr      += num_b;
        
        for (uint64_t byte = 0; byte < num_b; ++byte)
            order_bm[ord][byte] = (uint8_t) 0xFF;
    
        free_list[ord] = 0x0;

        {
            void *args[] = {&order_blk_cnt[ord], &num_b};
            serial_printf("\t\tblk_cnt: %u\n\t\tnum_b:\t %u\n", args);
        }
    }

    uint8_t *dat_end = dat_ptr;

    ent_ptr = ent_start;
    {
        void *args[] = {&ent_ptr, &ent_end};
        serial_printf("\tent_ptr: %x\n\tent_end: %x\n", args);
    }
    while (ent_ptr < ent_end) {
        struct mmap_entry *ent = (struct mmap_entry *) ent_ptr;
        {
            void *args[] = {&ent, &ent->type};
            serial_printf("\tent: %x (%d)\n", args);
        }
        if (ent->type == 1) {                                       // entry is marked as usable RAM
            uint64_t rgn_start = ent->base_addr;
            uint64_t rgn_end   = ent->base_addr + ent->len;

            uint64_t frame_start = (rgn_start + PAGE_SIZE - 1) / PAGE_SIZE;
            uint64_t frame_end   = rgn_end / PAGE_SIZE;

            {
                void *args[] = {&frame_start, &frame_end};
                serial_printf("\tfrm_st:\t %x\n\tfrm_end: %x\n", args);
            }

            if (frame_start < frame_end)
                seed_region(frame_start, frame_end);
        }
        ent_ptr += ent_sz;
    }

}