/* COM1 serial port: mirrors the console and accepts input (handy for headless testing). */
#include "kernel.h"
#define COM1 0x3F8

void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x01); /* 115200 baud */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); /* 8N1 */
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}
void serial_putc(char c) {
    if (c == '\n') serial_putc('\r');
    for (int i = 0; i < 100000 && !(inb(COM1 + 5) & 0x20); i++) {}
    outb(COM1, (uint8_t)c);
}
void serial_puts(const char *s) { while (*s) serial_putc(*s++); }
int serial_ready(void) { return inb(COM1 + 5) & 1; }
char serial_getc(void) { return (char)inb(COM1); }
