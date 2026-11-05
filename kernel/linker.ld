ENTRY(_start)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    /* low phys + low virt bootstap code */
    . = 1M;
    .boot : {
        KEEP(*(.multiboot_header))
        *(.boot.text)
        *(.boot.rodata)
        *(.boot.data)
        *(.boot.bss)
    }

    . = KERNEL_VMA + .;
    .text : AT(ADDR(.text) - KERNEL_VMA) {
        *(.text)
    }
    .rodata : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata)
    }
    .data : AT(ADDR(.data) - KERNEL_VMA) {
        *(.data)
    }
    .bss : AT(ADDR(.bss) - KERNEL_VMA) {
        *(COMMON)
        *(.bss)
    }

    kernel_end = .;
    kernel_phys_end = kernel_end - KERNEL_VMA;
}