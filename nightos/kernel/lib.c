/* NightOS minimal C library. */
#include "kernel.h"

void *memset(void *d, int c, size_t n) { uint8_t *p = d; while (n--) *p++ = (uint8_t)c; return d; }
void *memcpy(void *d, const void *s, size_t n) { uint8_t *a = d; const uint8_t *b = s; while (n--) *a++ = *b++; return d; }
void *memmove(void *d, const void *s, size_t n) {
    uint8_t *a = d; const uint8_t *b = s;
    if (a < b) while (n--) *a++ = *b++;
    else { a += n; b += n; while (n--) *--a = *--b; }
    return d;
}
int memcmp(const void *x, const void *y, size_t n) {
    const uint8_t *a = x, *b = y;
    for (; n; n--, a++, b++) if (*a != *b) return *a - *b;
    return 0;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return (uint8_t)*a - (uint8_t)*b; }
int strncmp(const char *a, const char *b, size_t n) {
    for (; n; n--, a++, b++) { if (*a != *b || !*a) return (uint8_t)*a - (uint8_t)*b; }
    return 0;
}
int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
int toupper(int c) { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
int strcasecmp(const char *a, const char *b) {
    while (*a && tolower(*a) == tolower(*b)) a++, b++;
    return tolower((uint8_t)*a) - tolower((uint8_t)*b);
}
char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)); return r; }
char *strncpy0(char *d, const char *s, size_t n) {
    size_t i = 0;
    if (!n) return d;
    for (; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
    return d;
}
char *strcat(char *d, const char *s) { strcpy(d + strlen(d), s); return d; }
char *strchr(const char *s, int c) { for (; *s; s++) if (*s == (char)c) return (char *)s; return c ? 0 : (char *)s; }
int atoi(const char *s) {
    int sign = 1, v = 0;
    while (*s == ' ') s++;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return v * sign;
}

uint32_t udiv64(uint64_t n, uint32_t d) {
    uint32_t hi = (uint32_t)(n >> 32), lo = (uint32_t)n, rem, q;
    rem = hi % d;
    __asm__("divl %4" : "=a"(q), "=d"(rem) : "a"(lo), "d"(rem), "rm"(d));
    return q;
}

/* printf core shared by the kernel and the Win32 wsprintfA shim */
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    size_t o = 0;
#define EMIT(ch) do { if (o + 1 < n) buf[o] = (ch); o++; } while (0)
    for (; *fmt; fmt++) {
        if (*fmt != '%') { EMIT(*fmt); continue; }
        fmt++;
        int left = 0, zero = 0, width = 0, lng = 0;
        if (*fmt == '-') { left = 1; fmt++; }
        if (*fmt == '0') { zero = 1; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        while (*fmt == 'l' || *fmt == 'h') { lng++; fmt++; }
        (void)lng;
        char tmp[16]; const char *s = tmp; int len = 0, neg = 0;
        switch (*fmt) {
        case 'd': case 'i': {
            int v = va_arg(ap, int); uint32_t u = v < 0 ? (uint32_t)-v : (uint32_t)v; neg = v < 0;
            do { tmp[15 - len++] = '0' + u % 10; u /= 10; } while (u);
            s = tmp + 16 - len; break;
        }
        case 'u': {
            uint32_t u = va_arg(ap, uint32_t);
            do { tmp[15 - len++] = '0' + u % 10; u /= 10; } while (u);
            s = tmp + 16 - len; break;
        }
        case 'x': case 'X': case 'p': {
            uint32_t u = va_arg(ap, uint32_t);
            const char *dg = *fmt == 'x' ? "0123456789abcdef" : "0123456789ABCDEF";
            if (*fmt == 'p') { width = 8; zero = 1; }
            do { tmp[15 - len++] = dg[u & 15]; u >>= 4; } while (u);
            s = tmp + 16 - len; break;
        }
        case 'c': tmp[0] = (char)va_arg(ap, int); len = 1; break;
        case 's': s = va_arg(ap, const char *); if (!s) s = "(null)"; len = (int)strlen(s); break;
        case '%': tmp[0] = '%'; len = 1; break;
        case 0: fmt--; continue;
        default: tmp[0] = '%'; tmp[1] = *fmt; len = 2; break;
        }
        int pad = width - len - neg;
        if (neg && zero) EMIT('-');
        if (!left) while (pad-- > 0) EMIT(zero ? '0' : ' ');
        if (neg && !zero) EMIT('-');
        for (int i = 0; i < len; i++) EMIT(s[i]);
        if (left) while (pad-- > 0) EMIT(' ');
    }
    if (n) buf[o < n ? o : n - 1] = 0;
    return (int)o;
#undef EMIT
}

int snprintf(char *buf, size_t n, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, n, fmt, ap); va_end(ap); return r;
}

int kprintf(const char *fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    con_puts(buf);
    return r;
}
