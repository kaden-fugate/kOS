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

%define USER_DATA_BASE 0x0000000050000000
%define SYS_WRITE      1
%define SYS_EXIT       2
%define SYS_YIELD      3

user_test_begin:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rbx, USER_DATA_BASE
    mov r12, [rbx]

    mov rax, 0xA5A5A5A5A5A5A5A5
    xor r12, rax
    mov [rbx], r12

    mov rax, SYS_YIELD
    int 0x80

    mov rax, SYS_WRITE
    lea rdi, [rel user_message] 
    mov rsi, user_message_end - user_message 
    int 0x80

    mov rax, SYS_EXIT
    int 0x80

    ud2

user_message:
    db "hello from ring 3", 10
user_message_end:

user_test_end: