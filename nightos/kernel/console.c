/* VGA text console with NightOS's top bar, scrolling work area and bottom hint bar. */
#include "kernel.h"

#define VGA ((volatile uint16_t *)0xB8000)
#define ATTR_TOP 0x1F     /* white on navy */
#define ATTR_TOP_ACC 0x1E /* yellow on navy */
#define ATTR_BOT 0x30     /* black on teal */
#define ATTR_BOT_KEY 0x3F

static int cx, cy;
static uint8_t attr = 0x07;
static char title[48] = "Desktop";

static void hw_cursor(void) {
    uint16_t pos = (uint16_t)((cy + CON_TOP) * 80 + cx);
    outb(0x3D4, 0x0F); outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E); outb(0x3D5, pos >> 8);
}

void con_cell(int x, int y, char ch, uint8_t a) {
    if (x < 0 || x >= 80 || y < 0 || y >= 25) return;
    VGA[y * 80 + x] = (uint16_t)((uint8_t)ch | (a << 8));
}
void con_text(int x, int y, const char *s, uint8_t a) { while (*s) con_cell(x++, y, *s++, a); }
void con_shade(int x, int y) {
    if (x < 0 || x >= 80 || y < 0 || y >= 25) return;
    uint16_t c = VGA[y * 80 + x];
    VGA[y * 80 + x] = (uint16_t)((c & 0xFF) | (0x08 << 8));
}

void con_draw_bars(void) {
    rtc_time t; rtc_read(&t);
    char buf[80];
    for (int x = 0; x < 80; x++) con_cell(x, 0, ' ', ATTR_TOP);
    con_cell(1, 0, (char)0x0F, ATTR_TOP_ACC); /* little star */
    con_text(3, 0, "NightOS", ATTR_TOP_ACC);
    con_text(11, 0, (char[]){(char)0xB3, 0}, ATTR_TOP);
    con_text(13, 0, title, ATTR_TOP);
    snprintf(buf, sizeof buf, "%04d-%02d-%02d  %02d:%02d", t.year, t.month, t.day, t.hour, t.min);
    con_text(80 - (int)strlen(buf) - 1, 0, buf, ATTR_TOP);
    for (int x = 0; x < 80; x++) con_cell(x, 24, ' ', ATTR_BOT);
    con_text(1, 24, " \x10 Night ", 0x5F);
    con_text(12, 24, "help", ATTR_BOT_KEY); con_text(17, 24, "commands", ATTR_BOT);
    con_text(27, 24, "dir", ATTR_BOT_KEY); con_text(31, 24, "files", ATTR_BOT);
    con_text(38, 24, "<name>.exe", ATTR_BOT_KEY); con_text(49, 24, "run a program", ATTR_BOT);
    con_text(64, 24, "shutdown", ATTR_BOT_KEY);
}

void con_set_title(const char *t) { strncpy0(title, t, sizeof title); con_draw_bars(); }
const char *con_get_title(void) { return title; }

void con_clear(void) {
    for (int y = 0; y < CON_ROWS; y++)
        for (int x = 0; x < 80; x++) con_cell(x, y + CON_TOP, ' ', attr);
    cx = cy = 0; hw_cursor();
}

void con_init(void) {
    outb(0x3D4, 0x0A); outb(0x3D5, 0x0D); /* underline cursor */
    outb(0x3D4, 0x0B); outb(0x3D5, 0x0E);
    attr = 0x07;
    con_clear();
    con_draw_bars();
}

static void scroll(void) {
    for (int y = 0; y < CON_ROWS - 1; y++)
        for (int x = 0; x < 80; x++) VGA[(y + CON_TOP) * 80 + x] = VGA[(y + CON_TOP + 1) * 80 + x];
    for (int x = 0; x < 80; x++) con_cell(x, CON_TOP + CON_ROWS - 1, ' ', attr);
    cy = CON_ROWS - 1;
}

void con_putc(char c) {
    serial_putc(c);
    switch (c) {
    case '\n': cx = 0; cy++; break;
    case '\r': cx = 0; break;
    case '\b': if (cx > 0) cx--; break;
    case '\t': cx = (cx + 8) & ~7; break;
    default: con_cell(cx, cy + CON_TOP, c, attr); cx++; break;
    }
    if (cx >= 80) { cx = 0; cy++; }
    if (cy >= CON_ROWS) scroll();
    hw_cursor();
}
void con_write(const char *s, size_t n) { while (n--) con_putc(*s++); }
void con_puts(const char *s) { while (*s) con_putc(*s++); }
void con_set_attr(uint8_t a) { attr = a; }
uint8_t con_get_attr(void) { return attr; }
void con_set_pos(int x, int y) {
    if (x < 0) x = 0;
    if (x > 79) x = 79;
    if (y < 0) y = 0;
    if (y >= CON_ROWS) y = CON_ROWS - 1;
    cx = x; cy = y; hw_cursor();
}

void con_save(uint16_t *buf) { for (int i = 0; i < 80 * 25; i++) buf[i] = VGA[i]; }
void con_restore(const uint16_t *buf) { for (int i = 0; i < 80 * 25; i++) VGA[i] = buf[i]; }

/* Full-screen kernel fault screen (only used when the kernel itself crashes). */
void con_panic_screen(const char *head, const char *detail) {
    for (int i = 0; i < 80 * 25; i++) VGA[i] = (uint16_t)(' ' | (0x5F << 8));
    con_text(4, 3, "(-_-) zzz", 0x5E);
    con_text(4, 5, "NightOS fell asleep and could not wake up.", 0x5F);
    con_text(4, 7, head, 0x5F);
    con_text(4, 8, detail, 0x5F);
    con_text(4, 11, "Please restart your computer.", 0x5F);
    serial_puts("\n*** KERNEL FAULT: "); serial_puts(head); serial_puts(" | "); serial_puts(detail); serial_puts("\n");
}
