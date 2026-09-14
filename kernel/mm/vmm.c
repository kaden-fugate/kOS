#include "mm/vmm.h"
#include "mm/pmm.h"
#include "drivers/serial.h"
#include "kutil/string.h"

static uint64_t *k_pml4;

void vmm_init() {
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));

    k_pml4 = PHYS_TO_VIRT(cr3 & PTE_ADDR_MASK);
    serial_printf("[vmm_init] k_pml4 at: %x\n", (void*[]){&k_pml4});
}

static uint64_t *get_or_create_table(uint64_t *parent_ent) {
    // if parent entry not preesnt:
    //  allocate physical memory for table (order 0)
    //  set table to point to physical mem + present flag + writeable + user
    if (!(*parent_ent & PTE_PRESENT)) {
        uint64_t new_table = pmm_alloc(0);
        memset((void*)PHYS_TO_VIRT(new_table), 0, PAGE_SIZE);
        *parent_ent = new_table | PTE_PRESENT | PTE_WRITEABLE | PTE_USER;
    }

    // return parent entrys physical address
    return PHYS_TO_VIRT(*parent_ent & PTE_ADDR_MASK);
}

void vmm_map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    // get pdpt, pd, pt
    uint64_t *pdpt = get_or_create_table(&pml4[PML4_IDX(virt)]);
    uint64_t *pd   = get_or_create_table(&pdpt[PDPT_IDX(virt)]);
    uint64_t *pt   = get_or_create_table(&pd[PD_IDX(virt)]);

    // map PTE to phys with given flags + present
    pt[PT_IDX(virt)] = (phys & PTE_ADDR_MASK) | PTE_PRESENT | flags;

    // invalidate the previous TLB cache for this virtual address
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_translate(uint64_t virt) {
    uint64_t *pdpt = 0x0;
    uint64_t *pd   = 0x0;
    uint64_t *pt   = 0x0;
    uint64_t  base = 0x0;

    // get pml4e (0x0 if not present)
    uint64_t pml4e = k_pml4[PML4_IDX(virt)];
    if (!(pml4e & PTE_PRESENT)) return 0x0;
    pdpt = PHYS_TO_VIRT(pml4e & PTE_ADDR_MASK);

    // get pdpte (0x0 if not present, huge page if PS flag set)
    uint64_t pdpte = pdpt[PDPT_IDX(virt)];
    if (!(pdpte & PTE_PRESENT)) return 0x0;
    if (pdpte & PTE_PS) { 
        /* handle huge page case here */ 
        base = pdpte & 0xFFFFFFC0000000ULL;
        return base | (virt & 0x3FFFFFFFULL);
    }
    pd = PHYS_TO_VIRT(pdpte & PTE_ADDR_MASK);

    // same as above for pde
    uint64_t pde = pd[PD_IDX(virt)];
    if (!(pde & PTE_PRESENT)) return 0x0;
    if (pde & PTE_PS) { 
        /*handle huge page case here*/
        base = pde & 0xFFFFFFFFFFE00000ULL;
        return base | (virt & 0x1FFFFFULL);
    }
    pt = PHYS_TO_VIRT(pde & PTE_ADDR_MASK);

    // get pte (0x0 if not present)
    uint64_t pte = pt[PT_IDX(virt)];
    if (!(pte & PTE_PRESENT)) return 0x0;

    base = pte & PTE_ADDR_MASK;
    return base | (virt & 0xFFF);
}

void vmm_unmap(uint64_t *pml4, uint64_t virt) {
    uint64_t *pdpt = PHYS_TO_VIRT(pml4[PML4_IDX(virt)] & PTE_ADDR_MASK);
    if (!(pml4[PML4_IDX(virt)] & PTE_PRESENT)) return;

    uint64_t *pd = PHYS_TO_VIRT(pdpt[PDPT_IDX(virt)] & PTE_ADDR_MASK);
    if (!(pdpt[PDPT_IDX(virt)] & PTE_PRESENT)) return;

    uint64_t *pt = PHYS_TO_VIRT(pd[PD_IDX(virt)] & PTE_ADDR_MASK);
    if (!(pd[PD_IDX(virt)] & PTE_PRESENT)) return;

    pt[PT_IDX(virt)] = 0x0;
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

uint64_t vmm_alloc(
    uint64_t *pml4, uint64_t virt, uint8_t order, uint64_t flags
) {
    uint64_t phys = pmm_alloc(order);
    if (!phys) return 0x0;

    uint64_t n_pages = 1ULL << order;
    for (int i = 0; i < n_pages; i++) {
        vmm_map(pml4, virt + (i * PAGE_SIZE), phys + (i * PAGE_SIZE), flags);
    }

    return virt;
}

void vmm_test() {
    
    // test vmm_translate
    uint64_t result = vmm_translate(0x100000);
    serial_printf(
        "[vmm_test] translate(0x100000) = %x (expected: 0x100000)\n",
        (void*[]){&result}
    );

    result = vmm_translate(0xFFFF800000000000);
    serial_printf(
        "[vmm_test] translate(0xFFFF800000000000) = %x (expected: 0x0)\n",
        (void*[]){&result}
    );

    // test vmm_map
    uint64_t phys = pmm_alloc(0);
    uint64_t virt = 0xFFFF800000000000;
    
    vmm_map(k_pml4, virt, phys, PTE_WRITEABLE);
    uint64_t trns = vmm_translate(virt);

    serial_printf(
        "[vmm_test] phys      = %x\n"
        "[vmm_test] virt      = %x\n"
        "[vmm_test] transated = %x (expected: %x)\n", 
        (void*[]){&phys, &virt, &trns, &phys}
    );

    // test mapping worked
    uint64_t *test_ptr = (uint64_t *)virt;
    *test_ptr = 0xCAFEBABE;
    serial_print("[vmm_test] no crash on write to virtual address? :^)\n");

    uint64_t *phys_ptr = (uint64_t *)phys;
    serial_printf(
        "[vmm_test] read physical mem = %x (expected: 0xCAFEBABE)\n",
        (void*[]){phys_ptr}
    );

    // test unmap virt mem
    vmm_unmap(k_pml4, virt);
    trns = vmm_translate(virt);
    
    serial_printf(
        "[vmm_test] transated = %x (expected: 0x0)\n", 
        (void*[]){&trns}
    );

    // test permissions
    uint64_t r_phys = pmm_alloc(0);
    uint64_t r_virt = 0xCAFEBABE;
    vmm_map(k_pml4, r_virt, r_phys, 0);

    serial_print("[vmm_test] write to read-only memory\n");
    *(uint64_t *)r_virt = 0xCAFEBABE;
    serial_print("[vmm_test] if we reached this point, permissions enforcement failed.\n");
}