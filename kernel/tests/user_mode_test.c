#include "user_mode_test.h"

#include "drivers/serial.h"
#include "kernel/task.h"
#include "kernel/user_mode.h"
#include "mm/def.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "kutil/string.h"

#define USER_CODE_BASE 0x0000000040000000ULL
#define USER_STACK_TOP 0x0000000070000000ULL

extern const uint8_t user_test_begin[];
extern const uint8_t user_test_end[];

static uint64_t user_code_phys;
static uint64_t user_stack_phys;

static void user_task() {
    enter_user(USER_CODE_BASE, USER_STACK_TOP - 16);
}

void run_user_mode_test() {
    uint64_t code_size = (uint64_t)(user_test_end - user_test_begin);

    if (code_size > PAGE_SIZE) {
        serial_printf("[user_test] user code is too large [%x]\n", 
            (void*[]){&code_size});
        return;
    }

    user_code_phys  = pmm_alloc(0);
    user_stack_phys = pmm_alloc(0);

    if (!user_code_phys || !user_stack_phys) {
        serial_print("[user_test] out of memory\n");
        return;
    }

    memcpy(PHYS_TO_VIRT(user_code_phys), user_test_begin, code_size);
    memset(PHYS_TO_VIRT(user_stack_phys), 0, PAGE_SIZE);

    vmm_map(k_pml4, USER_CODE_BASE, user_code_phys, PTE_USER);
    vmm_map(k_pml4, USER_STACK_TOP - PAGE_SIZE, user_stack_phys, 
        PTE_USER | PTE_WRITEABLE);

    struct task *t = task_create(user_task);
    if (!t) {
        serial_print("[user_test] task creation failed\n");
        return;
    }

    serial_print("[user_test] starting ring-3 task\n");
    task_add(t);

    task_join(t);
    task_reap(t);

    vmm_unmap(k_pml4, USER_CODE_BASE);
    vmm_unmap(k_pml4, USER_STACK_TOP - PAGE_SIZE);
    pmm_free(user_code_phys, 0);
    pmm_free(user_stack_phys, 0);

    serial_print("[user_test] PASS: user task faulted and was reaped\n");
}