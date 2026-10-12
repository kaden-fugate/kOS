#include "pmm_stress.h"

#include <stdint.h>
#include "drivers/serial.h"
#include "kernel/cpu.h"
#include "kernel/task.h"
#include "mm/pmm.h"

#define PS_ITERS   0xFFFFF
#define PS_HELD    12
#define PS_MAX_ORD 3
#define PS_FRAMES  (1ULL << 18)

static uint8_t claimed[PS_FRAMES / 8];
static volatile uint64_t ps_done;
static volatile uint64_t ps_failed;

struct held { uint64_t addr, order, stamp; };

// atomically claim every frame in a block. fails if frame has been claimed
static int claim(uint64_t addr, uint64_t order) {
    uint64_t first = addr / PAGE_SIZE, n = 1ULL << order;
    uint64_t f = irq_save();
    int ok = 1;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t fr = first + i;
        if (claimed[fr / 8] & (1 << (fr % 8))) ok = 0;
    }
    if (ok) {
        for (uint64_t i = 0; i < n; i++) {
            uint64_t fr = first + i;
            claimed[fr / 8] |= (1 << (fr % 8));
        }    
    }
    irq_restore(f);
    return ok;
}

// atomically unclaim every frame in a block
static void unclaim(uint64_t addr, uint64_t order) {
    uint64_t first = addr / PAGE_SIZE, n = 1ULL << order;
    uint64_t f = irq_save();
    for (uint64_t i = 0; i < n; i++) {
        uint64_t fr = first + i;
        claimed[fr / 8] &= ~(1 << (fr % 8));
    }
    irq_restore(f);
}

// stamp block with addr as a key
static void stamp_block(struct held *h) {
    for (uint64_t i = 0; i < (1ULL << h->order); i++) 
        *(volatile uint64_t *)(h->addr + i * PAGE_SIZE) = h->stamp;
}

// check blocks stamped key
static int stamp_ok(struct held *h) {
    for (uint64_t i = 0; i < (1ULL << h->order); i++) 
        if (*(volatile uint64_t *)(h->addr + i * PAGE_SIZE) != h->stamp)
            return 0;
    return 1;
}

// if we failed, log error, halt fo-eva
static void fail(const char *what, uint64_t id, uint64_t addr, uint64_t order) {
    ps_failed = 1;
    serial_printf("[pmm_stress %x]: %s addr=[%x] order=[%u]\n", 
        (void*[]){&id, (void*)what, &addr, &order});
    for (;;) asm volatile("hlt");
}

// if our block is good, lets unclaim and free it
static void release(struct held *h, uint64_t id) {
    if (!stamp_ok(h)) fail("STAMP OVERWRITTEN", id, h->addr, h->order);
    unclaim(h->addr, h->order);
    pmm_free(h->addr, h->order);
    h->addr = 0;
}

static void pmm_stress(uint64_t id) {
    struct held h[PS_HELD];
    for (int i = 0; i < PS_HELD; i++) h[i].addr = 0;
    uint64_t seed = id * 0x9E3779B97F4A7C15ULL; // magic number #1

    for (uint64_t k = 0; k < PS_ITERS; k++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; // magic number #2 and #3
        struct held *s = &h[(seed >> 33) % PS_HELD];

        // stamp present, release it
        if (s->addr) release(s, id);

        // otherwise, stamp the block. if its already claimed, this is a double alloc
        else {
            uint64_t order = (seed >> 45) % (PS_MAX_ORD + 1);
            uint64_t addr = pmm_alloc(order);
            if (!addr) continue;
            if (!claim(addr, order)) fail("DOUBLE ALLOCATION", id, addr, order);
            s->addr = addr;
            s->order = order;
            s->stamp = (id << 56) ^ addr;
            stamp_block(s);
        }

    }

    // release all blocks
    for (int i = 0; i < PS_HELD; i++) if (h[i].addr) release(&h[i], id);
    ps_done++;

}

void pmm_stress_a() { asm volatile("sti"); pmm_stress(0x10); }
void pmm_stress_b() { asm volatile("sti"); pmm_stress(0x80); }

void run_pmm_test() {
    struct task *pmm_test_a = task_create(pmm_stress_a);
    struct task *pmm_test_b = task_create(pmm_stress_b);

    uint64_t before = pmm_free_bytes();

    task_add(pmm_test_a);
    task_add(pmm_test_b);

    while (ps_done < 2 && !ps_failed) asm volatile("hlt");

    uint64_t after = pmm_free_bytes();
    uint64_t errs = pmm_check();
    serial_printf("[run_pmm_test]: free before=[%u] after=[%u] errors=%u\n", 
        (void*[]){&before, &after, &errs});
    serial_print(before == after && !errs && !ps_failed 
        ? "[run_pmm_test]: PASS\n"
        : "[run_pmm_test]: FAIL\n"
    );
}