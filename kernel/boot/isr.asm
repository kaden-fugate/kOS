bits 64

; declared in idt.c
extern idt_common_handler

; MACRO - dummy exception with no cpu error code
; DEFIN - put zeroes as the error code, push current vector
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0    
    push qword %1 
    jmp isr_common_stub
%endmacro

; MACRO - exception WITH cpu error code
; DEFIN - vectors 8 - 16 (excluding 9, 15, 16) have cpu push a real error code
;         before jumping. cant push a dummy error code for it.
%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1 
    jmp isr_common_stub
%endmacro

; generate all 32 cpu exceptions
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; every stub will jump here
isr_common_stub:
    ; save the registers not already saved by the cpu in the order that idt.h's
    ; interrupt_frame expects to read them back
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; move a pointer to everything we just pushed into rdi. this will be the
    ; argument for our C handler (see idt.c for more info)
    mov rdi, rsp
    call idt_common_handler

    ; restore all registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; discard vector and error code that we pushed prior to ist_common_stub
    ; jump. not expected in iretq
    add rsp, 16

    ; return but also pop rip, cs, rflags, rsp, ss (these were pushed to the
    ; cpu automatically when the interrupt fired)
    iretq