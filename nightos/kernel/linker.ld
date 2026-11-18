ENTRY(_start)
SECTIONS
{
    . = 1M;
    .text ALIGN(4K) : { *(.multiboot) *(.text*) }
    .rodata ALIGN(4K) : { *(.rodata*) }
    .data ALIGN(4K) : { *(.data*) }
    .bss ALIGN(4K) : { *(COMMON) *(.bss*) }
    kernel_end = .;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
ASSERT(kernel_end <= 0x400000, "kernel too big: overlaps the program load area at 4 MB")
