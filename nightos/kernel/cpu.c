/* GDT (with an FS segment pointing at a thread block, as Windows programs expect) and IDT. */
#include "kernel.h"

typedef struct __attribute__((packed)) { uint16_t lim; uint16_t base_lo; uint8_t base_mid, access, gran, base_hi; } gdt_entry;
typedef struct __attribute__((packed)) { uint16_t lim; uint32_t base; } table_ptr;
typedef struct __attribute__((packed)) { uint16_t off_lo, sel; uint8_t zero, flags; uint16_t off_hi; } idt_entry;

static gdt_entry gdt[4];
static idt_entry idt[32];
static table_ptr gdtr, idtr;
static uint8_t teb[0x1000] __attribute__((aligned(16)));
static uint8_t peb[0x200] __attribute__((aligned(16)));

extern void gdt_flush(void *);
extern void idt_load(void *);
extern uint32_t isr_table[32];

uint8_t *cpu_teb(void) { return teb; }
uint8_t *cpu_peb(void) { return peb; }

static void set_gdt(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    gdt[i].lim = limit & 0xFFFF;
    gdt[i].base_lo = base & 0xFFFF;
    gdt[i].base_mid = (base >> 16) & 0xFF;
    gdt[i].access = access;
    gdt[i].gran = (uint8_t)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    gdt[i].base_hi = (base >> 24) & 0xFF;
}

void gdt_init(void) {
    set_gdt(0, 0, 0, 0, 0);
    set_gdt(1, 0, 0xFFFFF, 0x9A, 0xC0);                 /* code */
    set_gdt(2, 0, 0xFFFFF, 0x92, 0xC0);                 /* data */
    set_gdt(3, (uint32_t)teb, sizeof teb - 1, 0x92, 0x40); /* FS: thread block */
    *(uint32_t *)(teb + 0x00) = 0xFFFFFFFF;             /* no exception handlers registered */
    *(uint32_t *)(teb + 0x18) = (uint32_t)teb;          /* self pointer */
    *(uint32_t *)(teb + 0x30) = (uint32_t)peb;          /* process block */
    gdtr.lim = sizeof gdt - 1; gdtr.base = (uint32_t)gdt;
    gdt_flush(&gdtr);
}

void idt_init(void) {
    for (int i = 0; i < 32; i++) {
        idt[i].off_lo = isr_table[i] & 0xFFFF;
        idt[i].off_hi = isr_table[i] >> 16;
        idt[i].sel = 0x08; idt[i].zero = 0; idt[i].flags = 0x8E;
    }
    idtr.lim = sizeof idt - 1; idtr.base = (uint32_t)idt;
    idt_load(&idtr);
}

typedef struct { uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, vec, err, eip, cs, eflags; } isr_frame;

static const char *exc_names[32] = {
    "Divide by zero", "Debug", "NMI", "Breakpoint", "Overflow", "Bound range", "Invalid opcode",
    "No FPU", "Double fault", "Coprocessor", "Bad TSS", "Segment not present", "Stack fault",
    "General protection fault", "Page fault", "Reserved", "x87 FPU error", "Alignment check",
    "Machine check", "SIMD FPU error",
};

void isr_handler(isr_frame *f) {
    const char *name = (f->vec < 32 && exc_names[f->vec]) ? exc_names[f->vec] : "CPU exception";
    char detail[96];
    snprintf(detail, sizeof detail, "at address %p (error code %x)", f->eip, f->err);
    if (prog_running()) {
        uint8_t old = con_get_attr();
        con_set_attr(0x0C);
        kprintf("\n[!] The program stopped working: %s %s\n", name, detail);
        kprintf("    NightOS closed it. Your system is fine.\n");
        con_set_attr(old);
        prog_abort(0xC0000000u | f->vec);
    }
    con_panic_screen(name, detail);
    for (;;) __asm__ volatile("cli; hlt");
}
