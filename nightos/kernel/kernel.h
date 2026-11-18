/* NightOS shared kernel declarations. */
#ifndef NIGHTOS_KERNEL_H
#define NIGHTOS_KERNEL_H
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define NIGHTOS_VERSION "0.1"

/* ---- port I/O ---- */
static inline void outb(uint16_t p, uint8_t v) { __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static inline uint8_t inb(uint16_t p) { uint8_t v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }
static inline void outw(uint16_t p, uint16_t v) { __asm__ volatile("outw %0,%1" :: "a"(v), "Nd"(p)); }
static inline uint64_t rdtsc(void) { uint32_t lo, hi; __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi)); return ((uint64_t)hi << 32) | lo; }

/* ---- lib.c ---- */
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
int strcasecmp(const char *a, const char *b);
char *strcpy(char *d, const char *s);
char *strncpy0(char *d, const char *s, size_t n); /* always terminates */
char *strcat(char *d, const char *s);
char *strchr(const char *s, int c);
int tolower(int c);
int toupper(int c);
int atoi(const char *s);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int kprintf(const char *fmt, ...);
uint32_t udiv64(uint64_t n, uint32_t d);

/* ---- setjmp (cpu.asm) ---- */
typedef uint32_t kjmp_buf[6];
int k_setjmp(kjmp_buf b) __attribute__((returns_twice));
void k_longjmp(kjmp_buf b, int v) __attribute__((noreturn));
uint32_t call_on_stack(void *fn, void *stack_top);

/* ---- serial.c ---- */
void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);
int serial_ready(void);
char serial_getc(void);

/* ---- console.c ---- */
#define CON_TOP 1
#define CON_ROWS 23
#define CON_COLS 80
void con_init(void);
void con_putc(char c);
void con_write(const char *s, size_t n);
void con_puts(const char *s);
void con_set_attr(uint8_t a);
uint8_t con_get_attr(void);
void con_clear(void);
void con_set_pos(int x, int y);
void con_draw_bars(void);
void con_set_title(const char *t);
const char *con_get_title(void);
void con_save(uint16_t *buf);
void con_restore(const uint16_t *buf);
void con_cell(int x, int y, char ch, uint8_t attr);
void con_text(int x, int y, const char *s, uint8_t attr);
void con_shade(int x, int y);
void con_panic_screen(const char *title, const char *detail);

/* ---- input.c ---- */
#define KEY_LEFT  0x101
#define KEY_RIGHT 0x102
#define KEY_UP    0x103
#define KEY_DOWN  0x104
int kgetc(void);
int kreadline(char *buf, int max);

/* ---- time.c ---- */
void time_init(void);
uint32_t time_ms(void);
void time_sleep(uint32_t ms);
typedef struct { int year, month, day, dow, hour, min, sec; } rtc_time;
void rtc_read(rtc_time *t);

/* ---- mem.c ---- */
void heap_init(uint32_t start, uint32_t end);
void *kmalloc(size_t n);
void *kzalloc(size_t n);
void *krealloc(void *p, size_t n);
void kfree(void *p);
void heap_stats(uint32_t *total, uint32_t *used, uint32_t *blocks);

/* ---- fs.c (RAM filesystem) ---- */
#define FS_MAX_FILES 64
#define FS_NAME_MAX 40
typedef struct {
    int used;
    char name[FS_NAME_MAX];
    uint8_t *data;
    uint32_t size, cap;
    int readonly;
} kfile;
void fs_init(void);
kfile *fs_find(const char *name);
kfile *fs_create(const char *name);
int fs_delete(const char *name);
int fs_write(kfile *f, uint32_t pos, const void *src, uint32_t n);
void fs_truncate(kfile *f);
kfile *fs_at(int i);

/* ---- cpu.c ---- */
void gdt_init(void);
void idt_init(void);
uint8_t *cpu_teb(void);
uint8_t *cpu_peb(void);

/* ---- pe.c / win32.c ---- */
int pe_run(const char *name, const char *cmdline);
void pe_info(const char *name);
void *win32_lookup(const char *dll, const char *func);
void win32_begin(const char *cmdline, uint32_t image_base);
void win32_end(void);
void win32_set_base(uint32_t base);
void win32_track(void *p);
void prog_abort(int code) __attribute__((noreturn));
int prog_running(void);
int messagebox(const char *text, const char *caption, unsigned type);

#endif
