/* PE32 (.exe) loader: maps sections, applies relocations, links imports to NightOS's Win32 layer. */
#include "kernel.h"

#define USER_LO 0x00400000u
#define USER_HI 0x01000000u
#define PROG_STACK (256 * 1024)

typedef struct __attribute__((packed)) { uint16_t magic; uint8_t pad[58]; uint32_t lfanew; } dos_hdr;
typedef struct __attribute__((packed)) {
    uint32_t sig; uint16_t machine, nsect; uint32_t stamp, symptr, nsym; uint16_t optsize, chars;
} nt_hdr;
typedef struct __attribute__((packed)) { uint32_t rva, size; } data_dir;
typedef struct __attribute__((packed)) {
    uint16_t magic; uint8_t lmaj, lmin; uint32_t code_sz, idata_sz, udata_sz, entry, code_base, data_base, image_base;
    uint32_t sect_align, file_align; uint16_t osmaj, osmin, imgmaj, imgmin, submaj, submin;
    uint32_t win32ver, image_size, headers_size, checksum; uint16_t subsystem, dllchars;
    uint32_t stack_res, stack_com, heap_res, heap_com, ldflags, ndirs; data_dir dir[16];
} opt_hdr;
typedef struct __attribute__((packed)) {
    char name[8]; uint32_t vsize, vaddr, raw_size, raw_ptr, relptr, lineptr; uint16_t nrel, nline; uint32_t chars;
} sect_hdr;
typedef struct __attribute__((packed)) { uint32_t lookup, stamp, fwd, name, iat; } imp_desc;

static int running;
static kjmp_buf exit_jmp;
static uint32_t exit_code;

int prog_running(void) { return running; }
void prog_abort(int code) { exit_code = (uint32_t)code; k_longjmp(exit_jmp, 1); }

static void missing_import(const char *name) {
    uint8_t old = con_get_attr();
    con_set_attr(0x0E);
    kprintf("\n[!] This program needs '%s', which NightOS doesn't provide yet.\n", name);
    con_set_attr(old);
    prog_abort(0xC0000139u);
}

/* Builds a tiny stub: push <name>; mov eax, missing_import; call eax */
static void *make_stub(const char *dll, const char *fn) {
    char *label = kmalloc(strlen(dll) + strlen(fn) + 2);
    uint8_t *s = kmalloc(12);
    if (!label || !s) return 0;
    strcpy(label, dll); strcat(label, "!"); strcat(label, fn);
    win32_track(label); win32_track(s);
    s[0] = 0x68; *(uint32_t *)(s + 1) = (uint32_t)label;
    s[5] = 0xB8; *(uint32_t *)(s + 6) = (uint32_t)missing_import;
    s[10] = 0xFF; s[11] = 0xD0;
    return s;
}

typedef struct { const uint8_t *file; uint32_t fsize; const nt_hdr *nt; const opt_hdr *opt; const sect_hdr *sec; } pe_view;

static const char *pe_parse(const kfile *f, pe_view *v) {
    if (f->size < sizeof(dos_hdr)) return "file is too small";
    const dos_hdr *d = (const dos_hdr *)f->data;
    if (d->magic != 0x5A4D) return "not an .exe file (missing MZ signature)";
    if (d->lfanew + sizeof(nt_hdr) + sizeof(opt_hdr) > f->size) return "damaged header";
    v->file = f->data; v->fsize = f->size;
    v->nt = (const nt_hdr *)(f->data + d->lfanew);
    if (v->nt->sig != 0x00004550) return "not a PE executable";
    if (v->nt->machine == 0x8664) return "64-bit program (NightOS runs 32-bit .exe files only for now)";
    if (v->nt->machine != 0x14C) return "unsupported CPU type";
    v->opt = (const opt_hdr *)(v->nt + 1);
    if (v->opt->magic != 0x10B) return "unsupported PE format";
    v->sec = (const sect_hdr *)((const uint8_t *)v->opt + v->nt->optsize);
    if ((const uint8_t *)(v->sec + v->nt->nsect) > f->data + f->size) return "damaged section table";
    if (v->opt->image_size > USER_HI - USER_LO) return "program is too large";
    return 0;
}

static const char *pe_map(const pe_view *v, uint32_t *base_out) {
    const opt_hdr *o = v->opt;
    uint32_t base = o->image_base;
    if (base < USER_LO || base + o->image_size > USER_HI) base = USER_LO;
    uint8_t *img = (uint8_t *)base;
    memset(img, 0, o->image_size);
    memcpy(img, v->file, o->headers_size < v->fsize ? o->headers_size : v->fsize);
    for (int i = 0; i < v->nt->nsect; i++) {
        const sect_hdr *s = &v->sec[i];
        uint32_t n = s->raw_size;
        if (s->vsize && s->vsize < n) n = s->vsize;
        if (s->raw_ptr + n > v->fsize || s->vaddr + n > o->image_size) return "section outside file";
        memcpy(img + s->vaddr, v->file + s->raw_ptr, n);
    }
    /* relocations */
    int32_t delta = (int32_t)(base - o->image_base);
    if (delta) {
        if (o->ndirs <= 5 || !o->dir[5].size) return "program can't be moved and its preferred address is unavailable";
        uint8_t *p = img + o->dir[5].rva, *end = p + o->dir[5].size;
        while (p < end) {
            uint32_t page = *(uint32_t *)p, bsize = *(uint32_t *)(p + 4);
            if (bsize < 8) break;
            for (uint16_t *e = (uint16_t *)(p + 8); (uint8_t *)e < p + bsize; e++) {
                if ((*e >> 12) == 3) *(uint32_t *)(img + page + (*e & 0xFFF)) += (uint32_t)delta;
            }
            p += bsize;
        }
    }
    /* imports */
    if (o->ndirs > 1 && o->dir[1].size) {
        for (imp_desc *d = (imp_desc *)(img + o->dir[1].rva); d->name; d++) {
            const char *dll = (const char *)(img + d->name);
            uint32_t *look = (uint32_t *)(img + (d->lookup ? d->lookup : d->iat));
            uint32_t *iat = (uint32_t *)(img + d->iat);
            for (; *look; look++, iat++) {
                char ordname[16];
                const char *fn;
                if (*look & 0x80000000u) { snprintf(ordname, sizeof ordname, "#%u", *look & 0xFFFF); fn = ordname; }
                else fn = (const char *)(img + *look + 2);
                void *impl = win32_lookup(dll, fn);
                if (!impl) impl = make_stub(dll, fn);
                *iat = (uint32_t)impl;
            }
        }
    }
    *base_out = base;
    return 0;
}

static kfile *find_exe(const char *name) {
    kfile *f = fs_find(name);
    if (!f) { char n2[FS_NAME_MAX + 4]; snprintf(n2, sizeof n2, "%s.exe", name); f = fs_find(n2); }
    return f;
}

int pe_run(const char *name, const char *cmdline) {
    static kfile *f;
    f = find_exe(name);
    if (!f) return -1;
    static pe_view v;
    static const char *err;
    err = pe_parse(f, &v);
    if (err) { kprintf("Can't run %s: %s\n", f->name, err); return -2; }

    static uint32_t base;
    static uint8_t *stack;
    static char saved_title[48];
    strncpy0(saved_title, con_get_title(), sizeof saved_title);
    win32_begin(cmdline, 0);
    exit_code = 0;
    running = 1;
    if (k_setjmp(exit_jmp) == 0) {
        err = pe_map(&v, &base);
        if (err) { kprintf("Can't run %s: %s\n", f->name, err); prog_abort(-2); }
        stack = kmalloc(PROG_STACK);
        if (!stack) { kprintf("Not enough memory to start %s\n", f->name); prog_abort(-3); }
        win32_track(stack);
        win32_set_base(base);
        uint8_t *teb = cpu_teb();
        *(uint32_t *)(teb + 0x04) = (uint32_t)(stack + PROG_STACK);
        *(uint32_t *)(teb + 0x08) = (uint32_t)stack;
        *(uint32_t *)(cpu_peb() + 0x08) = base;
        con_set_title(f->name);
        exit_code = call_on_stack((void *)(base + v.opt->entry), stack + PROG_STACK);
    }
    running = 0;
    win32_end();
    con_set_title(saved_title);
    con_set_attr(0x07);
    return (int)exit_code;
}

void pe_info(const char *name) {
    kfile *f = find_exe(name);
    if (!f) { kprintf("No such file: %s\n", name); return; }
    pe_view v;
    const char *err = pe_parse(f, &v);
    if (err) { kprintf("%s: %s\n", f->name, err); return; }
    const opt_hdr *o = v.opt;
    kprintf("%s: 32-bit Windows %s program, %u bytes\n", f->name, o->subsystem == 3 ? "console" : "GUI", f->size);
    kprintf("  image base %p, size %u, entry +%x\n", o->image_base, o->image_size, o->entry);
    for (int i = 0; i < v.nt->nsect; i++) {
        char nm[9]; memcpy(nm, v.sec[i].name, 8); nm[8] = 0;
        kprintf("  section %-8s at +%x  %u bytes\n", nm, v.sec[i].vaddr, v.sec[i].vsize);
    }
    if (o->ndirs <= 1 || !o->dir[1].size) return;
    /* imports live at RVAs; translate RVA -> file offset */
#define RVA(r) ({ uint32_t _r = (r); const uint8_t *_p = 0; \
        for (int _i = 0; _i < v.nt->nsect; _i++) if (_r >= v.sec[_i].vaddr && _r < v.sec[_i].vaddr + v.sec[_i].raw_size) \
            { _p = v.file + v.sec[_i].raw_ptr + (_r - v.sec[_i].vaddr); break; } _p; })
    const imp_desc *d = (const imp_desc *)RVA(o->dir[1].rva);
    int ok = 0, missing = 0;
    for (; d && d->name; d++) {
        const char *dll = (const char *)RVA(d->name);
        const uint32_t *l = (const uint32_t *)RVA(d->lookup ? d->lookup : d->iat);
        kprintf("  uses %s:", dll ? dll : "?");
        int col = 12;
        for (; l && *l; l++) {
            if (*l & 0x80000000u) { kprintf(" #%u", *l & 0xFFFF); missing++; continue; }
            const char *fn = (const char *)RVA(*l + 2);
            if (!fn) continue;
            int have = win32_lookup(dll, fn) != 0;
            have ? ok++ : missing++;
            if (col + (int)strlen(fn) + 2 > 78) { kprintf("\n   "); col = 3; }
            uint8_t a = con_get_attr();
            con_set_attr(have ? 0x0A : 0x0C);
            kprintf(" %s", fn);
            con_set_attr(a);
            col += (int)strlen(fn) + 1;
        }
        kprintf("\n");
    }
    kprintf("  %d functions supported, %d missing\n", ok, missing);
}
