#include "user_mode_test.h"

#include "drivers/serial.h"
#include "kernel/task.h"
#include "kernel/user_mode.h"
#include "mm/def.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "kutil/string.h"

#define USER_CODE_BASE 0x0000000040000000ULL
#define USER_DATA_BASE 0x0000000050000000ULL
#define USER_STACK_TOP 0x0000000070000000ULL

extern const uint8_t user_test_begin[];
extern const uint8_t user_test_end[];

static uint64_t a_code_phys;
static uint64_t a_stack_phys;
static uint64_t a_data_phys;
static uint64_t b_code_phys;
static uint64_t b_stack_phys;
static uint64_t b_data_phys;

static void user_task() {
    enter_user(USER_CODE_BASE, USER_STACK_TOP - 16);
}

void run_user_mode_test() {
    uint64_t code_size = (uint64_t)(user_test_end - user_test_begin);

    if (code_size > PAGE_SIZE) {
        serial_printf("[user_test] user code is too large [%x]\n", 
            code_size);
        return;
    }

    a_code_phys  = pmm_alloc(0);
    a_stack_phys = pmm_alloc(0);
    a_data_phys  = pmm_alloc(0);
    b_code_phys  = pmm_alloc(0);
    b_stack_phys = pmm_alloc(0);
    b_data_phys  = pmm_alloc(0);

    if (!a_code_phys || !a_stack_phys || !a_data_phys || !b_code_phys || !b_stack_phys
        || !b_data_phys) {
        serial_print("[user_test] out of memory\n");
        return;
    }

    memcpy(PHYS_TO_VIRT(a_code_phys), user_test_begin, code_size);
    memset(PHYS_TO_VIRT(a_stack_phys), 0, PAGE_SIZE);
    memset(PHYS_TO_VIRT(a_data_phys), 0xF0, 8);

    memcpy(PHYS_TO_VIRT(b_code_phys), user_test_begin, code_size);
    memset(PHYS_TO_VIRT(b_stack_phys), 0, PAGE_SIZE);
    memset(PHYS_TO_VIRT(b_data_phys), 0x0F, 8);

    uint64_t *a_pml4;
    uint64_t  a_pml4_phys;
    if (vmm_create_user_pml4(&a_pml4, &a_pml4_phys) != 0) {
        serial_print("[user_test] out of memory\n");
        return;
    }

    uint64_t *b_pml4;
    uint64_t  b_pml4_phys;
    if (vmm_create_user_pml4(&b_pml4, &b_pml4_phys) != 0) {
        serial_print("[user_test] out of memory\n");
        return;
    }

    vmm_map(a_pml4, USER_CODE_BASE, a_code_phys, PTE_USER);
    vmm_map(a_pml4, USER_STACK_TOP - PAGE_SIZE, a_stack_phys, 
        PTE_USER | PTE_WRITEABLE);
    vmm_map(a_pml4, USER_DATA_BASE, a_data_phys, PTE_USER | PTE_WRITEABLE);

    vmm_map(b_pml4, USER_CODE_BASE, b_code_phys, PTE_USER);
    vmm_map(b_pml4, USER_STACK_TOP - PAGE_SIZE, b_stack_phys, 
        PTE_USER | PTE_WRITEABLE);
    vmm_map(b_pml4, USER_DATA_BASE, b_data_phys, PTE_USER | PTE_WRITEABLE);

    struct task *at = task_create(user_task);
    struct task *bt = task_create(user_task);
    if (!at) {
        serial_print("[user_test] task creation failed\n");
        return;
    }
    if (!bt) {
        serial_print("[user_test] task creation failed\n");
        return;
    }

    at->pml4      = a_pml4;
    at->pml4_phys = a_pml4_phys;

    bt->pml4      = b_pml4;
    bt->pml4_phys = b_pml4_phys;

    serial_print("[user_test] starting ring-3 task\n");
    task_add(at);
    task_add(bt);

    task_join(at);
    task_reap(at);

    task_join(bt);
    task_reap(bt);

    serial_printf("[user_test]: TASK A [%x], TASK B [%x]\n", 
        *(uint64_t*)PHYS_TO_VIRT(a_data_phys), *(uint64_t*)PHYS_TO_VIRT(b_data_phys));

    vmm_unmap(a_pml4, USER_CODE_BASE);
    vmm_unmap(a_pml4, USER_STACK_TOP - PAGE_SIZE);
    vmm_unmap(a_pml4, USER_DATA_BASE);
    pmm_free(a_code_phys,  0);
    pmm_free(a_stack_phys, 0);
    pmm_free(a_data_phys,  0);

    vmm_unmap(b_pml4, USER_CODE_BASE);
    vmm_unmap(b_pml4, USER_STACK_TOP - PAGE_SIZE);
    vmm_unmap(b_pml4, USER_DATA_BASE);
    pmm_free(b_code_phys, 0);
    pmm_free(b_stack_phys, 0);
    pmm_free(b_data_phys,  0);

}