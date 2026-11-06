#include "syscall.h"
#include "task.h"
#include "drivers/console.h"

#define SYS_OK     0ULL
#define SYS_ENOSYS ((uint64_t)-1)

static uint64_t sys_ping() {
    return 0xCAFEBABE;
}

static uint64_t sys_write(const char *buf, uint16_t len) {
    if (len > 4096)
        return SYS_ENOSYS;

    console_write(buf, len);
    return len;
}

uint64_t syscall_dispatch(struct interrupt_frame *frame) {
    switch (frame->rax) {
        case SYS_PING:
            return sys_ping();
        case SYS_WRITE:
            return sys_write((const char *)frame->rdi, frame->rsi);
        case SYS_EXIT:
            task_exit();
        case SYS_YIELD:
            schedule();
            return SYS_OK;
    }

    return SYS_ENOSYS;
}