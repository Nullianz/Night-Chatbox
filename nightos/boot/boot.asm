; NightOS boot entry (Multiboot v1). GRUB or QEMU -kernel jumps here in 32-bit protected mode.
MB_ALIGN    equ 1 << 0
MB_MEMINFO  equ 1 << 1
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_MAGIC    equ 0x1BADB002
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)

section .multiboot
align 4
    dd MB_MAGIC, MB_FLAGS, MB_CHECKSUM

section .bss
align 16
kstack_bottom: resb 65536
kstack_top:

section .text
global _start
extern kmain
_start:
    cli
    mov esp, kstack_top
    ; enable x87 + SSE so programs built for modern x86 can use them
    mov ecx, cr0
    and ecx, ~(1 << 2)        ; clear EM
    or  ecx, (1 << 1)         ; set MP
    mov cr0, ecx
    mov ecx, cr4
    or  ecx, (3 << 9)         ; OSFXSR | OSXMMEXCPT
    mov cr4, ecx
    fninit
    push ebx                  ; multiboot info
    push eax                  ; multiboot magic
    call kmain
.hang:
    cli
    hlt
    jmp .hang
