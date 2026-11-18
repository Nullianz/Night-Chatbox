/* Timekeeping: TSC calibrated against the PIT, plus the CMOS real-time clock. */
#include "kernel.h"

static uint64_t tsc0;
static uint32_t tsc_per_ms = 1000000;

void time_init(void) {
    uint8_t v = inb(0x61);
    outb(0x61, (uint8_t)((v & ~0x02) | 0x01));
    outb(0x43, 0xB0);              /* PIT channel 2, mode 0 */
    outb(0x42, 11932 & 0xFF);      /* ~10 ms */
    outb(0x42, 11932 >> 8);
    v = inb(0x61);
    outb(0x61, (uint8_t)(v & ~1));
    outb(0x61, (uint8_t)(v | 1));
    uint64_t a = rdtsc();
    for (uint32_t i = 0; i < 50000000 && !(inb(0x61) & 0x20); i++) {}
    uint64_t b = rdtsc();
    uint32_t t = udiv64(b - a, 10);
    if (t > 1000) tsc_per_ms = t;
    tsc0 = rdtsc();
}

uint32_t time_ms(void) { return udiv64(rdtsc() - tsc0, tsc_per_ms); }
void time_sleep(uint32_t ms) { uint32_t s = time_ms(); while (time_ms() - s < ms) __asm__ volatile("pause"); }

static uint8_t cmos(uint8_t r) { outb(0x70, r); return inb(0x71); }
static int bcd(int v, int is_bcd) { return is_bcd ? (v & 0x0F) + (v >> 4) * 10 : v; }

void rtc_read(rtc_time *t) {
    for (int i = 0; i < 100000 && (cmos(0x0A) & 0x80); i++) {}
    int b = !(cmos(0x0B) & 0x04);
    t->sec = bcd(cmos(0x00), b);
    t->min = bcd(cmos(0x02), b);
    int h = cmos(0x04);
    int pm = h & 0x80;
    t->hour = bcd(h & 0x7F, b);
    if (!(cmos(0x0B) & 0x02) && pm) t->hour = (t->hour % 12) + 12;
    t->dow = bcd(cmos(0x06), b);
    t->day = bcd(cmos(0x07), b);
    t->month = bcd(cmos(0x08), b);
    t->year = 2000 + bcd(cmos(0x09), b);
}
