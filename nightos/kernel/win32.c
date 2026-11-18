/*
 * NightOS's own implementation of a subset of the Win32 API (kernel32 / user32).
 * Written from scratch against the publicly documented function signatures;
 * no Microsoft code is used.
 */
#include "kernel.h"

#define WINAPI __attribute__((stdcall))
typedef uint32_t DWORD;
typedef int BOOL;
typedef void *HANDLE;

#define INVALID_HANDLE ((HANDLE)-1)
#define H_STDIN  ((HANDLE)0x10)
#define H_STDOUT ((HANDLE)0x14)
#define H_STDERR ((HANDLE)0x18)
#define H_HEAP   ((HANDLE)0x20)
#define H_FILE0  0x100
#define MAX_HANDLES 32
#define MAX_TRACK 2048

enum { ERR_OK = 0, ERR_FILE_NOT_FOUND = 2, ERR_TOO_MANY = 4, ERR_ACCESS = 5, ERR_BAD_HANDLE = 6,
       ERR_NOMEM = 8, ERR_EXISTS = 80, ERR_PARAM = 87 };

typedef struct { int used; kfile *f; uint32_t pos; int write; } handle;
static handle handles[MAX_HANDLES];
static void *tracked[MAX_TRACK];
static DWORD last_error;
static char cmdline_buf[256];
static uint32_t image_base;

/* ---------- bookkeeping so programs can't leak memory or files ---------- */
void win32_track(void *p) {
    for (int i = 0; i < MAX_TRACK; i++) if (!tracked[i]) { tracked[i] = p; return; }
}
static void untrack(void *p) {
    for (int i = 0; i < MAX_TRACK; i++) if (tracked[i] == p) { tracked[i] = 0; return; }
}
static void *palloc(size_t n) { void *p = kmalloc(n); if (p) win32_track(p); return p; }
static void pfree(void *p) { if (p) { untrack(p); kfree(p); } }

void win32_begin(const char *cmdline, uint32_t base) {
    strncpy0(cmdline_buf, cmdline, sizeof cmdline_buf);
    image_base = base;
    last_error = 0;
    memset(handles, 0, sizeof handles);
    memset(tracked, 0, sizeof tracked);
}
void win32_set_base(uint32_t base) { image_base = base; }
void win32_end(void) {
    for (int i = 0; i < MAX_TRACK; i++) if (tracked[i]) { kfree(tracked[i]); tracked[i] = 0; }
    memset(handles, 0, sizeof handles);
}

static handle *get_file(HANDLE h) {
    uint32_t i = (uint32_t)h - H_FILE0;
    if (i >= MAX_HANDLES || !handles[i].used) return 0;
    return &handles[i];
}
static int is_console_out(HANDLE h) { return h == H_STDOUT || h == H_STDERR; }

/* ---------- console input: line-based like the Windows console ---------- */
static char inbuf[260];
static int in_len, in_pos;
static DWORD console_read(char *buf, DWORD n) {
    if (in_pos >= in_len) {
        int l = kreadline(inbuf, sizeof inbuf - 2);
        inbuf[l] = '\r'; inbuf[l + 1] = '\n';
        in_len = l + 2; in_pos = 0;
    }
    DWORD c = 0;
    while (c < n && in_pos < in_len) buf[c++] = inbuf[in_pos++];
    return c;
}

/* ================= kernel32 ================= */
static void WINAPI k_ExitProcess(DWORD code) { prog_abort((int)code); }
static HANDLE WINAPI k_GetStdHandle(DWORD which) {
    switch ((int)which) { case -10: return H_STDIN; case -11: return H_STDOUT; case -12: return H_STDERR; }
    return INVALID_HANDLE;
}
static DWORD WINAPI k_GetLastError(void) { return last_error; }
static void WINAPI k_SetLastError(DWORD e) { last_error = e; }
static char *WINAPI k_GetCommandLineA(void) { return cmdline_buf; }
static HANDLE WINAPI k_GetModuleHandleA(const char *name) { return name ? 0 : (HANDLE)image_base; }
static DWORD WINAPI k_GetTickCount(void) { return time_ms(); }
static void WINAPI k_Sleep(DWORD ms) { time_sleep(ms); }
static DWORD WINAPI k_GetCurrentProcessId(void) { return 1; }

static BOOL WINAPI k_WriteFile(HANDLE h, const void *buf, DWORD n, DWORD *written, void *ovl) {
    (void)ovl;
    if (written) *written = 0;
    if (is_console_out(h)) { con_write(buf, n); if (written) *written = n; return 1; }
    handle *fh = get_file(h);
    if (!fh) { last_error = ERR_BAD_HANDLE; return 0; }
    if (!fh->write || !fs_write(fh->f, fh->pos, buf, n)) { last_error = ERR_ACCESS; return 0; }
    fh->pos += n;
    if (written) *written = n;
    return 1;
}
static BOOL WINAPI k_WriteConsoleA(HANDLE h, const void *buf, DWORD n, DWORD *written, void *r) {
    (void)h; (void)r;
    con_write(buf, n); if (written) *written = n; return 1;
}
static BOOL WINAPI k_ReadFile(HANDLE h, void *buf, DWORD n, DWORD *rd, void *ovl) {
    (void)ovl;
    if (rd) *rd = 0;
    if (h == H_STDIN) { DWORD c = console_read(buf, n); if (rd) *rd = c; return 1; }
    handle *fh = get_file(h);
    if (!fh) { last_error = ERR_BAD_HANDLE; return 0; }
    DWORD avail = fh->pos < fh->f->size ? fh->f->size - fh->pos : 0;
    if (n > avail) n = avail;
    memcpy(buf, fh->f->data + fh->pos, n);
    fh->pos += n;
    if (rd) *rd = n;
    return 1;
}
static BOOL WINAPI k_ReadConsoleA(HANDLE h, void *buf, DWORD n, DWORD *rd, void *ctl) {
    (void)h; (void)ctl;
    DWORD c = console_read(buf, n); if (rd) *rd = c; return 1;
}
static BOOL WINAPI k_SetConsoleTextAttribute(HANDLE h, uint16_t a) { (void)h; con_set_attr((uint8_t)a); return 1; }
static BOOL WINAPI k_SetConsoleTitleA(const char *t) { con_set_title(t); return 1; }
static BOOL WINAPI k_SetConsoleCursorPosition(HANDLE h, DWORD xy) { (void)h; con_set_pos(xy & 0xFFFF, xy >> 16); return 1; }
static void WINAPI k_OutputDebugStringA(const char *s) { serial_puts("[debug] "); serial_puts(s); }

static HANDLE WINAPI k_CreateFileA(const char *name, DWORD access, DWORD share, void *sa, DWORD disp, DWORD flags, HANDLE tmpl) {
    (void)share; (void)sa; (void)flags; (void)tmpl;
    int want_write = (access & 0x40000000u) || (access & 0x10000000u);
    kfile *f = fs_find(name);
    switch (disp) {
    case 1: /* CREATE_NEW */ if (f) { last_error = ERR_EXISTS; return INVALID_HANDLE; } f = fs_create(name); break;
    case 2: /* CREATE_ALWAYS */ if (!f) f = fs_create(name); else if (f->readonly) { last_error = ERR_ACCESS; return INVALID_HANDLE; } else fs_truncate(f); break;
    case 3: /* OPEN_EXISTING */ if (!f) { last_error = ERR_FILE_NOT_FOUND; return INVALID_HANDLE; } break;
    case 4: /* OPEN_ALWAYS */ if (!f) f = fs_create(name); break;
    case 5: /* TRUNCATE_EXISTING */ if (!f) { last_error = ERR_FILE_NOT_FOUND; return INVALID_HANDLE; } if (f->readonly) { last_error = ERR_ACCESS; return INVALID_HANDLE; } fs_truncate(f); break;
    default: last_error = ERR_PARAM; return INVALID_HANDLE;
    }
    if (!f) { last_error = ERR_NOMEM; return INVALID_HANDLE; }
    if (want_write && f->readonly) { last_error = ERR_ACCESS; return INVALID_HANDLE; }
    for (int i = 0; i < MAX_HANDLES; i++)
        if (!handles[i].used) {
            handles[i] = (handle){1, f, 0, want_write};
            last_error = 0;
            return (HANDLE)(H_FILE0 + i);
        }
    last_error = ERR_TOO_MANY;
    return INVALID_HANDLE;
}
static BOOL WINAPI k_CloseHandle(HANDLE h) {
    handle *fh = get_file(h);
    if (fh) { fh->used = 0; return 1; }
    return h == H_STDIN || is_console_out(h) || h == H_HEAP;
}
static DWORD WINAPI k_GetFileSize(HANDLE h, DWORD *hi) {
    handle *fh = get_file(h);
    if (hi) *hi = 0;
    if (!fh) { last_error = ERR_BAD_HANDLE; return 0xFFFFFFFF; }
    return fh->f->size;
}
static DWORD WINAPI k_SetFilePointer(HANDLE h, int32_t dist, int32_t *hi, DWORD method) {
    (void)hi;
    handle *fh = get_file(h);
    if (!fh) { last_error = ERR_BAD_HANDLE; return 0xFFFFFFFF; }
    int32_t base = method == 0 ? 0 : method == 1 ? (int32_t)fh->pos : (int32_t)fh->f->size;
    if (base + dist < 0) { last_error = ERR_PARAM; return 0xFFFFFFFF; }
    fh->pos = (uint32_t)(base + dist);
    return fh->pos;
}
static BOOL WINAPI k_DeleteFileA(const char *name) {
    kfile *f = fs_find(name);
    if (!f) { last_error = ERR_FILE_NOT_FOUND; return 0; }
    if (!fs_delete(name)) { last_error = ERR_ACCESS; return 0; }
    return 1;
}

/* memory */
static HANDLE WINAPI k_GetProcessHeap(void) { return H_HEAP; }
static void *WINAPI k_HeapAlloc(HANDLE heap, DWORD flags, DWORD n) {
    (void)heap;
    void *p = palloc(n);
    if (p && (flags & 0x08)) memset(p, 0, n);
    if (!p) last_error = ERR_NOMEM;
    return p;
}
static BOOL WINAPI k_HeapFree(HANDLE heap, DWORD flags, void *p) { (void)heap; (void)flags; pfree(p); return 1; }
static void *WINAPI k_HeapReAlloc(HANDLE heap, DWORD flags, void *p, DWORD n) {
    (void)heap; (void)flags;
    untrack(p);
    void *q = krealloc(p, n);
    win32_track(q ? q : p);
    return q;
}
static void *WINAPI k_VirtualAlloc(void *addr, DWORD n, DWORD type, DWORD prot) {
    (void)addr; (void)type; (void)prot;
    uint8_t *raw = palloc(n + 4096 + 16);
    if (!raw) { last_error = ERR_NOMEM; return 0; }
    uint8_t *al = (uint8_t *)(((uint32_t)raw + 16 + 4095) & ~4095u);
    ((uint32_t *)al)[-1] = (uint32_t)raw;
    memset(al, 0, n);
    return al;
}
static BOOL WINAPI k_VirtualFree(void *p, DWORD n, DWORD type) {
    (void)n; (void)type;
    if (p) pfree((void *)((uint32_t *)p)[-1]);
    return 1;
}

/* strings */
static int WINAPI k_lstrlenA(const char *s) { return s ? (int)strlen(s) : 0; }
static char *WINAPI k_lstrcpyA(char *d, const char *s) { return strcpy(d, s); }
static char *WINAPI k_lstrcatA(char *d, const char *s) { return strcat(d, s); }
static int WINAPI k_lstrcmpA(const char *a, const char *b) { int r = strcmp(a, b); return r < 0 ? -1 : r > 0; }
static int WINAPI k_lstrcmpiA(const char *a, const char *b) { int r = strcasecmp(a, b); return r < 0 ? -1 : r > 0; }

typedef struct { uint16_t year, month, dow, day, hour, min, sec, ms; } systime;
static void WINAPI k_GetLocalTime(systime *st) {
    rtc_time t; rtc_read(&t);
    st->year = (uint16_t)t.year; st->month = (uint16_t)t.month; st->dow = (uint16_t)((t.dow + 6) % 7);
    st->day = (uint16_t)t.day; st->hour = (uint16_t)t.hour; st->min = (uint16_t)t.min; st->sec = (uint16_t)t.sec; st->ms = 0;
}
static BOOL WINAPI k_GetComputerNameA(char *buf, DWORD *n) {
    const char *name = "NIGHTBOX";
    if (!n || *n <= strlen(name)) { last_error = 111; return 0; }
    strcpy(buf, name); *n = (DWORD)strlen(name); return 1;
}
static DWORD WINAPI k_GetVersion(void) { return 0x00010000u | 0x0100; } /* "NightOS 0.1" */

/* ================= user32 ================= */
int messagebox(const char *text, const char *caption, unsigned type) {
    static uint16_t saved[80 * 25];
    if (!caption) caption = "Message";
    if (!text) text = "";
    int yesno = (type & 0xF) == 4 || (type & 0xF) == 3;
    int okcancel = (type & 0xF) == 1;
    /* measure */
    int lines = 1, w = 0, cur = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '\n') { lines++; if (cur > w) w = cur; cur = 0; } else cur++;
    }
    if (cur > w) w = cur;
    if ((int)strlen(caption) > w) w = (int)strlen(caption);
    if (w < 24) w = 24;
    if (w > 70) w = 70;
    if (lines > 16) lines = 16;
    int bw = w + 6, bh = lines + 6;
    int x0 = (80 - bw) / 2, y0 = (25 - bh) / 2;
    const uint8_t body = 0x70, frame = 0x71, head = 0x1F, btn = 0x1F, btn_off = 0x78;

    serial_puts("\n[MessageBox] "); serial_puts(caption); serial_puts(": "); serial_puts(text); serial_puts("\n");
    con_save(saved);
    for (int y = 0; y < bh; y++)
        for (int x = 0; x < bw; x++) con_cell(x0 + x, y0 + y, ' ', body);
    for (int x = 0; x < bw; x++) { con_cell(x0 + x, y0, ' ', head); con_cell(x0 + x, y0 + bh - 1, (char)0xC4, frame); }
    for (int y = 1; y < bh - 1; y++) { con_cell(x0, y0 + y, (char)0xB3, frame); con_cell(x0 + bw - 1, y0 + y, (char)0xB3, frame); }
    con_cell(x0, y0 + bh - 1, (char)0xC0, frame); con_cell(x0 + bw - 1, y0 + bh - 1, (char)0xD9, frame);
    con_text(x0 + 2, y0, caption, head);
    con_text(x0 + bw - 4, y0, "[x]", head);
    for (int x = 0; x < bw; x++) con_shade(x0 + x + 2, y0 + bh);
    for (int y = 1; y <= bh; y++) { con_shade(x0 + bw, y0 + y); con_shade(x0 + bw + 1, y0 + y); }
    int ly = y0 + 2, lx = x0 + 3;
    for (const char *p = text; *p && ly < y0 + 2 + lines; p++) {
        if (*p == '\n') { ly++; lx = x0 + 3; continue; }
        if (lx < x0 + bw - 3) con_cell(lx++, ly, *p, body);
    }
    const char *b1 = yesno ? "  Yes  " : "  OK  ", *b2 = yesno ? "  No  " : okcancel ? " Cancel " : 0;
    int sel = 0, result;
    for (;;) {
        int by = y0 + bh - 2;
        int tw = (int)strlen(b1) + (b2 ? (int)strlen(b2) + 3 : 0);
        int bx = x0 + (bw - tw) / 2;
        con_text(bx, by, b1, sel == 0 ? btn : btn_off);
        if (b2) con_text(bx + (int)strlen(b1) + 3, by, b2, sel == 1 ? btn : btn_off);
        int c = kgetc();
        if (c == '\n' || c == ' ') { result = sel == 0 ? (yesno ? 6 : 1) : (yesno ? 7 : 2); break; }
        if (c == 27) { result = yesno ? 7 : (okcancel ? 2 : 1); break; }
        if (b2 && (c == KEY_LEFT || c == KEY_RIGHT || c == '\t')) sel ^= 1;
        if (yesno && (c == 'y' || c == 'Y')) { result = 6; break; }
        if (yesno && (c == 'n' || c == 'N')) { result = 7; break; }
    }
    con_restore(saved);
    serial_puts(result == 6 ? "[MessageBox] -> Yes\n" : result == 7 ? "[MessageBox] -> No\n" :
                result == 2 ? "[MessageBox] -> Cancel\n" : "[MessageBox] -> OK\n");
    return result;
}
static int WINAPI u_MessageBoxA(HANDLE wnd, const char *text, const char *cap, DWORD type) {
    (void)wnd; return messagebox(text, cap, type);
}
static int u_wsprintfA(char *buf, const char *fmt, ...) { /* cdecl, like the real one */
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(buf, 1025, fmt, ap);
    va_end(ap);
    return r > 1024 ? 1024 : r;
}
static BOOL WINAPI u_MessageBeep(DWORD t) {
    (void)t;
    outb(0x43, 0xB6); outb(0x42, (1193182 / 880) & 0xFF); outb(0x42, (1193182 / 880) >> 8);
    outb(0x61, inb(0x61) | 3); time_sleep(120); outb(0x61, inb(0x61) & ~3);
    return 1;
}

/* ================= export table ================= */
typedef struct { const char *dll; const char *name; void *fn; } export;
static const export exports[] = {
#define K(n) { "kernel32.dll", #n, (void *)k_##n }
#define U(n) { "user32.dll", #n, (void *)u_##n }
    K(ExitProcess), K(GetStdHandle), K(GetLastError), K(SetLastError), K(GetCommandLineA),
    K(GetModuleHandleA), K(GetTickCount), K(Sleep), K(GetCurrentProcessId),
    K(WriteFile), K(WriteConsoleA), K(ReadFile), K(ReadConsoleA), K(SetConsoleTextAttribute),
    K(SetConsoleTitleA), K(SetConsoleCursorPosition), K(OutputDebugStringA),
    K(CreateFileA), K(CloseHandle), K(GetFileSize), K(SetFilePointer), K(DeleteFileA),
    K(GetProcessHeap), K(HeapAlloc), K(HeapFree), K(HeapReAlloc), K(VirtualAlloc), K(VirtualFree),
    K(lstrlenA), K(lstrcpyA), K(lstrcatA), K(lstrcmpA), K(lstrcmpiA),
    K(GetLocalTime), K(GetComputerNameA), K(GetVersion),
    U(MessageBoxA), U(wsprintfA), U(MessageBeep),
    { 0, 0, 0 }
};

void *win32_lookup(const char *dll, const char *func) {
    if (!dll || !func) return 0;
    for (const export *e = exports; e->dll; e++)
        if (!strcasecmp(e->dll, dll) && !strcmp(e->name, func)) return e->fn;
    return 0;
}
