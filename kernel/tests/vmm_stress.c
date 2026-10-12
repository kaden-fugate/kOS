#include "vmm_stress.h"

#include <stdint.h>
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "kernel/task.h"
#include "kernel/cpu.h"
#include "drivers/serial.h"

#define VT_ROUNDS 96
#define VT_PT_BASE   0xFFFFA10000000000ULL
#define VT_PD_BASE   0xFFFFA20000000000ULL
#define VT_PDPT_BASE 0xFFFFA40000000000ULL

static uint64_t vt_va[2][VT_ROUNDS];
static uint64_t vt_pa[2][VT_ROUNDS];
static volatile uint64_t vt_failed;
static volatile uint64_t vt_mapped;

static uint64_t vt_region(uint64_t r) {
    uint64_t idx = r / 3;
    switch (r % 3) {
        case 0:  return VT_PT_BASE + idx * 0x200000ULL;      // new PT
        case 1:  return VT_PD_BASE + idx * 0x40000000ULL;    // new PD + PT
        default: return VT_PDPT_BASE + idx * 0x8000000000ULL;  // new PDPT + PD + PT
    }
}

static void vt_fail(const char *what, uint64_t id, uint64_t va) {
    vt_failed = 1;
    serial_printf("[vmm_stress %x]: %s va=[%x]\n", (void*[]){&id, (void*)what, &va});
    for (;;) asm volatile("hlt");
}

static void vt_worker(uint64_t id) {
    for (uint64_t r = 0; r < VT_ROUNDS; r++) {
        uint64_t va    = vt_region(r) + id * PAGE_SIZE;
        uint64_t pa    = pmm_alloc(0);
        if (!pa) vt_fail("OUT OF MEMORY", id, va);
        
        uint64_t stamp = (id << 56) ^ pa ^ r;
        *(volatile uint64_t *)PHYS_TO_VIRT(pa) = stamp;

        vmm_map(k_pml4, va, pa, PTE_WRITEABLE);
        vt_va[id][r] = va;
        vt_pa[id][r] = pa;

        if (vmm_translate(va) != pa) vt_fail("MAPPING LOST (immediate)", id, va);
        if (*(volatile uint64_t *)va != stamp) vt_fail("WRONG DATA", id, va);
    }

    uint64_t f = irq_save();
    vt_mapped++;
    irq_restore(f);
    while(vt_mapped < 2 && !vt_failed) asm volatile("hlt");

    for (uint64_t r = 0; r < VT_ROUNDS; r++) {
        uint64_t va = vt_va[id][r];
        uint64_t pa = vt_pa[id][r];
        uint64_t stamp = (id << 56) ^ pa ^ r;
        if (vmm_translate(va) != pa) vt_fail("MAPPING LOST (audit)", id, va);
        if (*(volatile uint64_t *)va != stamp) vt_fail("WRONG DATA (audit)", id, va);
    }

    for (uint64_t r = 0; r < VT_ROUNDS; r++) {
        vmm_unmap(k_pml4, vt_va[id][r]);
        if (vmm_translate(vt_va[id][r]) != 0) 
            vt_fail("STILL MAPPED AFTER UNMAP", id, vt_va[id][r]);
        pmm_free(vt_pa[id][r], 0);
    }
}

void vt_a() {asm volatile("sti"); vt_worker(0);}
void vt_b() {asm volatile("sti"); vt_worker(1);}

void run_vmm_test() {
    struct task *vmm_test_a = task_create(vt_a);
    struct task *vmm_test_b = task_create(vt_b);
    uint64_t before = pmm_free_bytes();

    task_add(vmm_test_a);
    task_add(vmm_test_b);

    while (vt_mapped < 2 && !vt_failed) asm volatile("hlt");

    uint64_t after = pmm_free_bytes();
    uint64_t used = before - after;
    uint64_t errs = pmm_check();
    serial_printf("[vmm_stress]: frames used by page tables: %u  pmm errors: %u\n",
                  (void*[]){&used, &errs});
    serial_print(!vt_failed && errs == 0 ? "[vmm_stress]: PASS\n" : "[vmm_stress]: FAIL\n");
}