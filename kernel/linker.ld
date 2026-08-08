ENTRY(_start)
SECTIONS
{
    . = 1M;

    .boot   : { *(.multiboot_header) }
    .text   : { *(.text)             }
    .rodata : { *(.rodata)           }
    .data   : { *(.data)             }
    .bss    : { *(COMMON) *(.bss)    }
    kernel_end = .;
}