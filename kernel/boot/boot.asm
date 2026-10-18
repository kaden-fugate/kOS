%define KERNEL_VMA    0xFFFFFFFF80000000
%define PHYS_MAP_BASE 0xFFFF800000000000
%define KERNEL_PHYS   0x0

%define IDENTITY_PML4_IDX 0
%define PHYSMAP_PML4_IDX  256
%define KERNEL_PML4_IDX   511

%define IDENTITY_PDPT_IDX 0
%define PHYSMAP_PDPT_IDX  0
%define KERNEL_PDPT_IDX   510

%define PAGE_PRESENT  0x001
%define PAGE_WRITABLE 0x002
%define PAGE_HUGE     0x080
%define PAGE_2M_FLAGS (PAGE_PRESENT | PAGE_WRITABLE | PAGE_HUGE)

global kernel_stack_top
global kernel_stack_bottom

section .bss
alignb 16
kernel_stack_bottom:
    resb 16384
kernel_stack_top:

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

section .boot.bss nobits alloc wrtie align=4096
align 4096

p4_table: resb 4096
p3_table: resb 4096
p2_table: resb 4096

p3_kernel: resb 4096
p2_kernel: resb 4096

align 16
stack_bottom: resb 16384
stack_top:

align 4
magic:    resd 1
info_ptr: resd 1

section .boot.text exec
bits 32
global _start
_start:
    cli
    cld

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
    or eax, PAGE_PRESENT | PAGE_WRITABLE 
    
    mov [p4_table + IDENTITY_PML4_IDX * 8], eax ; temp lower half pml4 entry
    mov [p4_table + PHYSMAP_PML4_IDX  * 8], eax ; direct physical mapping

    mov eax, p3_kernel
    or  eax, PAGE_PRESENT | PAGE_WRITABLE
    mov [p4_table + KERNEL_PML4_IDX   * 8], eax ; higher-half kernel image

    ; PDPT[0] -> PD
    mov eax, p2_table
    or eax, PAGE_PRESENT | PAGE_WRITABLE

    mov [p3_table + IDENTITY_PDPT_IDX * 8], eax ; identity and physmap both use entry 0
    xor ecx, ecx

.map_identity:
    mov eax, ecx
    shl eax, 21
    or eax, PAGE_2M_FLAGS

    mov [p2_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_identity

    mov eax, p2_kernel
    or eax, PAGE_PRESENT | PAGE_WRITABLE

    mov [p3_kernel + KERNEL_PDPT_IDX * 8], eax
    xor ecx, ecx

.map_kernel:
    mov eax, ecx
    shl eax, 21
    add eax, KERNEL_PHYS
    or eax, PAGE_2M_FLAGS

    mov [p2_kernel + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_kernel
    ret

enable_paging:
    mov eax, cr4
    or eax, 1 << 5           ; PAE
    mov cr4, eax

    mov eax, p4_table
    mov cr3, eax

    mov ecx, 0xC0000080      ; EFER MSR
    rdmsr
    or eax, 1 << 8           ; LME (long mode enable)
    wrmsr

    mov eax, cr0
    or eax, 1 << 31          ; PG (enable paging)
    mov cr0, eax
    ret

section .boot.rodata
align 8
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53) ; code seg, present, long mode
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .boot.text exec
bits 64
extern kernel_main
long_mode_start:
    mov rsp, kernel_stack_top

    xor eax, eax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    cld

    mov edi, [info_ptr]
    mov esi, [magic]

    mov rax, kernel_main
    call rax

.hang:
    hlt
    jmp .hang