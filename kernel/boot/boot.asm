section .multiboot_header
header_start:
    dd 0xe85250d6                ; multiboot2 magic number
    dd 0                         ; architecture: 0 = i386 protected mode
    dd header_end - header_start ; header length
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start)) ; checksum
    ; required end tag
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 16384
stack_top:
align 4
magic: resd 1
info_ptr: resd 1

section .text
bits 32
global _start
_start:
    mov esp, stack_top

    ; save eax and ebx
    mov [magic], eax
    mov [info_ptr], ebx

    call set_up_page_tables
    call enable_paging
    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

set_up_page_tables:
    ; PML4[0] -> PDPT
    mov eax, p3_table
    or eax, 0b11            ; present + writable
    mov [p4_table], eax

    ; PDPT[0] -> PD
    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    ; identity-map first 1GB using 2MB huge pages
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011      ; present + writable + huge page
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    ret

enable_paging:
    mov eax, p4_table
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5           ; PAE
    mov cr4, eax

    mov ecx, 0xC0000080      ; EFER MSR
    rdmsr
    or eax, 1 << 8           ; LME (long mode enable)
    wrmsr

    mov eax, cr0
    or eax, 1 << 31          ; PG (enable paging)
    mov cr0, eax
    ret

section .rodata
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53) ; code seg, present, long mode
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 64
extern kernel_main
long_mode_start:
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov edi, [info_ptr]
    mov esi, [magic]
    call kernel_main
    hlt