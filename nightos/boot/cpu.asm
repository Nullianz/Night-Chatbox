; Low-level CPU helpers: GDT/IDT loading, exception stubs, setjmp/longjmp, stack switch.
section .text

global gdt_flush
gdt_flush:                     ; void gdt_flush(void *gdtr)
    mov eax, [esp + 4]
    lgdt [eax]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov gs, ax
    mov ax, 0x18               ; FS -> thread block (what Windows programs expect)
    mov fs, ax
    jmp 0x08:.reload
.reload:
    ret

global idt_load
idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; ---- exception stubs ----
extern isr_handler
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0
    push dword %1
    jmp isr_common
%endmacro
%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1
    jmp isr_common
%endmacro

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
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_ERR   29
ISR_ERR   30
ISR_NOERR 31

isr_common:
    pusha
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    push esp
    call isr_handler
    add esp, 4
    popa
    add esp, 8
    iret

global isr_table
section .rodata
isr_table:
%assign i 0
%rep 32
    dd isr%+i
%assign i i+1
%endrep

section .text
; int k_setjmp(kjmp_buf b) / void k_longjmp(kjmp_buf b, int v)
global k_setjmp
k_setjmp:
    mov eax, [esp + 4]
    mov [eax], ebx
    mov [eax + 4], esi
    mov [eax + 8], edi
    mov [eax + 12], ebp
    lea ecx, [esp + 4]
    mov [eax + 16], ecx
    mov ecx, [esp]
    mov [eax + 20], ecx
    xor eax, eax
    ret

global k_longjmp
k_longjmp:
    mov edx, [esp + 4]
    mov eax, [esp + 8]
    test eax, eax
    jnz .nz
    inc eax
.nz:
    mov cx, 0x18               ; programs may have clobbered FS
    mov fs, cx
    mov ebx, [edx]
    mov esi, [edx + 4]
    mov edi, [edx + 8]
    mov ebp, [edx + 12]
    mov esp, [edx + 16]
    jmp [edx + 20]

; uint32_t call_on_stack(void *fn, void *stack_top)
global call_on_stack
call_on_stack:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    mov eax, [ebp + 8]
    mov esp, [ebp + 12]
    call eax
    lea esp, [ebp - 12]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
