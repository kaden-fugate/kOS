; void enter_user(uint64_t user_rip, uint64_t user_rsp)
; rdi = user RIP, rsi = user RSP

global enter_user
enter_user:
    cli

    push qword 0x23      ; user SS: GDT entry 4, RPL 3
    push rsi             ; user RSP
    pushfq
    or qword[rsp], 0x200 ; user RFLAGS: IF=1
    push qword 0x1B      ; user CS: GDT entry 3, RPL 3
    push rdi             ; user RIP
    iretq

section .rodata
global user_test_begin
global user_test_end

user_test_begin:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, 1
    int 0x80

    ud2
user_test_end: