/* PS/2 keyboard (polled) + serial input, with a simple line editor. */
#include "kernel.h"

static const char map_lo[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n', 0, 'a','s',
    'd','f','g','h','j','k','l',';','\'','`', 0, '\\','z','x','c','v',
    'b','n','m',',','.','/', 0, '*', 0, ' ',
};
static const char map_hi[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0, 'A','S',
    'D','F','G','H','J','K','L',':','"','~', 0, '|','Z','X','C','V',
    'B','N','M','<','>','?', 0, '*', 0, ' ',
};
static int shift, caps, ext;

static int kbd_poll(void) {
    uint8_t st = inb(0x64);
    if (!(st & 1)) return -1;
    uint8_t sc = inb(0x60);
    if (st & 0x20) return -1; /* mouse byte */
    if (sc == 0xE0) { ext = 1; return -1; }
    if (ext) {
        ext = 0;
        switch (sc) {
        case 0x4B: return KEY_LEFT; case 0x4D: return KEY_RIGHT;
        case 0x48: return KEY_UP;   case 0x50: return KEY_DOWN;
        case 0x1C: return '\n';
        }
        return -1;
    }
    if (sc & 0x80) {
        sc &= 0x7F;
        if (sc == 0x2A || sc == 0x36) shift = 0;
        return -1;
    }
    if (sc == 0x2A || sc == 0x36) { shift = 1; return -1; }
    if (sc == 0x3A) { caps = !caps; return -1; }
    if (sc == 0x4B) return KEY_LEFT;
    if (sc == 0x4D) return KEY_RIGHT;
    if (sc == 0x48) return KEY_UP;
    if (sc == 0x50) return KEY_DOWN;
    char c = shift ? map_hi[sc] : map_lo[sc];
    if (caps && c >= 'a' && c <= 'z') c -= 32;
    else if (caps && c >= 'A' && c <= 'Z') c += 32;
    return c ? (uint8_t)c : -1;
}

int kgetc(void) {
    uint32_t last = time_ms();
    for (;;) {
        int c = kbd_poll();
        if (c >= 0) return c;
        if (serial_ready()) {
            char s = serial_getc();
            if (s == '\r') return '\n';
            if (s == 0x7F) return '\b';
            if (s == 0x1B) return 27;
            return (uint8_t)s;
        }
        uint32_t now = time_ms();
        if (now - last > 1000) { last = now; con_draw_bars(); } /* keep the clock ticking */
    }
}

int kreadline(char *buf, int max) {
    int n = 0;
    for (;;) {
        int c = kgetc();
        if (c == '\n') { con_putc('\n'); break; }
        if (c == '\b') { if (n) { n--; con_puts("\b \b"); } continue; }
        if (c < 32 || c > 126) continue;
        if (n < max - 1) { buf[n++] = (char)c; con_putc((char)c); }
    }
    buf[n] = 0;
    return n;
}
