#include "mm/pmm.h"
#include "drivers/serial.h"

static uint64_t total_blocks;

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: buddy_alloc                                                  |
| description:   when seed_region finds a block size at a given order and a   |
|                given starting page, we need to actually set these pages to  |
|                free.                                                        |
|                                                                             |
|                to do this we need to add the physical address to the page to|
|                our 'free_list' AND we need to mark the corresponding bit in |
|                the given order's bitmap as a 0 so that we'll know it's free |
|                in the future.                                               |
| tips:                                                                       |
|                1. (block_idx / 8) is the blocks byte in the orders bitmap.  |
|                2. (block_idx % 8) is the blocks bit in the bitmaps byte     |
|                                                                             |
\* ------------------------------------------------------------------------- */
void buddy_alloc(uint64_t start, uint8_t order) {
    struct free_block *free = (struct free_block *) (start * PAGE_SIZE);
    free->next = free_list[order];
    free_list[order] = free;

    uint64_t block_idx = start >> order;
    order_bm[order][block_idx / 8] &= ~(1ULL << (block_idx % 8));
}

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: seed_region                                                  |
| description:   given a starting and ending page number, we need to find how |
|                we can most efficiently break this range into blocks of      |
|                PAGE_SIZE * 2 ^ (order).                                     |
|                                                                             |
|                start at order = 10, decrease order by 1 until we get a block|
|                size that aligns with the start page and doesnt overflow     |
|                past ending page number.                                     |
|                                                                             |
\* ------------------------------------------------------------------------- */
void seed_region(uint64_t start, uint64_t end) {
    uint64_t order;
    uint64_t block_sz;
    uint64_t frame = start;
    uint8_t  aligned = 0;
    uint8_t  fits    = 0;

    while (frame < end) {

        order = MAX_ORDER;

        while (order > 0) {

            // does block_sz align with start does it fit inside the range?
            block_sz = 1ULL << order;
            aligned = (frame % block_sz == 0);
            fits    = (frame + block_sz <= end);

            if (aligned && fits) break;
            --order;
        }
        block_sz = 1ULL << order;
        
        buddy_alloc(frame, order);
        frame += block_sz;
    }
}

/* ------------------------------------------------------------------------- *\
|                                                                             |
| function name: check_sum                                                    |
| description:   debug and check if the amount of memory that we allocated    |
|                reports to the amount of memory we counted while iterating   |
|                over each entry in the usable RAM.                           |
|                                                                             |
|                ALSO, we want to compare the counted free + rounding loss to |
|                do an accurate check. we expect 4KiB lost due to the fact    |
|                that we skip the block starting at physical memory address   |
|                0x0.                                                         |
|                                                                             |
\* ------------------------------------------------------------------------- */
void check_sum(uint64_t reported_free) {
    uint64_t free_bytes = 0;
    uint64_t ord_byte   = 0;
    uint64_t cnt        = 0;

    struct free_block *cur = 0x0;

    for (uint64_t ord = 0; ord <= MAX_ORDER; ++ord) {
        ord_byte = 0;
        cnt      = 0;
        cur      = free_list[ord];

        while (cur) {
            ++cnt;
            cur = cur->next;
        }

        ord_byte = cnt * (1ULL << ord) * PAGE_SIZE;
        free_bytes += ord_byte;
        {
            void *args[] = {&ord, &cnt, &ord_byte};
            serial_printf("[check_sum] ord: %u: %u blocks (%u bytes)\n", args);
        }
    }

    {
        uint64_t diff = reported_free - free_bytes;
        void *args[] = {&free_bytes, &reported_free, &diff};
        serial_printf(
            "\n[check_sum] counted free: %u\n[check_sum] reported free: %u\n[check_sum] diff: %u\n\n", 
            args
        );
    }
}

void pmm_init(uint32_t info_addr) {

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | section ID:    SECTION 1.0                                                  |
    | description:   first off, we need to actually find WHERE the multiboot2 tag |
    |                with type = 6 is located in memory. from there we can        |
    |                go on and actually find the range of our usable memory.      |
    |                                                                             |
    |                info_addr comes from long_mode_start in boot.asm. we pass it |
    |                as a variable to kernel_main so that we know where the       |
    |                multiboot2 flags are.                                        |
    | process:                                                                    |
    |             1. init ptr to the first multiboot2 tag (tag_ptr)               |
    |             2. get size of the multiboot2 tag (stored as a 32 bit located at|
    |                info_addr in physical mem).                                  |
    |             3. iterate over each tag:                                       |
    |                  a. if tag's type is 0, break. this is the terminator tag   |
    |                  b. if tag's type is 6, store the mmap tag into mmap_tag,   |
    |                     break. we have found our memory map tag.                | 
    |                                                                             |
    \* ------------------------------------------------------------------------- */

    /* --------------------------- SECTION 1.0 BEGIN ----------------------------*/
    uint8_t *mb_ptr     = (uint8_t *) (uint64_t) info_addr;                    
    uint8_t *tag_ptr = mb_ptr + 8;                                             // [SECTION 1.0 -> 1] skip past size + magic
    uint32_t mb_size  = *(uint32_t *) mb_ptr;                                  // [SECTION 1.0 -> 2] multiboot header defines size of tag region

    struct mb_tag *mmap_tag = 0x0;
    
    while (tag_ptr < mb_ptr + mb_size) {                                       // [SECTION 1.0 -> 3] iterate over each tag
        struct mb_tag *tag = (struct mb_tag *) tag_ptr;

        if (!tag->type) break;                                                 // [SECTION 1.0 -> 3.a] terminator tag, stop

        if (tag->type == 6) {
            mmap_tag = tag;                                                    
            break;                                                             // [SECTION 1.0 -> 3.b] found our mmap tag, stop
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
    /* ---------------------------- SECTION 1.0 END -----------------------------*/

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | section ID:    SECTION 2.0                                                  |
    | description:   now that we have the memory map tag, we should iterate over  |
    |                it to find the highest physical address that we have access  |
    |                to. this will be needed to determine the number of bytes we  |
    |                will need for each orders bitmap (completed in section 2.1). |
    | process:                                                                    |
    |             1. in memory map tag, we have entries defining each physical    |
    |                memory range that we can use (if entry type is 1). make a ptr|
    |                to the first entry.                                          |
    |             2. get the size of a single entry so we can jump to the next    |
    |                one.                                                         |
    |             3. for each entry:                                              |
    |                  a. if entry type is not 1, do nothing                      |
    |                  b. otherwise, get the end of the entries described region, |
    |                     if higher than current mmap_end, update mmap_end.       |
    |             4. increment to the next entry.                                 |
    |                                                                             |
    \* ------------------------------------------------------------------------- */

    /* --------------------------- SECTION 2.0 BEGIN ----------------------------*/

    uint8_t *ent_start =  (uint8_t  *) mmap_tag + 16;                          // [SECTION 2.0 -> 1] first entry in mmap
    uint8_t *ent_end   =  (uint8_t  *) mmap_tag + mmap_tag->size;              
    uint32_t ent_sz    = *(uint32_t *) ((uint8_t *)mmap_tag + 8);              // [SECTION 2.0 -> 2] size of an entry
    uint64_t       mmap_end = 0x0;

    uint8_t *ent_ptr = ent_start;
    while (ent_ptr < ent_end) {
        struct mmap_entry *ent = (struct mmap_entry *) ent_ptr;
        if (ent->type == 1) {                                                  // [SECTION 2.0 -> 3.a] entry is marked as usable RAM
            uint64_t rgn_end = ent->base_addr + ent->len;
            if (rgn_end > mmap_end) mmap_end = rgn_end;
        }
        ent_ptr += ent_sz;                                                     // [SECITON 2.0 -> 4] jump to next entry
    }
    /* ---------------------------- SECTION 2.0 END -----------------------------*/

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | section ID:    SECTION 2.1                                                  |
    | description:   now that we have the highest address of physical memory, we  |
    |                can initialize free_list and order_bm. free_list for each    |
    |                order should be set to null, the order_bm for each order     |
    |                needs to know how many blocks of order n can be fit into the |
    |                given bitmap. each bit in the map corresponds to a single    |
    |                block of order n.                                            |
    | process:                                                                    |
    |             1. the total number of pages of free memory (our smallest memory|
    |                unit) is highest address (mmap_end) / PAGE_SIZE (4096).      |
    |             2. well if we're going to allocate bytes to each orders bitmap, |
    |                where will we store these bytes? great question! we will     |
    |                store them at kernel_end! just make sure to mark this memory |
    |                as reserved so we don't stomp all over it! :)                |
    |             3. for each order:                                              |
    |                  a. find the number of blocks that can fit into this order  |
    |                  b. allocate the number of bytes worth of blocks to order_bm|
    |                     (8 blocks per byte, round to num bytes needed)          |
    |                  c. mark the bytes of the bit map as fully reserved for now.|
    |                  d. set the head of the free list at this order to null.    |
    |            4. mark the end of where we stored order_bm.                     |
    |                                                                             |
    \* ------------------------------------------------------------------------- */

    /* --------------------------- SECTION 2.1 BEGIN ----------------------------*/
    total_blocks     = ((uint64_t) mmap_end / PAGE_SIZE);                      // [SECTION 2.1 -> 1] total number of pages we can fit into mmap
    uint8_t *dat_ptr = (uint8_t *) &kernel_end;                                // [SECTION 2.1 -> 2] this is where we'll store order_bm :)

    for (uint64_t ord = 0; ord <= MAX_ORDER; ++ord) {                          // [SECTION 2.1 -> 3] for each order (asc)

        order_blk_cnt[ord] = total_blocks >> ord;                              // [SECTION 2.1 -> 3.a] number of blocks that fit this order
        uint64_t num_b = (order_blk_cnt[ord] + 7) / 8;                         // [SECTION 2.1 -> 3.b] this is the number of bytes required

        order_bm[ord] = dat_ptr;                                               // [SECTION 2.1 -> 3.b] actually allocate the mem for bitmap
        dat_ptr      += num_b;
        
        for (uint64_t byte = 0; byte < num_b; ++byte)
            order_bm[ord][byte] = (uint8_t) 0xFF;                              // [SECTION 2.1 -> 3.c] set all blocks as reserved
    
        free_list[ord] = 0x0;                                                  // [SECTION 2.1 -> 3.d] free_list for order points to null

    }

    uint8_t *dat_end = dat_ptr;                                                // [SECTION2.1 -> 4] end of order_bm
    /* ---------------------------- SECTION 2.0 END -----------------------------*/

    /* ------------------------------------------------------------------------- *\
    |                                                                             |
    | section ID:    SECTION 3.0                                                  |
    | description:   finally, now that we have the bitmap and free list           |
    |                initialized, we can mark the proper pages as allocated.      |
    |                we'll also track the amount of memory seeded vs. the total   |
    |                memory available to us in bytes. some blocks will not be     |
    |                aligned with the pages so we expect to see some memory loss. |
    |                do check sum at the end to verify pmm is working.            |
    | process:                                                                    |
    |             1. return entry pointer to the beginning of the list of entries.|
    |             2. for each entry:                                              |
    |                a. if entry type is 1:                                       |
    |                   i.   get the beginning and end of the available memory    |
    |                        in units = # of pages in mem.                        |
    |                   ii.  IF we find that the block starts at physical memory  |
    |                        address 0, we need to increment it by 1 page (0x0    |
    |                        null. it's going to make things confusing).          |
    |                   iii. keep track of the rounding loss and the amount of    |
    |                        free bytes we've counted for the check sum later on. |
    |                   iv.  seed the region of pages, will find the most         |
    |                        efficient split for what blocks our pages can fit    |
    |                        into.                                                |
    |             3. check that the counted free vs. allocated match up (excluding|
    |                rounding loss, we expect 4KiB to be left unallocated). but we|
    |                expect that loss to not be reported as the page is not in the|
    |                free list nor was it counted towards free_bytes.             |
    |                                                                             |
    \* ------------------------------------------------------------------------- */

    /* --------------------------- SECTION 3.0 BEGIN ----------------------------*/
    uint64_t      free_bytes = 0x0;

    uint64_t reserved_start = (uint64_t) 0x100000 / PAGE_SIZE;
    uint64_t reserved_end   = ((uint64_t) dat_end + PAGE_SIZE - 1) / PAGE_SIZE;

    {
        uint64_t reserve_sz = reserved_end * PAGE_SIZE - reserved_start * PAGE_SIZE;
        void *args[] = {&reserve_sz};
        serial_printf("[pmm_init]:\tkernel reserve size: %u\n", args);
    }

    ent_ptr = ent_start;                                                       // [SECTION 3.0 -> 1]
    while (ent_ptr < ent_end) {                                                // [SECTION 3.0 -> 2] each entry...
        struct mmap_entry *ent = (struct mmap_entry *) ent_ptr;                 
        if (ent->type == 1) {                                                  // [SECTION 3.0 -> 3.a] usable RAM
            
            uint64_t rgn_start = ent->base_addr;
            uint64_t rgn_end   = ent->base_addr + ent->len;

            uint64_t frame_start = (rgn_start + PAGE_SIZE - 1) / PAGE_SIZE;    // [SECTION 3.0 -> 3.a.i] first page in memory region
            uint64_t frame_end   = rgn_end / PAGE_SIZE;

            if (!frame_start) ++frame_start;                                   // [SECTION 3.0 -> 3.a.ii] if that first page is at 0x0, we skip it
            
            uint64_t left_end = frame_end < reserved_start 
                                ? frame_end : reserved_start;
            if (frame_start < left_end) {
                seed_region(frame_start, left_end);
                // free_bytes += (left_end - frame_start) * PAGE_SIZE;
                //diff        = (left_end - frame_start) * PAGE_SIZE;
            }

            uint64_t right_start = frame_start > reserved_end 
                                   ? frame_start : reserved_end;
            if (right_start < frame_end){
                seed_region(right_start, frame_end);
                // free_bytes += (frame_end - right_start) * PAGE_SIZE;
                //diff       += (frame_end - right_start) * PAGE_SIZE;
            }
            
            free_bytes += rgn_end - rgn_start;
        }
        ent_ptr += ent_sz;
    }

    check_sum(free_bytes);                                                     // [SECTION 3.0 -> 4] check that the pmm is allocating bytes as expected
    /* ---------------------------- SECTION 3.0 END -----------------------------*/

}

uint64_t pmm_alloc(uint64_t order) {

    uint64_t found_order = order;
    while (found_order <= MAX_ORDER && free_list[found_order] == 0x0) 
        ++found_order;

    if (found_order > MAX_ORDER) {
        serial_print("[pmm_alloc]:\tFATAL ERROR. NO MEMORY.\n");
        return 0x0;
    }

    // remove from free_list
    struct free_block *addr = free_list[found_order];
    free_list[found_order] = addr->next;

    // {
    //     uint64_t add = (uint64_t) addr;
    //     void *args[] = {&found_order, &add};
    //     serial_printf("[pmm_alloc]:\tFOUND AT ORDER %u (%x)\n", args);
    // }

    // mark bit as occupied
    uint64_t block_idx = ((uint64_t) addr / PAGE_SIZE) >> found_order;
    order_bm[found_order][block_idx / 8] |= (uint8_t)(1ULL << (block_idx % 8));

    // allocate new blocks for each order >= order
    while (found_order > order) {
        --found_order;

        // set buddy to free
        uint64_t buddy = ((uint64_t) addr / PAGE_SIZE) + (1ULL << found_order);
        buddy_alloc(buddy, found_order);

        // mark block to alloc from as used
        block_idx = ((uint64_t) addr / PAGE_SIZE) >> found_order;
        order_bm[found_order][block_idx / 8] |= (1ULL << (block_idx % 8));
    }

    return (uint64_t) addr;
}