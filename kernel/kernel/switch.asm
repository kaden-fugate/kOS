; void switch_context(uint64_t *old_rsp, uint64_t new_rsp)
;   rdi = where to store the outgoing task's rsp
;   rsi = the incoming task's saved rsp
global switch_context

switch_context:
    push rbp        ; save old tasks registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov [rdi], rsp  ; move rsp into addr pointed to by rdi
    mov rsp,   rsi  ; now use incoming tasks stack

    pop r15         ; restore new tasks registers
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret             ; pop return addr off new stack and jump to it